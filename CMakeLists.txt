cmake_minimum_required(VERSION 3.17)

# Project name and version
project(GPGPU-Sim 
        VERSION 4.2.0 
        DESCRIPTION "cycle-level simulator modeling contemporary graphics processing units (GPUs)" 
        HOMEPAGE_URL https://github.com/accel-sim/gpgpu-sim_distribution
        LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# GPGPU-Sim build option
option(GPGPUSIM_ENABLE_TRACE "Whether to enable GPGPU-Sim debug tracing" ON)

# GPGPU-Sim conditional build variable
set(GPGPUSIM_USE_POWER_MODEL False)
set(GPGPUSIM_USE_OPENCL False)

include(gpgpusim_check.cmake)
include(gen_setup_environment.cmake)

# CMake target
# GPGPU-Sim CUDA Runtime lib
add_library(cudart SHARED $<TARGET_OBJECTS:gpgpusim_entrypoint>)
set_target_properties(cudart PROPERTIES 
                      SOVERSION 11.0)
add_subdirectory(src)

# Set linker option for libcudart.so
target_link_options(cudart 
                    PUBLIC $<$<BOOL:UNIX>:--version-script=linux-so-version.txt>
                    PUBLIC $<$<BOOL:UNIX>:-lm -lz -lGL -lpthread>
                    PUBLIC $<$<BOOL:APPLE>:-headerpad_max_install_names -undefined,dynamic_lookup -compatibility_version 1.1 -current_version 1.1>
                    PUBLIC $<$<BOOL:APPLE>:-lm -lz -lpthread>)
# cuda: CUDA API lib
# ptxsim: cuda-sim, functional simulator
# gpgpusim: gpu simulator (gpgpu-sim)
# intersim: interconnect simulator
# accelwattch: power simulator
# Rest of source files in src/ will be created with gpgpusim_entrypoint target
target_link_libraries(cudart PUBLIC cuda ptxsim gpgpusim intersim)
if(GPGPUSIM_USE_POWER_MODEL)
target_link_libraries(cudart PUBLIC accelwattch)
endif()

# TODO Need to create symlinks for different cuda version

# TODO Conditionally build for Opencl?
# if(GPGPUSIM_USE_OPENCL)
# add_library(OpenCL)
# endif()