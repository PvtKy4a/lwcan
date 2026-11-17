/**
 * @file test_timeouts.c
 * @brief Unit tests for timeouts module
 */

#include "unity.h"

#include "lwcan/timeouts.h"
#include "lwcan/private/timeouts_private.h"
#include "lwcan/system.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/init.h"
#include "lwcanarch/system_arch.h"

#include <stdint.h>
#include <string.h>

/* Mock callback variables */
static uint32_t callback_called_count = 0;
static void *callback_arg_value = NULL;

/* Test setup and teardown */
void setUp(void)
{
    lwcan_init();

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
    struct lwcan_timeout timeout;

    /* Register a timeout for 100ms */
    lwcan_timeout(&timeout, 100, mock_timeout_handler, (void *)(uintptr_t)0x12345678);

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
    struct lwcan_timeout timeout1, timeout2, timeout3;

    /* Register multiple timeouts with different delays */
    lwcan_timeout(&timeout1, 50, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout2, 100, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(&timeout3, 150, mock_timeout_handler, (void *)(uintptr_t)0x3);

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
    struct lwcan_timeout timeout1, timeout2, timeout3;

    /* Register timeouts out of order */
    lwcan_timeout(&timeout3, 150, mock_timeout_handler, (void *)(uintptr_t)0x3);
    lwcan_timeout(&timeout1, 50, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout2, 100, mock_timeout_handler, (void *)(uintptr_t)0x2);

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
    struct lwcan_timeout timeout1, timeout2, timeout3;

    /* Register multiple timeouts */
    lwcan_timeout(&timeout1, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout2, 200, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(&timeout3, 300, mock_timeout_handler, (void *)(uintptr_t)0x3);

    /* Remove middle timeout */
    lwcan_untimeout(&timeout2);

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
    struct lwcan_timeout timeout;

    /* Register a timeout */
    lwcan_timeout(&timeout, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    /* Try to remove a different timeout */
    struct lwcan_timeout other;
    lwcan_untimeout(&other);

    /* original timeout should fire */
    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);
}

/* Test removing all timeouts */
void test_timeout_remove_all(void)
{
    struct lwcan_timeout timeout1, timeout2, timeout3;

    /* Register multiple timeouts */
    lwcan_timeout(&timeout1, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout2, 200, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(&timeout3, 300, mock_timeout_handler, (void *)(uintptr_t)0x3);

    /* Remove all timeouts */
    lwcan_untimeout(&timeout1);
    lwcan_untimeout(&timeout2);
    lwcan_untimeout(&timeout3);

    /* no timeouts should fire */
    mock_system_time_set(300);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(0, callback_called_count);
}

/* Test removing with NULL timeout */
void test_timeout_remove_null(void)
{
    /* Should not crash when there are no timeouts */
    lwcan_untimeout(NULL);
}

/* Test timeout with zero delay */
void test_timeout_zero_delay(void)
{
    struct lwcan_timeout timeout;

    /* Register a timeout with zero delay */
    lwcan_timeout(&timeout, 0, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(1);

    /* Should fire immediately */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x1, callback_arg_value);
}

/* Test timeout with NULL handler */
void test_timeout_null_handler(void)
{
    struct lwcan_timeout timeout;

    /* Should not crash with NULL handler */
    lwcan_timeout(&timeout, 100, NULL, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    /* Should not crash when handler is NULL */
    lwcan_timeouts_handler();
}

/* Test timeout with NULL arg */
void test_timeout_null_arg(void)
{
    struct lwcan_timeout timeout;

    /* Register a timeout with NULL arg */
    lwcan_timeout(&timeout, 100, mock_timeout_handler, NULL);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_NULL(callback_arg_value);
}

/* Test timeout with NULL timeout pointer */
void test_timeout_null_pointer(void)
{
    /* Should not crash with NULL timeout pointer */
    lwcan_timeout(NULL, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    /* Should not fire since registration failed */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(0, callback_called_count);
}

/* Test timeout handler with multiple calls */
void test_timeout_multiple_calls(void)
{
    struct lwcan_timeout timeout1, timeout2, timeout3;

    /* Register same timeout multiple times with different structs */
    lwcan_timeout(&timeout1, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout2, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout3, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(3, callback_called_count);
}

/* Test timeout handler ordering with same timeout values */
void test_timeout_same_values(void)
{
    struct lwcan_timeout timeout1, timeout2, timeout3;

    /* Register timeouts with same delay */
    lwcan_timeout(&timeout1, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);
    lwcan_timeout(&timeout2, 100, mock_timeout_handler, (void *)(uintptr_t)0x2);
    lwcan_timeout(&timeout3, 100, mock_timeout_handler, (void *)(uintptr_t)0x3);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(3, callback_called_count);
}

/* Test timeout after handler call */
void test_timeout_after_handler_call(void)
{
    struct lwcan_timeout timeout1, timeout2;

    /* Register a timeout */
    lwcan_timeout(&timeout1, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);

    lwcan_timeout(&timeout2, 50, mock_timeout_handler, (void *)(uintptr_t)0x2);

    mock_system_time_set(150);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(2, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x2, callback_arg_value);
}

/* Test timeout with large delay */
void test_timeout_large_delay(void)
{
    struct lwcan_timeout timeout;

    /* Register a timeout with large delay */
    lwcan_timeout(&timeout, 0x10000, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(1000);

    /* Timeout should not have fired */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(0, callback_called_count);

    lwcan_untimeout(&timeout);
}

/* Test timeout handler is called only once */
void test_timeout_called_once(void)
{
    struct lwcan_timeout timeout;

    /* Register a timeout */
    lwcan_timeout(&timeout, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);

    /* Call handler again - should not be called again */
    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
}

/* Test timeout reuse after firing */
void test_timeout_reuse_after_fire(void)
{
    struct lwcan_timeout timeout;

    /* Register a timeout */
    lwcan_timeout(&timeout, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    mock_system_time_set(100);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);

    /* Reuse the same timeout struct */
    lwcan_timeout(&timeout, 50, mock_timeout_handler, (void *)(uintptr_t)0x2);

    mock_system_time_set(150);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(2, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x2, callback_arg_value);
}

/* Test timeout reuse after removal */
void test_timeout_reuse_after_remove(void)
{
    struct lwcan_timeout timeout;

    /* Register a timeout */
    lwcan_timeout(&timeout, 100, mock_timeout_handler, (void *)(uintptr_t)0x1);

    /* Remove before it fires */
    lwcan_untimeout(&timeout);

    /* Reuse the same timeout struct */
    lwcan_timeout(&timeout, 50, mock_timeout_handler, (void *)(uintptr_t)0x2);

    mock_system_time_set(50);

    lwcan_timeouts_handler();
    TEST_ASSERT_EQUAL(1, callback_called_count);
    TEST_ASSERT_EQUAL((void *)(uintptr_t)0x2, callback_arg_value);
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

    RUN_TEST(test_timeout_remove_null);

    RUN_TEST(test_timeout_zero_delay);

    RUN_TEST(test_timeout_null_handler);

    RUN_TEST(test_timeout_null_arg);

    RUN_TEST(test_timeout_null_pointer);

    RUN_TEST(test_timeout_multiple_calls);

    RUN_TEST(test_timeout_same_values);

    RUN_TEST(test_timeout_after_handler_call);

    RUN_TEST(test_timeout_large_delay);

    RUN_TEST(test_timeout_called_once);

    RUN_TEST(test_timeout_reuse_after_fire);

    RUN_TEST(test_timeout_reuse_after_remove);

    return UNITY_END();
}
