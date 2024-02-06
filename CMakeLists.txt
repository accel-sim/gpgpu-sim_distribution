cmake_minimum_required(VERSION 3.17)

project(GPGPU-Sim VERSION 4.2.0 LANGUAGES CXX)

# Dependency checking
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(Doxygen)
find_package(Python3)


# Env checking
message(CHECK_START "Additional settings for ${CMAKE_PROJECT_NAME}")
list(APPEND CMAKE_MESSAGE_INDENT "  ")

# Check for OS
message(CHECK_START "Checking for OS")
if(NOT APPLE AND NOT LINUX)
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
else()
    message(CHECK_PASS ${CMAKE_SYSTEM_NAME})
endif()

# Export CUDA_INSTALL_PATH
# message(CHECK_START "Checking for $CUDA_INSTALL_PATH")
# if(NOT DEFINED ENV{CUDA_INSTALL_PATH})
#     message(FATAL_ERROR "Install CUDA Toolkit and set CUDA_INSTALL_PATH.")
# elseif(NOT IS_ABSOLUTE $ENV{CUDA_INSTALL_PATH})
#     message(FATAL_ERROR "CUDA_INSTALL_PATH not an absolute path: $ENV{CUDA_INSTALL_PATH}")
# elseif(NOT IS_DIRECTORY $ENV{CUDA_INSTALL_PATH})
#     message(FATAL_ERROR "CUDA_INSTALL_PATH not a directory: $ENV{CUDA_INSTALL_PATH}")
# else()
#     message(CHECK_PASS "found: $ENV{CUDA_INSTALL_PATH}")
# endif()

# Check for NVCC



list(POP_BACK CMAKE_MESSAGE_INDENT)
message(CHECK_PASS "done")

# TODO Need to create a setup script to set some variables for others to interact with
set(SETUP_SCRIPT_FILENAME "setup")
message(STATUS "Writing setup commands to ${SETUP_SCRIPT_FILENAME}")
file(WRITE ${SETUP_SCRIPT_FILENAME} "export GPGPUSIM_SETUP_ENVIRONMENT_WAS_RUN=1\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export GPGPUSIM_ROOT=${PROJECT_SOURCE_DIR}\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export CUDA_INSTALL_PATH=${CUDAToolkit_TARGET_DIR}\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export PATH=`echo $PATH | sed 's#$GPGPUSIM_ROOT/bin:$CUDA_INSTALL_PATH/bin:##'`}\n")
file(APPEND ${SETUP_SCRIPT_FILENAME} "export PATH=$GPGPUSIM_ROOT/bin:$CUDA_INSTALL_PATH/bin:$PATH
\n")

