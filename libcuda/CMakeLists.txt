# TODO Flex/bison/c/cxx flags
# Specify Flex and Bison target
BISON_TARGET(cuobjdump_parser cuobjdump.y ${CMAKE_CURRENT_BINARY_DIR}/cuobjdump_parser.c
            COMPILE_FLAGS "-t -d -v --report=all -p cuobjdump_ --file-prefix=${CMAKE_CURRENT_BINARY_DIR}/cuobjdump")
FLEX_TARGET(cuobjdump_lexer cuobjdump.l ${CMAKE_CURRENT_BINARY_DIR}/cuobjdump_lexer.c
            COMPILE_FLAGS "-B -P cuobjdump_")
ADD_FLEX_BISON_DEPENDENCY(cuobjdump_lexer cuobjdump_parser)

# Create libcuda.a with all source files
file(GLOB cuda_SRC "*.cc" "*.c")
add_library(cuda STATIC ${cuda_SRC} ${BISON_cuobjdump_parser_OUTPUTS} ${FLEX_cuobjdump_lexer_OUTPUTS})
target_link_libraries(cuda ${FLEX_LIBRARIES})
# Add current dir to include path
# Also add flex/bison generated header files
target_include_directories(cuda PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# Add cuda include path for own reference
target_include_directories(cuda PRIVATE ${CUDAToolkit_INCLUDE_DIRS})

# Add project build dir to include path
target_include_directories(cuda PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Set definitions
target_compile_definitions(cuda PRIVATE CUDART_VERSION=${CUDA_VERSION_NUMBER})
if(GPGPUSIM_USE_OPENCL)
target_compile_definitions(cuda PRIVATE OPENGL_SUPPORT)
endif()

# Set compiler flags based on whether processing C/CXX files or whether debug is enable
if($<CONFIG:Debug>)
    target_compile_options(cuda PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wno-unused-function -Wno-sign-compare -g -fPIC>)
    target_compile_options(cuda PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wno-unused-function -Wno-sign-compare -ggdb -fPIC>)
else()
    target_compile_options(cuda PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-O3 -g -Wall -Wno-unused-function -Wno-sign-compare -fPIC>)
    target_compile_options(cuda PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wno-unused-function -Wno-sign-compare -fPIC>)
endif()