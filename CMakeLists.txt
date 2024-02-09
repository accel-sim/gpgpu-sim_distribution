cmake_minimum_required(VERSION 3.17)

# Project name and version
project(GPGPU-Sim 
        VERSION 4.2.0 
        DESCRIPTION "cycle-level simulator modeling contemporary graphics processing units (GPUs)" 
        HOMEPAGE_URL https://github.com/accel-sim/gpgpu-sim_distribution
        LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# GPGPU-Sim build option
option(GPGPUSIM_ENABLE_TRACE "Whether to enable GPGPU-Sim debug tracing" ON)

# GPGPU-Sim conditional build variable
set(GPGPUSIM_USE_POWER_MODEL OFF)
set(GPGPUSIM_USE_OPENCL OFF)

include(gpgpusim_check.cmake)
include(gen_setup_environment.cmake)

# Create version file
# TODO Get the correct git diff info on every build
set(GPGPUSIM_BUILD_STRING "gpgpu-sim_git-commit-${GPGPUSIM_GIT_HASH}_modified_TODO_FILL_INFO")
configure_file(version.in ${CMAKE_CURRENT_BINARY_DIR}/detailed_version)

# CMake target
# GPGPU-Sim CUDA Runtime lib
# Use the entrypoint object files sources else CMake will complain
add_library(cudart SHARED $<TARGET_OBJECTS:gpgpusim_entrypoint>)
set_target_properties(cudart PROPERTIES 
                      SOVERSION ${CUDAToolkit_VERSION_MAJOR}.${CUDAToolkit_VERSION_MINOR})

# Add global C/CXX compilation flags and definitions
# TODO Specify more build modes like gem5 with fast opt?
if($<CONFIG:Debug>)
    add_compile_definitions(DEBUG=1)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wno-unused-function;-Wno-sign-compare;-g;-fPIC>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wno-unused-function;-Wno-sign-compare;-ggdb;-fPIC>")
else()
    add_compile_definitions(DEBUG=0)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-O3;-g;-Wall;-Wno-unused-function;-Wno-sign-compare;-fPIC>")
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wall;-Wno-unused-function;-Wno-sign-compare;-fPIC>")
endif()

# Add CUDA version
add_compile_definitions(CUDART_VERSION=${CUDA_VERSION_NUMBER})

# OpenCL support
if(GPGPUSIM_USE_OPENCL)
    add_compile_definitions(OPENGL_SUPPORT)
endif()

# Tracing support
if(GPGPUSIM_ENABLE_TRACE)
    add_compile_definitions(TRACING_ON=1)
endif()

# Add subdirectory
add_subdirectory(src)
add_subdirectory(libcuda)
add_subdirectory(libopencl)

# Set linker option for libcudart.so
message($<$<PLATFORM_ID:UNIX>:-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-so-version.txt)
target_link_options(cudart 
                    PUBLIC $<$<PLATFORM_ID:UNIX>:-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux-so-version.txt>
                    PUBLIC $<$<PLATFORM_ID:UNIX>:-lm -lz -lGL -lpthread>
                    PUBLIC $<$<PLATFORM_ID:APPLE>:-Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-compatibility_version,1.1,-current_version,1.1>
                    PUBLIC $<$<PLATFORM_ID:APPLE>:-lm -lz -lpthread>)
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