// SPDX-License-Identifier: BSD-3-Clause
// Directed requests exercise the production data_cache, tag array and MSHRs.
// No CUDA application, GPU hardware, trace file or mock cache is required.

#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "gpgpu-sim/gpu-cache.h"
#include "gpgpu-sim/gpu-sim.h"
#include "gpgpu-sim/mem_fetch.h"
#include "gpgpu_context.h"
#include "option_parser.h"

namespace {

void Require(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}

class LowerMemory final : public mem_fetch_interface {
 public:
  bool full(unsigned, bool) const override { return false; }
  void push(mem_fetch *request) override { requests.push_back(request); }
  std::vector<mem_fetch *> requests;
};

// The directed requests do not evict dirty lines or fetch data on write misses.
// An unexpected allocator call is a test failure instead of a fake response.
class UnexpectedAllocator final : public mem_fetch_allocator {
 public:
  mem_fetch *alloc(new_addr_type, mem_access_type, unsigned, bool,
                   unsigned long long, unsigned long long) const override {
    throw std::runtime_error("unexpected short allocation");
  }
  mem_fetch *alloc(const warp_inst_t &, const mem_access_t &,
                   unsigned long long) const override {
    throw std::runtime_error("unexpected instruction allocation");
  }
  mem_fetch *alloc(new_addr_type, mem_access_type, const active_mask_t &,
                   const mem_access_byte_mask_t &,
                   const mem_access_sector_mask_t &, unsigned, bool,
                   unsigned long long, unsigned, unsigned, unsigned,
                   mem_fetch *, unsigned long long) const override {
    throw std::runtime_error("unexpected writeback allocation");
  }
};

#ifndef L2_OBSERVER_TEST_BASELINE
struct Recorder {
  static void Record(void *context,
                     const l2_cache_access_event &event) noexcept {
    auto &recorder = *static_cast<Recorder *>(context);
    if (recorder.count == recorder.events.size()) {
      recorder.overflow = true;
      return;
    }
    recorder.events[recorder.count++] = event;
  }
  std::array<l2_cache_access_event, 32> events{};
  std::size_t count = 0;
  bool overflow = false;
};
#endif

class SimulatorFixture {
 public:
  SimulatorFixture() : configuration(&context) {
    option_parser_t parser = option_parser_create();
    context.ptx_reg_options(parser);
    context.func_sim->ptx_opcocde_latency_options(parser);
    configuration.reg_options(parser);
    const char *arguments[] = {"cache_fixture_test",
                               "-SST_mode",
                               "1",
                               "-gpgpu_n_clusters",
                               "1",
                               "-gpgpu_n_cores_per_cluster",
                               "1",
                               "-gpgpu_n_mem",
                               "1",
                               "-gpgpu_n_chiplet_partition",
                               "1",
                               "-visualizer_enabled",
                               "0",
                               "-gpgpu_tensor_core_avail",
                               "1",
                               "-gpgpu_num_tensor_core_units",
                               "1",
                               "-gpgpu_tex_cache:l1",
                               "N:4:128:2,L:R:m:N:L,F:8:4,8:4",
                               "-gpgpu_const_cache:l1",
                               "N:4:64:2,L:R:f:N:L,A:2:4,4:4,32",
                               "-gpgpu_cache:il1",
                               "N:4:128:2,L:R:f:N:L,A:2:4,4:4,32"};
    option_parser_cmdline(parser, sizeof(arguments) / sizeof(arguments[0]),
                          arguments);
    configuration.init();
    gpu = std::make_unique<sst_gpgpu_sim>(configuration, &context);
    option_parser_destroy(parser);
  }

  std::unique_ptr<mem_fetch> Request(new_addr_type address,
                                     mem_access_type type = GLOBAL_ACC_R,
                                     unsigned subpartition = 0) {
    active_mask_t active;
    active.set(0);
    mem_access_byte_mask_t bytes;
    const unsigned offset = address % 128;
    for (unsigned byte = offset; byte < offset + 32; ++byte) bytes.set(byte);
    mem_access_sector_mask_t sectors;
    sectors.set(offset / 32);
    const bool write = type == GLOBAL_ACC_W || type == CHIPLET_ACC_W;
    mem_access_t access(type, address, 32, write, active, bytes, sectors,
                        &context);
    auto request = std::make_unique<mem_fetch>(
        access, static_cast<const warp_inst_t *>(nullptr), 17, 8, 2, 3, 0,
        gpu->getMemoryConfig(), 0);
    // SST mode skips physical routing, which is irrelevant to a cache unit
    // test. Initialize the routing fields used by cache observations
    // explicitly.
    request->set_chip(0);
    request->set_partition(subpartition);
    return request;
  }

  gpgpu_context context;
  gpgpu_sim_config configuration;
  std::unique_ptr<gpgpu_sim> gpu;
#ifndef L2_OBSERVER_TEST_BASELINE
  Recorder recorder;
  bool expect_events = false;
#endif
};

class CacheFixture {
 public:
  CacheFixture(SimulatorFixture &simulator, const char *name,
               cache_gpu_level level, const std::string &configuration)
      : simulator(simulator), level(level), configuration_text(configuration) {
    config.init(configuration_text.data(), FuncCachePreferNone);
    cache = std::make_unique<data_cache>(
        name, config, 0, 0, &memory, &allocator, IN_PARTITION_L2_MISS_QUEUE,
        L2_WR_ALLOC_R, L2_WRBK_ACC, simulator.gpu.get(), level);
  }

  cache_request_status Access(mem_fetch *request, unsigned time,
                              cache_request_status expected_return,
                              cache_request_status expected_stat) {
    mem_access_type type = request->get_access_type();
    const auto before = cache->get_stats(&type, 1, &expected_stat, 1);
#ifndef L2_OBSERVER_TEST_BASELINE
    const auto observations_before = simulator.recorder.count;
    const auto address_before = request->get_addr();
#endif
    std::list<cache_event> events;
    const auto actual =
        cache->access(request->get_addr(), request, time, events);
    Require(actual == expected_return,
            "return status: expected " +
                std::string(cache_request_status_str(expected_return)) +
                ", got " + cache_request_status_str(actual));
    Require(cache->get_stats(&type, 1, &expected_stat, 1) == before + 1,
            "native statistics did not count " +
                std::string(cache_request_status_str(expected_stat)));
#ifndef L2_OBSERVER_TEST_BASELINE
    const bool accepted_l2 = simulator.expect_events && level == L2_GPU_CACHE &&
                             expected_return != RESERVATION_FAIL;
    Require(simulator.recorder.count == observations_before + accepted_l2,
            "wrong number of observer callbacks for one cache access");
    if (accepted_l2) {
      const auto &event = simulator.recorder.events[observations_before];
      Require(event.gpu_cycle == simulator.gpu->global_cycle(),
              "observer lost the 64-bit GPU cycle");
      Require(event.cache_time == time && event.gpu_cycle != time,
              "cache time was conflated with GPU cycle");
      Require(event.address == address_before &&
                  event.request_uid == request->get_request_uid() &&
                  event.stream_id == request->get_streamID() &&
                  event.sub_partition_id == request->get_sub_partition_id() &&
                  event.access_type == type && event.status == expected_stat &&
                  event.sector_mask ==
                      request->get_access_sector_mask().to_ulong() &&
                  event.atom_size == config.get_atom_sz() &&
                  event.is_atomic == request->isatomic(),
              "observer event differs from the accepted cache request");
    }
#endif
    returns.push_back(actual);
    cache->cycle();
    return actual;
  }

  void Fill(mem_fetch *request, unsigned time, unsigned expected_ready) {
#ifndef L2_OBSERVER_TEST_BASELINE
    const auto observations_before = simulator.recorder.count;
#endif
    Require(cache->waiting_for_fill(request), "request is not pending a fill");
    cache->fill(request, time);
    unsigned ready = 0;
    while (cache->access_ready()) {
      cache->next_access();
      ++ready;
    }
    Require(ready == expected_ready,
            "unexpected count of completed MSHR requests");
    cache->cycle();
#ifndef L2_OBSERVER_TEST_BASELINE
    Require(simulator.recorder.count == observations_before,
            "fill or queue retirement emitted an access event");
#endif
  }

  std::string Summary(const std::string &name, mem_access_type type) const {
    std::string output = name;
    for (auto status : {HIT, HIT_RESERVED, MISS, SECTOR_MISS, RESERVATION_FAIL,
                        MSHR_HIT, WRITE_ALLOCATED}) {
      output += " " + std::string(cache_request_status_str(status)) + "=" +
                std::to_string(cache->get_stats(&type, 1, &status, 1));
    }
    output += " sent=" + std::to_string(memory.requests.size()) + " returns=";
    for (const auto status : returns)
      output += std::to_string(static_cast<unsigned>(status)) + ",";
    return output;
  }

  SimulatorFixture &simulator;
  cache_gpu_level level;
  std::string configuration_text;
  cache_config config;
  LowerMemory memory;
  UnexpectedAllocator allocator;
  std::unique_ptr<data_cache> cache;
  std::vector<cache_request_status> returns;
};

struct RunResult {
  std::vector<std::string> summaries;
  // PerfCounter stores references into each cache's statistics. Retain the
  // caches until all runs finish so those references remain valid.
  std::vector<std::unique_ptr<CacheFixture>> caches;
};

RunResult RunCases(SimulatorFixture &simulator) {
  RunResult result;
  auto &summaries = result.summaries;
  result.caches.push_back(
      std::make_unique<CacheFixture>(simulator, "fixture_l2", L2_GPU_CACHE,
                                     "S:4:128:2,L:B:m:N:L,A:4:4,8:4,32"));
  auto &l2 = *result.caches.back();
  auto cold = simulator.Request(0x1000);
  auto merged = simulator.Request(0x1000);
  auto hit = simulator.Request(0x1000);
  auto sector = simulator.Request(0x1020);
  auto write_hit = simulator.Request(0x1000, GLOBAL_ACC_W);
  auto local_hit = simulator.Request(0x1000, LOCAL_ACC_R);
  auto chiplet_read = simulator.Request(0x1000, CHIPLET_ACC_R);
  auto chiplet_write = simulator.Request(0x1000, CHIPLET_ACC_W);
  l2.Access(cold.get(), 1, MISS, MISS);
  // Access return and recorded statistics intentionally differ for a merge.
  l2.Access(merged.get(), 2, MISS, HIT_RESERVED);
  Require(l2.memory.requests.size() == 1,
          "merged read issued a second request");
  l2.Fill(cold.get(), 3, 2);
  l2.Access(hit.get(), 4, HIT, HIT);
  l2.Access(sector.get(), 5, MISS, SECTOR_MISS);
  l2.Fill(sector.get(), 6, 1);
  l2.Access(write_hit.get(), 7, HIT, HIT);
  l2.Access(local_hit.get(), 8, HIT, HIT);
  l2.Access(chiplet_read.get(), 9, HIT, HIT);
  l2.Access(chiplet_write.get(), 10, HIT, HIT);
  summaries.push_back(l2.Summary("l2_read", GLOBAL_ACC_R));
  summaries.push_back(l2.Summary("l2_write", GLOBAL_ACC_W));
  summaries.push_back(l2.Summary("l2_local", LOCAL_ACC_R));
  summaries.push_back(l2.Summary("l2_chiplet_read", CHIPLET_ACC_R));
  summaries.push_back(l2.Summary("l2_chiplet_write", CHIPLET_ACC_W));

  result.caches.push_back(std::make_unique<CacheFixture>(
      simulator, "fixture_mshr_full", L2_GPU_CACHE,
      "S:4:128:2,L:B:m:N:L,A:1:4,8:4,32"));
  auto &exhausted = *result.caches.back();
  auto pending = simulator.Request(0x2000);
  auto rejected = simulator.Request(0x2080);
  exhausted.Access(pending.get(), 10, MISS, MISS);
  exhausted.Access(rejected.get(), 11, RESERVATION_FAIL, RESERVATION_FAIL);
  Require(exhausted.memory.requests.size() == 1,
          "reservation failure issued a downstream request");
  exhausted.Fill(pending.get(), 12, 1);
  exhausted.Access(rejected.get(), 13, MISS, MISS);
  Require(exhausted.memory.requests.size() == 2,
          "accepted retry did not issue exactly one new request");
  exhausted.Fill(rejected.get(), 14, 1);
  summaries.push_back(exhausted.Summary("l2_exhausted", GLOBAL_ACC_R));

  result.caches.push_back(
      std::make_unique<CacheFixture>(simulator, "fixture_l1", L1_GPU_CACHE,
                                     "S:4:128:2,L:B:m:N:L,A:4:4,8:4,32"));
  auto &l1 = *result.caches.back();
  auto l1_cold = simulator.Request(0x3000);
  auto l1_hit = simulator.Request(0x3000);
  l1.Access(l1_cold.get(), 20, MISS, MISS);
  l1.Fill(l1_cold.get(), 21, 1);
  l1.Access(l1_hit.get(), 22, HIT, HIT);
  summaries.push_back(l1.Summary("l1_read", GLOBAL_ACC_R));

  result.caches.push_back(
      std::make_unique<CacheFixture>(simulator, "fixture_normal", L2_GPU_CACHE,
                                     "N:4:128:2,L:B:m:N:L,A:4:4,8:4,32"));
  auto &normal = *result.caches.back();
  auto unaligned = simulator.Request(0x1040);
  auto normal_hit = simulator.Request(0x1040);
  normal.Access(unaligned.get(), 30, MISS, MISS);
  Require(unaligned->get_addr() == 0x1000 && unaligned->get_data_size() == 128,
          "normal-cache miss did not exercise request address/size mutation");
  normal.Fill(unaligned.get(), 31, 1);
  normal.Access(normal_hit.get(), 32, HIT, HIT);
  summaries.push_back(normal.Summary("l2_normal", GLOBAL_ACC_R));

  result.caches.push_back(std::make_unique<CacheFixture>(
      simulator, "fixture_write_allocate", L2_GPU_CACHE,
      "S:4:128:2,L:B:m:L:L,A:4:4,8:4,32"));
  auto &write_allocate = *result.caches.back();
  auto write_miss = simulator.Request(0x4000, GLOBAL_ACC_W);
  auto allocated_hit = simulator.Request(0x4000);
  write_allocate.Access(write_miss.get(), 40, MISS, MISS);
  mem_access_type write_type = GLOBAL_ACC_W;
  cache_request_status supplementary = WRITE_ALLOCATED;
  Require(
      write_allocate.cache->get_stats(&write_type, 1, &supplementary, 1) == 1,
      "write-allocate test did not exercise supplementary accounting");
  write_allocate.Access(allocated_hit.get(), 41, HIT, HIT);
  Require(write_allocate.memory.requests.empty(),
          "full-sector lazy allocation unexpectedly fetched data");
  summaries.push_back(
      write_allocate.Summary("l2_write_allocate", GLOBAL_ACC_W));
  summaries.push_back(
      write_allocate.Summary("l2_allocated_read", GLOBAL_ACC_R));
  return result;
}

}  // namespace

int main() {
  try {
    SimulatorFixture simulator;
    simulator.gpu->gpu_tot_sim_cycle = (1ULL << 32) + 90;
    simulator.gpu->gpu_sim_cycle = 7;
    const auto baseline = RunCases(simulator);
#ifndef L2_OBSERVER_TEST_BASELINE
    Require(simulator.gpu->l2_cache_events.register_callback(
                &Recorder::Record, &simulator.recorder) ==
                l2_observer_status::success,
            "observer registration failed");
    simulator.expect_events = true;
    const auto observed = RunCases(simulator);
    Require(!simulator.recorder.overflow && simulator.recorder.count == 14,
            "expected fourteen accepted L2 cache-call events");
    unsigned global_events = 0;
    for (std::size_t index = 0; index < simulator.recorder.count; ++index) {
      const auto type = simulator.recorder.events[index].access_type;
      if (type == GLOBAL_ACC_R || type == GLOBAL_ACC_W) ++global_events;
    }
    Require(global_events == 11,
            "consumer GLOBAL read/write filter is incorrect");
    Require(simulator.gpu->l2_cache_events.unregister_callback(
                &Recorder::Record, &simulator.recorder) ==
                l2_observer_status::success,
            "observer unregistration failed");
    simulator.expect_events = false;
    const auto after_unregister = RunCases(simulator);
    Require(
        baseline.summaries == observed.summaries &&
            baseline.summaries == after_unregister.summaries,
        "observer changed native statistics, access results or queue traffic");
    std::cout << "CACHE_OBSERVER accepted=14 global_consumer=11"
                 " baseline_equal=true after_unregister_equal=true\n";
#endif
    for (const auto &summary : baseline.summaries)
      std::cout << "CACHE_NATIVE " << summary << '\n';
    std::cout << "CACHE_FIXTURE_PASS\n";
    return EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::cerr << "CACHE_FIXTURE_FAIL: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
