add_executable(l2_cache_fixture_test cache_fixture_test.cc)
target_include_directories(l2_cache_fixture_test PRIVATE
    "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/libcuda"
    ${CUDAToolkit_INCLUDE_DIRS})
if(GPGPUSIM_USE_POWER_MODEL)
  target_compile_definitions(l2_cache_fixture_test PRIVATE GPGPUSIM_POWER_MODEL)
  target_include_directories(l2_cache_fixture_test PRIVATE ${GPGPUSIM_POWER_MODEL})
endif()
target_link_libraries(l2_cache_fixture_test PRIVATE cudart)
add_test(NAME l2_cache_fixture COMMAND l2_cache_fixture_test)

add_executable(l2_observer_lifecycle_test observer_lifecycle_test.cc)
target_include_directories(l2_observer_lifecycle_test PRIVATE
    "${PROJECT_SOURCE_DIR}/src/gpgpu-sim")
add_test(NAME l2_observer_lifecycle COMMAND l2_observer_lifecycle_test)
set_tests_properties(l2_cache_fixture l2_observer_lifecycle
    PROPERTIES TIMEOUT 30)
