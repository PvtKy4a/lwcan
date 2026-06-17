/**
 * @file test_length.c
 * @brief Unit tests for length module
 */

#include "unity.h"

#include "lwcan/length.h"
#include "lwcan/error.h"

#include <stdint.h>
#include <stdbool.h>

/* Test setup and teardown */
void setUp(void)
{
}

void tearDown(void)
{
}

/* Test CAN DLC to length conversion */
void test_can_dlc_to_length(void)
{
    uint8_t dlc;
    uint8_t length;

    /* Standard DLC values */
    dlc = 0;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(0, length);

    dlc = 1;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(1, length);

    dlc = 2;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(2, length);

    dlc = 3;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(3, length);

    dlc = 4;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(4, length);

    dlc = 5;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(5, length);

    dlc = 6;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(6, length);

    dlc = 7;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(7, length);

    dlc = 8;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(8, length);

    /* CAN FD DLC values */
    dlc = 9;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(12, length);

    dlc = 10;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(16, length);

    dlc = 11;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(20, length);

    dlc = 12;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(24, length);

    dlc = 13;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(32, length);

    dlc = 14;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(48, length);

    dlc = 15;
    length = can_fd_dlc2len(dlc);
    TEST_ASSERT_EQUAL(64, length);
}

/* Test length to CAN DLC conversion */
void test_length_to_can_dlc(void)
{
    uint8_t dlc;
    uint8_t length;

    /* Standard lengths */
    length = 0;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(0, dlc);

    length = 1;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(1, dlc);

    length = 2;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(2, dlc);

    length = 3;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(3, dlc);

    length = 4;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(4, dlc);

    length = 5;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(5, dlc);

    length = 6;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(6, dlc);

    length = 7;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(7, dlc);

    length = 8;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(8, dlc);

    /* Boundary cases */
    length = 9;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(9, dlc);

    length = 12;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(9, dlc);

    length = 13;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(10, dlc);

    length = 16;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(10, dlc);

    length = 17;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(11, dlc);

    length = 20;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(11, dlc);

    length = 21;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(12, dlc);

    length = 24;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(12, dlc);

    length = 25;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(13, dlc);

    length = 32;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(13, dlc);

    length = 33;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(14, dlc);

    length = 40;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(14, dlc);

    length = 41;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(14, dlc);

    length = 48;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(14, dlc);

    length = 49;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(15, dlc);

    length = 56;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(15, dlc);

    length = 57;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(15, dlc);

    length = 64;
    dlc = can_fd_len2dlc(length);
    TEST_ASSERT_EQUAL(15, dlc);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_can_dlc_to_length);

    RUN_TEST(test_length_to_can_dlc);

    return UNITY_END();
}
