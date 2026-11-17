cmake_minimum_required(VERSION 3.22)

project("lwcan_tests")

include(CTest)

set(LWCAN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(LWCAN_SOURCES
    ${LWCAN_ROOT}/src/buffer.c
    ${LWCAN_ROOT}/src/canif.c
    ${LWCAN_ROOT}/src/init.c
    ${LWCAN_ROOT}/src/isotp_in.c
    ${LWCAN_ROOT}/src/isotp_out.c
    ${LWCAN_ROOT}/src/isotp.c
    ${LWCAN_ROOT}/src/length.c
    ${LWCAN_ROOT}/src/memory.c
    ${LWCAN_ROOT}/src/raw.c
    ${LWCAN_ROOT}/src/timeouts.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mock/system_arch.c)

add_subdirectory(unity)

function(add_unity_test TEST_NAME TEST_SOURCES)
    add_executable(${TEST_NAME} ${LWCAN_SOURCES} ${TEST_SOURCES})

    target_include_directories(${TEST_NAME} PRIVATE
        ${LWCAN_ROOT}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/mock)

    target_link_libraries(${TEST_NAME} PRIVATE unity)

    # Регистрируем в CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_unity_test(test_memory
    unit/test_memory.c)

add_unity_test(test_buffer
    unit/test_buffer.c)

add_unity_test(test_length
    unit/test_length.c)

add_unity_test(test_timeouts
    unit/test_timeouts.c)

add_unity_test(test_canif
    unit/test_canif.c)

add_unity_test(test_raw
    unit/test_raw.c)

add_unity_test(test_isotp
    unit/test_isotp.c)

# Code coverage (optional)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_options(${PROJECT_NAME} PRIVATE --coverage)
endif()
