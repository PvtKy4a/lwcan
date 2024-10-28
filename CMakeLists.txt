cmake_minimum_required(VERSION 3.22)

add_library(lwcan)

target_sources(lwcan PRIVATE
    src/buffer.c
    src/canif.c
    src/init.c
    src/isotp_in.c
    src/isotp_out.c
    src/isotp.c
    src/memory.c
    src/raw.c
    src/timeouts.c
)

target_include_directories(lwcan PRIVATE ${LWCAN_INCLUDE_DIRS})

target_compile_options(lwcan PRIVATE ${LWCAN_COMPILER_FLAGS})

target_compile_definitions(lwcan PRIVATE ${LWCAN_DEFINITIONS})
