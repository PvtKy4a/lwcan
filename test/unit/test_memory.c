/**
 * @file test_memory.c
 * @brief Unit tests for memory module
 */

#include "unity.h"

#include "lwcan/memory.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/init.h"
#include "lwcanarch/system_arch.h"

#include <string.h>
#include <stdio.h>

/* Test setup and teardown */
void setUp(void)
{
    lwcan_init();

    mock_system_time_set(0);
}

void tearDown(void)
{
}

/* Test basic allocation */
void test_memory_basic_allocation(void)
{
    void *ptr;

    /* Test successful allocation */
    ptr = lwcan_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr);
    lwcan_free(ptr);

    /* Test that allocated memory is zero-initialized by calloc */
    ptr = lwcan_calloc(1, 64);
    TEST_ASSERT_NOT_NULL(ptr);

    uint8_t *data = (uint8_t *)ptr;
    for (int i = 0; i < 64; i++)
    {
        TEST_ASSERT_EQUAL(0, data[i]);
    }

    lwcan_free(ptr);

    /* Test allocation of exactly available memory */
    ptr = lwcan_malloc(LWCAN_MEM_SIZE);
    TEST_ASSERT_NULL(ptr); /* Should fail due to heap structure overhead */
}

/* Test multiple allocations */
void test_memory_multiple_allocations(void)
{
    void *ptrs[10];
    int i;

    /* Allocate multiple small blocks */
    for (i = 0; i < 10; i++)
    {
        ptrs[i] = lwcan_malloc(32);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Free all and verify */
    for (i = 0; i < 10; i++)
    {
        lwcan_free(ptrs[i]);
        ptrs[i] = NULL;
    }

    /* Allocate again to verify reuse */
    for (i = 0; i < 10; i++)
    {
        ptrs[i] = lwcan_malloc(32);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
        lwcan_free(ptrs[i]);
    }
}

/* Test free and reuse */
void test_memory_free_and_reuse(void)
{
    void *ptr1, *ptr2, *ptr3;

    /* Allocate and free */
    ptr1 = lwcan_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr1);
    lwcan_free(ptr1);

    /* Allocate again - should reuse freed block */
    ptr2 = lwcan_malloc(64);
    TEST_ASSERT_NOT_NULL(ptr2);

    /* Free and allocate larger block */
    lwcan_free(ptr2);

    ptr3 = lwcan_malloc(128);
    TEST_ASSERT_NOT_NULL(ptr3);
    lwcan_free(ptr3);
}

/* Test fragmentation */
void test_memory_fragmentation(void)
{
    void *ptrs[10];
    int i;

    /* Create fragmentation by allocating and freeing alternately */
    for (i = 0; i < 10; i++)
    {
        ptrs[i] = lwcan_malloc(32);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }

    /* Free every other block */
    for (i = 0; i < 10; i += 2)
    {
        lwcan_free(ptrs[i]);
        ptrs[i] = NULL;
    }

    /* Try to allocate blocks that fit in freed spaces */
    for (i = 0; i < 10; i += 2)
    {
        ptrs[i] = lwcan_malloc(32);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
        lwcan_free(ptrs[i]);
    }

    /* Cleanup */
    for (i = 0; i < 10; i++)
    {
        lwcan_free(ptrs[i]);
    }
}

/* Test edge cases */
void test_memory_edge_cases(void)
{
    void *ptr;

    /* Allocate zero bytes */
    ptr = lwcan_malloc(0);
    TEST_ASSERT_NULL(ptr);

    /* Allocate very large size */
    ptr = lwcan_malloc(1024 * 1024); /* 1MB */
    TEST_ASSERT_NULL(ptr);

    /* Free NULL pointer (should be safe) */
    lwcan_free(NULL); /* Should not crash */

    /* Allocate and free repeatedly */
    for (int i = 0; i < 100; i++)
    {
        ptr = lwcan_malloc(8);
        if (ptr != NULL)
        {
            lwcan_free(ptr);
        }
    }
}

/* Test alignment */
void test_memory_alignment(void)
{
    void *ptr;
    uintptr_t addr;

    /* Allocate multiple times and check alignment */
    for (int i = 0; i < 10; i++)
    {
        ptr = lwcan_malloc(8);
        TEST_ASSERT_NOT_NULL(ptr);

        addr = (uintptr_t)ptr;
        TEST_ASSERT_EQUAL(0, addr & 0x7); /* 8-byte aligned */
        lwcan_free(ptr);
    }
}

/* Test memory pool exhaustion */
void test_memory_pool_exhaustion(void)
{
    void *ptrs[100];
    int count = 0;

    /* Try to allocate until failure */
    for (int i = 0; i < 100; i++)
    {
        ptrs[i] = lwcan_malloc(32);
        if (ptrs[i] != NULL)
        {
            count++;
        }
        else
        {
            break;
        }
    }

    TEST_ASSERT_GREATER_THAN(0, count);

    /* Free all allocated blocks */
    for (int i = 0; i < count; i++)
    {
        lwcan_free(ptrs[i]);
    }
}

/* Test calloc functionality */
void test_calloc_initialization(void)
{
    void *ptr;
    uint8_t *data;
    int i;

    ptr = lwcan_calloc(10, 8); /* 80 bytes */
    TEST_ASSERT_NOT_NULL(ptr);

    data = (uint8_t *)ptr;

    /* Verify all bytes are zero */
    for (i = 0; i < 80; i++)
    {
        TEST_ASSERT_EQUAL(0, data[i]);
    }

    /* Write pattern and verify */
    for (i = 0; i < 80; i++)
    {
        data[i] = (uint8_t)(i & 0xFF);
    }

    /* Free and reallocate - should be zero again */
    lwcan_free(ptr);

    ptr = lwcan_calloc(1, 80);
    TEST_ASSERT_NOT_NULL(ptr);

    data = (uint8_t *)ptr;
    for (i = 0; i < 80; i++)
    {
        TEST_ASSERT_EQUAL(0, data[i]);
    }

    lwcan_free(ptr);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_memory_basic_allocation);

    RUN_TEST(test_memory_multiple_allocations);

    RUN_TEST(test_memory_free_and_reuse);

    RUN_TEST(test_memory_fragmentation);

    RUN_TEST(test_memory_edge_cases);

    RUN_TEST(test_memory_alignment);

    RUN_TEST(test_memory_pool_exhaustion);

    RUN_TEST(test_calloc_initialization);

    return UNITY_END();
}
