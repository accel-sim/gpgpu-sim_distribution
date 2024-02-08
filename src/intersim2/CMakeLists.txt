option(GPGPUSIM_INTERSIM_STANDALONE "Whether to also build intersim in standalone mode" OFF)

# Specify Flex and Bison target
BISON_TARGET(intersim_config_parser config.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c
            COMPILE_FLAGS "-y -d --file-prefix=${CMAKE_CURRENT_BINARY_DIR}/y")
FLEX_TARGET(intersim_config_lexer config.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
ADD_FLEX_BISON_DEPENDENCY(intersim_config_lexer intersim_config_parser)

# Create booksim or libintersim.a
# Shared include path
list(APPEND intersim_INC ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/allocators
    ${CMAKE_CURRENT_SOURCE_DIR}/arbiters
    ${CMAKE_CURRENT_SOURCE_DIR}/networks
    ${CMAKE_CURRENT_SOURCE_DIR}/power
    ${CMAKE_CURRENT_SOURCE_DIR}/routers
    ${PROJECT_SOURCE_DIR}/src)

# Shared source files
file(GLOB_RECURSE intersim_SRC "*.cc" "*.c" "*.cpp")
list(REMOVE_ITEM ${intersim_SRC} interconnect_interface.cpp)

# If standalone, also build for it
if(GPGPUSIM_INTERSIM_STANDALONE)
    add_executable(booksim ${intersim_SRC})
    target_include_directories(booksim PUBLIC 
        ${intersim_INC})
    target_include_directories(booksim PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(booksim PUBLIC ${CUDAToolkit_INCLUDE_DIRS})
    # Remove globally set TRACING_ON flag
    target_compile_options(booksim PRIVATE -UTRACING_ON)
endif()

# Specify sources for libintersim.a
add_library(intersim STATIC 
    ${intersim_SRC} interconnect_interface.cpp
    ${BISON_intersim_config_parser_OUTPUTS}
    ${FLEX_intersim_config_lexer_OUTPUTS})
target_include_directories(intersim PUBLIC 
    ${intersim_INC}
    ${PROJECT_SOURCE_DIR}/src/gpgpu-sim)
target_include_directories(intersim PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(intersim PUBLIC ${CUDAToolkit_INCLUDE_DIRS})
target_compile_definitions(intersim PRIVATE CREATE_LIBRARY)
# Remove globally set TRACING_ON flag
target_compile_options(intersim PRIVATE -UTRACING_ON)
