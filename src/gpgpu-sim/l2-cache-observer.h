// SPDX-License-Identifier: BSD-3-Clause

#ifndef GPGPU_SIM_L2_CACHE_OBSERVER_H
#define GPGPU_SIM_L2_CACHE_OBSERVER_H

struct l2_cache_access_event;
class data_cache;

enum class l2_observer_status {
  success,
  invalid_callback,
  already_registered,
  not_registered,
  observer_mismatch
};

// One non-owning observer per simulator instance. Registration, unregistration
// and simulation must run on the same thread, outside an observer callback.
// The callback must not re-enter or modify the simulator. The event is valid
// only during the callback; copy it if it is needed afterwards.
class l2_cache_observer {
 public:
  using callback_type = void (*)(void *,
                                 const l2_cache_access_event &) noexcept;

  l2_cache_observer() = default;
  l2_cache_observer(const l2_cache_observer &) = delete;
  l2_cache_observer &operator=(const l2_cache_observer &) = delete;

  l2_observer_status register_callback(callback_type callback,
                                       void *context = nullptr) noexcept {
    if (!callback) return l2_observer_status::invalid_callback;
    if (callback_) return l2_observer_status::already_registered;
    context_ = context;
    callback_ = callback;
    return l2_observer_status::success;
  }

  l2_observer_status unregister_callback(callback_type callback,
                                         void *context = nullptr) noexcept {
    if (!callback) return l2_observer_status::invalid_callback;
    if (!callback_) return l2_observer_status::not_registered;
    if (callback != callback_ || context != context_)
      return l2_observer_status::observer_mismatch;
    callback_ = nullptr;
    context_ = nullptr;
    return l2_observer_status::success;
  }

  bool has_observer() const noexcept { return callback_ != nullptr; }

 private:
  friend class data_cache;

  void notify(const l2_cache_access_event &event) const noexcept {
    callback_(context_, event);
  }

  callback_type callback_ = nullptr;
  void *context_ = nullptr;
};

#endif  // GPGPU_SIM_L2_CACHE_OBSERVER_H
