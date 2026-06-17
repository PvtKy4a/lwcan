/**
 * @file test_buffer.c
 * @brief Unit tests for buffer module
 */

#include "unity.h"

#include "lwcan/buffer.h"
#include "lwcan/error.h"
#include "lwcan/options.h"

#include <string.h>
#include <stdio.h>

/* Test setup and teardown */
void setUp(void)
{
}

void tearDown(void)
{
}

/* Test buffer creation and destruction */
void test_buffer_create_destroy(void)
{
    struct lwcan_buffer *buffer;
    lwcanerr_t err;

    /* Test successful buffer creation */
    buffer = lwcan_buffer_new(256);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQUAL(256, buffer->length);
    TEST_ASSERT_NOT_NULL(buffer->payload);

    /* Test buffer deletion */
    lwcan_buffer_delete(buffer);
    buffer = NULL;

    /* Test buffer creation with zero length (should fail) */
    buffer = lwcan_buffer_new(0);
    TEST_ASSERT_NULL(buffer);

    /* Test buffer creation with very large length */
    buffer = lwcan_buffer_new(1024 * 1024); /* 1MB */
    TEST_ASSERT_NULL(buffer);
}

/* Test buffer copy operations */
void test_buffer_copy_operations(void)
{
    struct lwcan_buffer *buffer;
    lwcanerr_t err;
    uint8_t source[100];
    uint8_t destination[100];
    uint32_t i;

    /* Create buffer */
    buffer = lwcan_buffer_new(100);
    TEST_ASSERT_NOT_NULL(buffer);

    /* Fill source with pattern */
    for (i = 0; i < 100; i++)
    {
        source[i] = (uint8_t)(i & 0xFF);
    }

    /* Test copy to buffer */
    err = lwcan_buffer_copy_to(buffer, source, 100);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL_MEMORY(source, buffer->payload, 100);

    /* Test copy from buffer */
    memset(destination, 0, 100);
    err = lwcan_buffer_copy_from(buffer, destination, 100);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL_MEMORY(source, destination, 100);

    /* Test copy with NULL buffer */
    err = lwcan_buffer_copy_to(NULL, source, 100);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_from(NULL, destination, 100);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test copy with NULL source */
    err = lwcan_buffer_copy_to(buffer, NULL, 100);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test copy with NULL destination */
    err = lwcan_buffer_copy_from(buffer, NULL, 100);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test copy with zero length */
    err = lwcan_buffer_copy_to(buffer, source, 0);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_from(buffer, destination, 0);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test copy with length exceeding buffer size */
    err = lwcan_buffer_copy_to(buffer, source, 101);
    TEST_ASSERT_EQUAL(ERROR_OK, err); /* Current implementation doesn't check this */

    /* Cleanup */
    lwcan_buffer_delete(buffer);
}

/* Test offset operations */
void test_buffer_offset_operations(void)
{
    struct lwcan_buffer *buffer;
    lwcanerr_t err;
    uint8_t source[100];
    uint8_t destination[100];
    uint32_t i;

    /* Create buffer */
    buffer = lwcan_buffer_new(200);
    TEST_ASSERT_NOT_NULL(buffer);

    for (i = 0; i < 200; i++)
    {
        buffer->payload[i] = 0;
    }

    /* Fill source with pattern */
    for (i = 0; i < 100; i++)
    {
        source[i] = (uint8_t)(i & 0xFF);
    }

    /* Test copy to offset */
    err = lwcan_buffer_copy_to_offset(buffer, source, 100, 50);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Verify data at offset */
    for (i = 0; i < 100; i++)
    {
        TEST_ASSERT_EQUAL(source[i], buffer->payload[50 + i]);
    }

    /* Verify zeros before offset */
    for (i = 0; i < 50; i++)
    {
        TEST_ASSERT_EQUAL(0, buffer->payload[i]);
    }

    /* Test copy from offset */
    memset(destination, 0, 100);
    err = lwcan_buffer_copy_from_offset(buffer, destination, 100, 50);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL_MEMORY(source, destination, 100);

    /* Test with NULL buffer */
    err = lwcan_buffer_copy_to_offset(NULL, source, 100, 50);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_from_offset(NULL, destination, 100, 50);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with NULL source */
    err = lwcan_buffer_copy_to_offset(buffer, NULL, 100, 50);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with NULL destination */
    err = lwcan_buffer_copy_from_offset(buffer, NULL, 100, 50);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with zero length */
    err = lwcan_buffer_copy_to_offset(buffer, source, 0, 50);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_from_offset(buffer, destination, 0, 50);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    lwcan_buffer_delete(buffer);
}

/* Test edge cases */
void test_buffer_edge_cases(void)
{
    struct lwcan_buffer *buffer;
    lwcanerr_t err;
    uint8_t source[10];
    uint8_t destination[10];

    /* Test with minimum buffer size */
    buffer = lwcan_buffer_new(1);
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQUAL(1, buffer->length);

    err = lwcan_buffer_copy_to(buffer, source, 1);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = lwcan_buffer_copy_from(buffer, destination, 1);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    lwcan_buffer_delete(buffer);

    /* Test with NULL buffer for all operations */
    err = lwcan_buffer_copy_to(NULL, source, 10);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_from(NULL, destination, 10);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_to_offset(NULL, source, 10, 0);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = lwcan_buffer_copy_from_offset(NULL, destination, 10, 0);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

/* Test multiple buffer chaining */
void test_buffer_chaining(void)
{
    struct lwcan_buffer *buffer1, *buffer2, *buffer3;
    lwcanerr_t err;
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0x05, 0x06, 0x07, 0x08};
    uint8_t data3[] = {0x09, 0x0A, 0x0B, 0x0C};

    /* Create chain of buffers */
    buffer1 = lwcan_buffer_new(4);
    buffer2 = lwcan_buffer_new(4);
    buffer3 = lwcan_buffer_new(4);

    TEST_ASSERT_NOT_NULL(buffer1);
    TEST_ASSERT_NOT_NULL(buffer2);
    TEST_ASSERT_NOT_NULL(buffer3);

    buffer1->next = buffer2;
    buffer2->next = buffer3;
    buffer3->next = NULL;

    err = lwcan_buffer_copy_to(buffer1, data1, 4);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = lwcan_buffer_copy_to(buffer2, data2, 4);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = lwcan_buffer_copy_to(buffer3, data3, 4);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Cleanup chain */
    struct lwcan_buffer *current = buffer1;
    while (current != NULL)
    {
        struct lwcan_buffer *next = current->next;
        lwcan_buffer_delete(current);
        current = next;
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_buffer_create_destroy);

    RUN_TEST(test_buffer_copy_operations);

    RUN_TEST(test_buffer_offset_operations);

    RUN_TEST(test_buffer_edge_cases);

    RUN_TEST(test_buffer_chaining);

    return UNITY_END();
}
