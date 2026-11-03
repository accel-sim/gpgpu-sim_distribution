// SPDX-License-Identifier: BSD-3-Clause

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "l2-cache-observer.h"

namespace {

void Require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}

void First(void *, const l2_cache_access_event &) noexcept {}
void Second(void *, const l2_cache_access_event &) noexcept {}

}  // namespace

int main() {
  try {
    l2_cache_observer first_instance;
    l2_cache_observer second_instance;
    int first_context = 0;
    int second_context = 0;
    using status = l2_observer_status;

    Require(!first_instance.has_observer(), "observer is enabled by default");
    Require(
        first_instance.register_callback(nullptr) == status::invalid_callback,
        "null callback registration was accepted");
    Require(
        first_instance.unregister_callback(nullptr) == status::invalid_callback,
        "null callback unregistration was accepted");
    Require(
        first_instance.unregister_callback(&First) == status::not_registered,
        "unregistration succeeded on an empty instance");

    Require(first_instance.register_callback(&First, &first_context) ==
                status::success,
            "first registration failed");
    Require(first_instance.register_callback(&Second, &second_context) ==
                status::already_registered,
            "duplicate registration replaced the original observer");
    Require(first_instance.unregister_callback(&Second, &first_context) ==
                status::observer_mismatch,
            "wrong callback removed the observer");
    Require(first_instance.unregister_callback(&First, &second_context) ==
                status::observer_mismatch,
            "wrong context removed the observer");
    Require(first_instance.has_observer(),
            "mismatched unregistration cleared state");

    Require(second_instance.register_callback(&Second) == status::success,
            "null context or a concurrent instance was rejected");
    Require(first_instance.unregister_callback(&First, &first_context) ==
                status::success,
            "duplicate registration did not preserve the first observer");
    Require(!first_instance.has_observer() && second_instance.has_observer(),
            "one instance changed another instance's observer");
    Require(first_instance.unregister_callback(&First, &first_context) ==
                status::not_registered,
            "repeated unregistration unexpectedly succeeded");
    Require(first_instance.register_callback(&Second) == status::success,
            "registration after unregistration failed");
    Require(first_instance.unregister_callback(&Second) == status::success,
            "null-context unregistration failed");
    Require(second_instance.unregister_callback(&Second) == status::success,
            "second instance lost its registration");

    std::cout << "OBSERVER_LIFECYCLE_PASS\n";
    return EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::cerr << "OBSERVER_LIFECYCLE_FAIL: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
