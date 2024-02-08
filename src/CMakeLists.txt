file(GLOB gpgpusim_entrypoint_SRC "*.cc" "*.c")
add_library(gpgpusim_entrypoint OBJECT ${gpgpusim_entrypoint_SRC})

# Add current folder and CUDA include to include path
target_include_directories(gpgpusim_entrypoint PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(gpgpusim_entrypoint PRIVATE ${CUDAToolkit_INCLUDE_DIRS})