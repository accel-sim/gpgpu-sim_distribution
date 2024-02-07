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

include(gpgpusim_check.cmake)
include(gen_setup_environment.cmake)