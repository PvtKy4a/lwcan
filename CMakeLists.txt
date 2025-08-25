cmake_minimum_required(VERSION 3.22)

add_library(lwcan)

target_sources(lwcan PRIVATE
    src/buffer.c
    src/canif.c
    src/init.c
    src/isotp_in.c
    src/isotp_out.c
    src/isotp.c
    src/length.c
    src/memory.c
    src/raw.c
    src/timeouts.c)

target_include_directories(lwcan PRIVATE ${LWCAN_INCLUDE_DIRS})
target_compile_options(lwcan PRIVATE ${LWCAN_COMPILER_FLAGS})
target_compile_definitions(lwcan PRIVATE ${LWCAN_DEFINITIONS})

add_library(lwcanapps)

target_sources(lwcanapps PRIVATE
    src/apps/uds/uds_client.c)

target_include_directories(lwcanapps PRIVATE ${LWCAN_INCLUDE_DIRS})
target_compile_options(lwcanapps PRIVATE ${LWCAN_COMPILER_FLAGS})
target_compile_definitions(lwcanapps PRIVATE ${LWCAN_DEFINITIONS})