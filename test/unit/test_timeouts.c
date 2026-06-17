/**
 * @file test_timeouts.c
 * @brief Unit tests for timeouts module
 */

#include "unity.h"

#include "lwcan/timeouts.h"
#include "lwcan/system.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/private/timeouts_private.h"
#include "lwcanarch/arch.h"

#include <stdint.h>
#include <string.h>

/* Mock callback variables */
static uint32_t callback_called_count = 0;
static void *callback_arg_value = NULL;

/* Test setup and teardown */
void setUp(void)
{
    lwcan_timeouts_init();

    mock_system_time_set(0);

    callback_called_count = 0;
}

void tearDown(void)
{
}

/* Mock callback functions */
static void mock_timeout_handler(void *arg)
{
    callback_called_count += 1;
    callback_arg_value = arg;
}

/* Test timeout creation and execution */
void test_timeout_basic(void)
{
    /* Register a timeout for 100ms */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x12345678);

    /* timeout should not fire */
    mock_system_time_set(99);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(0, callback_called_count);

    /* timeout should fire */
    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x12345678, callback_arg_value);
}

/* Test multiple timeouts */
void test_multiple_timeouts(void)
{
    /* Register multiple timeouts with different delays */
    lwcan_timeout(50, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(150, mock_timeout_handler, (void *)(uintptr_t)0x3);

    /* first timeout should fire */
    mock_system_time_set(50);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);

    /* second timeout should fire */
    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(2, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x2, callback_arg_value);

    /* third timeout should fire */
    mock_system_time_set(150);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(3, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x3, callback_arg_value);
}

/* Test timeout ordering */
void test_timeout_ordering(void)
{
    /* Register timeouts out of order */
    lwcan_timeout(150, mock_timeout_handler, (void *)(uintptr_t)0x3);
    lwcan_timeout(50, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x2);

    /* first timeout should fire */
    mock_system_time_set(50);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);

    /* second timeout should fire */
    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(2, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x2, callback_arg_value);

    /* third timeout should fire */
    mock_system_time_set(150);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(3, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x3, callback_arg_value);
}

/* Test timeout removal */
void test_timeout_remove(void)
{
    /* Register multiple timeouts */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(200, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(300, mock_timeout_handler, (void *)(uintptr_t)0x3);

    /* Remove middle timeout */
    lwcan_untimeout(mock_timeout_handler, (void *)(uintptr_t)0x2);

    /* first timeout should fire */
    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count); /* Only first timeout should fire */
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);

    /* third timeout should fire */
    mock_system_time_set(300);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(2, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x3, callback_arg_value);
}

/* Test removing non-existent timeout */
void test_timeout_remove_nonexistent(void)
{
    /* Register a timeout */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    /* Try to remove a different timeout */
    lwcan_untimeout(mock_timeout_handler, (void *)(uintptr_t)0x999);

    /* original timeout should fire */
    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);
}

/* Test removing all timeouts */
void test_timeout_remove_all(void)
{
    /* Register multiple timeouts */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(200, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(300, mock_timeout_handler, (void *)(uintptr_t)0x3);

    /* Remove all timeouts */
    lwcan_untimeout(mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_untimeout(mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_untimeout(mock_timeout_handler, (void *)(uintptr_t)0x3);

    /* no timeouts should fire */
    mock_system_time_set(300);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(0, callback_called_count);
}

/* Test removing with NULL handler */
void test_timeout_remove_null_handler(void)
{
    /* Should not crash when there are no timeouts */
    lwcan_untimeout(NULL, NULL);
}

/* Test timeout with zero delay */
void test_timeout_zero_delay(void)
{
    /* Register a timeout with zero delay */
    lwcan_timeout(0, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(1);

    /* Should fire immediately */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);
}

/* Test timeout with NULL handler */
void test_timeout_null_handler(void)
{
    /* Should not crash with NULL handler */
    lwcan_timeout(100, NULL, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    /* Should not crash when handler is NULL */
    lwcan_timeouts_handler();
}

/* Test timeout with NULL arg */
void test_timeout_null_arg(void)
{
    /* Register a timeout with NULL arg */
    lwcan_timeout(100, mock_timeout_handler, NULL);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_NULL(callback_arg_value);
}

/* Test timeout exhaustion */
void test_timeout_exhaustion(void)
{
    /* Allocate all available timeouts */
    for (uint8_t i = 0; i < LWCAN_TIMEOUTS_NUM; i++)
    {
        lwcan_timeout(1000, mock_timeout_handler, (void *)(uintptr_t)(i + 1));
    }

    /* Try to allocate one more - should fail gracefully */
    lwcan_timeout(1000, mock_timeout_handler, (void *)(uintptr_t)0x999);

    mock_system_time_set(1000);

    lwcan_timeouts_handler();
    /* All valid timeouts should have fired */
    TEST_ASSERT_EQUAL(10, callback_called_count);
}

/* Test timeout handler with multiple calls */
void test_timeout_multiple_calls(void)
{
    /* Register same timeout multiple times */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(3, callback_called_count);
}

/* Test timeout handler ordering with same timeout values */
void test_timeout_same_values(void)
{
    /* Register timeouts with same delay */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x3);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(3, callback_called_count);
}

/* Test timeout after handler call */
void test_timeout_after_handler_call(void)
{
    /* Register a timeout */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);

    lwcan_timeout(50, mock_timeout_handler, (void *)(uintptr_t)0x2);

    mock_system_time_set(150);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(2, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x2, callback_arg_value);
}

/* Test timeout with large delay */
void test_timeout_large_delay(void)
{
    /* Register a timeout with large delay */
    lwcan_timeout(0x10000, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(1000);

    /* Timeout should not have fired */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(0, callback_called_count);

    lwcan_untimeout(mock_timeout_handler, (void *)(uintptr_t)0x1);
}

/* Test timeout handler is called only once */
void test_timeout_called_once(void)
{
    /* Register a timeout */
    lwcan_timeout(100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);

    /* Call handler again - should not be called again */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_timeout_basic);

    RUN_TEST(test_multiple_timeouts);

    RUN_TEST(test_timeout_ordering);

    RUN_TEST(test_timeout_remove);

    RUN_TEST(test_timeout_remove_nonexistent);

    RUN_TEST(test_timeout_remove_all);

    RUN_TEST(test_timeout_remove_null_handler);

    RUN_TEST(test_timeout_zero_delay);

    RUN_TEST(test_timeout_null_handler);

    RUN_TEST(test_timeout_null_arg);

    RUN_TEST(test_timeout_exhaustion);

    RUN_TEST(test_timeout_multiple_calls);

    RUN_TEST(test_timeout_same_values);

    RUN_TEST(test_timeout_after_handler_call);

    RUN_TEST(test_timeout_large_delay);

    RUN_TEST(test_timeout_called_once);

    return UNITY_END();
}
