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

# TODO Compilation flags

# Dependency checking
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(Doxygen)
find_package(Python3)

# GPGPU-Sim additional checking and info
message(CHECK_START "Additional settings for ${CMAKE_PROJECT_NAME}")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

# Check for OS
message(CHECK_START "Checking for OS")
if((NOT APPLE) AND (NOT UNIX) AND (NOT LINUX))
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
else()
    message(CHECK_PASS ${CMAKE_SYSTEM_NAME})
endif()

# Check for version
message(CHECK_START "Checking GPGPU-Sim version")
message(CHECK_PASS "${CMAKE_PROJECT_VERSION}")

# Check for git commit hash
message(CHECK_START "Checking git commit hash")
# Get the latest abbreviated commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GPGPUSIM_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GPGPUSIM_CHECK_GIT_HASH
)
if(${GPGPUSIM_CHECK_GIT_HASH})
    message(CHECK_FAIL "not a git repo")
else()
    message(CHECK_PASS "${GPGPUSIM_GIT_HASH}")
endif()

# Check for compiler and version
message(CHECK_START "Checking CXX compiler")
if(NOT (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU))
    message(CHECK_FAIL "GPGPU-Sim only tested with GCC: ${CMAKE_CXX_COMPILER_ID}")
else()
    message(CHECK_PASS "${CMAKE_CXX_COMPILER}")
endif()
message(CHECK_START "Checking CXX compiler version")
message(CHECK_PASS "${CMAKE_CXX_COMPILER_VERSION}")
set(GPGPSIM_CC_VERSION )

# Check for CUDA nvcc and version 
# Check already done with find_package, here just to display the path and version
message(CHECK_START "Checking CUDA compiler")
if(NOT DEFINED ${CMAKE_CUDA_COMPILER})
    message(CHECK_FAIL "not found")
else()
    message(CHECK_PASS "${CMAKE_CUDA_COMPILER}")
    message(CHECK_START "Checking CUDA compiler version")
    message(CHECK_PASS "${CMAKE_CUDA_COMPILER_VERSION}")
    if((CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 2.0.3) OR (CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER 11.1.0))
        message(FATAL_ERROR "GPGPU-Sim ${CMAKE_PROJECT_VERSION} not tested with CUDA version ${CMAKE_CUDA_COMPILER_VERSION} (please see README)")
    endif()
endif()

# Check for Power model
# TODO How to configure the project to look for it?
message(CHECK_START "Checking for GPGPU-Sim power model")
if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/src/accelwattch)
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/src/accelwattch/gpgpu_sim.verify)
        message(FATAL_ERROR "gpgpu_sim.verify not found in ${PROJECT_SOURCE_DIR}/src/accelwattch/")
    endif()
    message(CHECK_PASS "${PROJECT_SOURCE_DIR}/src/accelwattch/")
elseif(DEFINED ${GPGPUSIM_POWER_MODEL})
    if(NOT EXISTS ${GPGPUSIM_POWER_MODEL}/gpgpu_sim.verify)
        message(FATAL_ERROR "gpgpu_sim.verify not found in ${GPGPUSIM_POWER_MODEL} - Either incorrect directory or incorrect McPAT version")
    endif()
    message(CHECK_PASS "${GPGPUSIM_POWER_MODEL}")
else()
    message(CHECK_PASS "configured without a power model")
endif()

# Set Build path
# Get CUDA version
execute_process(
    COMMAND "${CMAKE_CUDA_COMPILER} --version | awk '/release/ {print $5;}' | sed 's/,//'`"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE CUDA_VERSION_STRING
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND "echo ${CUDA_VERSION_STRING} | sed 's/\./ /' | awk '{printf(\"%02u%02u\", 10*int($1), 10*$2);}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE CUDA_VERSION_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get debug or release
# Set with -DCMAKE_BUILD_TYPE=Debug|Release to change build type
if(NOT CMAKE_BUILD_TYPE)
    set(GPGPUSIM_BUILD_MODE "release" CACHE STRING "" FORCE)
else()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" GPGPUSIM_BUILD_MODE)
endif()
set(GPGPUSIM_CONFIG "gcc-${CMAKE_CXX_COMPILER_VERSION}/cuda-${CUDA_VERSION_NUMBER}/${GPGPUSIM_BUILD_MODE}")
set(CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}/${GPGPUSIM_CONFIG}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
message(STATUS "Setting binary directory to ${CMAKE_BINARY_DIR}")

list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")
message(STATUS "Be sure to run 'source setup_environment' "
               "before you run CUDA program with GPGPU-Sim or building with external"
               "simulator like SST")

# TODO Need to create a setup script to set some variables for others to interact with
set(SETUP_SCRIPT_FILENAME "setup")
message(STATUS "Writing setup commands to '${SETUP_SCRIPT_FILENAME}'")
file(WRITE ${SETUP_SCRIPT_FILENAME} "export GPGPUSIM_SETUP_ENVIRONMENT_WAS_RUN=1\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export GPGPUSIM_ROOT=${PROJECT_SOURCE_DIR}\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export GPGPUSIM_CONFIG=${GPGPUSIM_CONFIG}\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export CUDA_INSTALL_PATH=${CUDAToolkit_TARGET_DIR}\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export PATH=`echo $PATH | sed 's#$GPGPUSIM_ROOT/bin:$CUDA_INSTALL_PATH/bin:##'`\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export PATH=$GPGPUSIM_ROOT/bin:$CUDA_INSTALL_PATH/bin:$PATH\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export CUDA_VERSION_NUMBER=${CUDA_VERSION_NUMBER}\n")
if(CUDA_VERSION_NUMBER GREATER_EQUAL 6000)
    file(APPEND ${SETUP_SCRIPT_FILENAME} "export PTX_SIM_USE_PTX_FILE=1.ptx\n")
    file(APPEND ${SETUP_SCRIPT_FILENAME} "export PTX_SIM_KERNELFILE=_1.ptx\n")
    file(APPEND ${SETUP_SCRIPT_FILENAME} "export CUOBJDUMP_SIM_FILE=jj\n")
endif()
# TODO What about OpenCL support?

# setting LD_LIBRARY_PATH as follows enables GPGPU-Sim to be invoked by 
# native CUDA and OpenCL applications. GPGPU-Sim is dynamically linked
# against instead of the CUDA toolkit.  This replaces this cumbersome
# static link setup in prior GPGPU-Sim releases.
if(APPLE)
file(APPEND ${SETUP_SCRIPT_FILENAME} "export DYLD_LIBRARY_PATH=`echo $DYLD_LIBRARY_PATH | sed -Ee 's#'$GPGPUSIM_ROOT'\/lib\/[0-9]+\/(debug|release):##'`")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export DYLD_LIBRARY_PATH=$GPGPUSIM_ROOT/lib/$GPGPUSIM_CONFIG:$DYLD_LIBRARY_PATH")
else()
file(APPEND ${SETUP_SCRIPT_FILENAME} "export LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH | sed -re 's#'$GPGPUSIM_ROOT'\/lib\/[0-9]+\/(debug|release):##'`")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export LD_LIBRARY_PATH=$GPGPUSIM_ROOT/lib/$GPGPUSIM_CONFIG:$LD_LIBRARY_PATH")
endif()

# TODO ignore the OPENCL_REMOTE_GPU_HOST part?