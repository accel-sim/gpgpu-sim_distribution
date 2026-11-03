# Accepted L2 cache access observer

`gpgpu_sim::l2_cache_events` provides one optional observer per simulator
instance. It reports accepted calls to the production `data_cache::access()`
path at L2 without changing its return value, native statistics, or requests.
No observer is registered by default. There is no RTL, PMU, tracing library,
file output, or additional build dependency.

## Use

Include `gpu-sim.h` and register a non-throwing callback before simulation:

```cpp
void observe(void *context, const l2_cache_access_event &event) noexcept {
  // Consume the immutable event; the context may be null.
}

auto status = gpu->l2_cache_events.register_callback(observe, context);
// Check status == l2_observer_status::success before running the simulation.
// ... run the simulator ...
status = gpu->l2_cache_events.unregister_callback(observe, context);
```

The caller owns the callback context and must unregister it before destroying
it. Registration, unregistration, and simulation run on the same thread.
Callbacks must not re-enter or modify the simulator, change registrations, or
retain the event reference. Copy the record if it must outlive the callback.
Different simulator instances have independent registrations. This is a C++
source interface, not a versioned binary or C ABI.

A null callback is rejected; a null context is allowed. Registering twice does
not replace an existing observer. Unregistration must match both the callback
and context. Invalid operations leave the current registration unchanged.

## Event semantics

There is one event after both native per-stream statistics updates for an
accepted L2 access. Its `status` is the same `select_stats_status()` result
used for native accounting: `HIT`, `HIT_RESERVED`, `MISS`, or `SECTOR_MISS`.
This differs from the access return value for a pending hit or sector miss.
`RESERVATION_FAIL` produces no event; a later accepted retry produces an event.
Supplementary `MSHR_HIT` and `WRITE_ALLOCATED` counters do not produce extra
events. No event is emitted for L1 accesses or cache fills.

All access types reaching this path are observable. A consumer interested in
accepted global reads and writes filters `access_type` for `GLOBAL_ACC_R` and
`GLOBAL_ACC_W`. A non-hit in that scope includes `HIT_RESERVED`, `MISS`, and
`SECTOR_MISS`; it is not the raw tag-miss count.

- `gpu_cycle` is the simulator's 64-bit global cycle at notification time.
- `cache_time` preserves the original unsigned cache-call time. It can have a
  memcpy offset or wrap independently of `gpu_cycle`.
- `address` is the original `access()` argument, even if a miss changes the
  request object's address. `atom_size` is the cache atom size in bytes, not
  the original request size. The request ID has the simulator's native unsigned
  width; it is not a globally unique, indefinitely increasing identifier.
- `stream_id`, `sub_partition_id`, `sector_mask`, and `is_atomic` describe the
  request presented to this cache path.

An event counts a cache access, not an instruction or universally one sector.
A normal cache has a line-sized atom; sector configurations have smaller atoms.
For Hopper, observation is after L2 request coalescing. Requests absorbed by
the coalescer are not separate accesses here. Chiplet access types are retained
when they reach this path, but direct forwarded-write tag updates, disabled L2,
bypass traffic, and memcpy tag prefills are outside its scope.

## Cost and tests

With no observer, the path checks the cache level and the callback pointer;
it does not construct an event, allocate, lock, format, or perform I/O. This
is not a claim of zero execution overhead. With an observer, notification is
synchronous and callback cost belongs to the consumer.

Configure the normal simulator build with
`-DGPGPUSIM_BUILD_L2_OBSERVER_TESTS=ON`, build it, and run
`cd <build-directory> && ctest --output-on-failure --no-tests=error`.
Tests use directed requests through the actual cache, tag array, and MSHRs,
with a deterministic lower-memory port. They are cache integration tests,
not a CUDA workload or a Hopper coalescer/chiplet integration run.
