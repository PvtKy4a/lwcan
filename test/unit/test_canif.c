/**
 * @file test_canif.c
 * @brief Unit tests for canif module
 */

#include "unity.h"

#include "lwcan/canif.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/init.h"
#include "lwcanarch/system_arch.h"

#include <stdint.h>
#include <string.h>

/* Test setup and teardown */
void setUp(void)
{
    lwcan_init();

    mock_system_time_set(0);
}

void tearDown(void)
{
}

/* Mock callback functions */
static lwcanerr_t mock_init(struct canif *canif)
{
    (void)canif;
    return ERROR_OK;
}

static lwcanerr_t mock_init_error(struct canif *canif)
{
    (void)canif;
    return ERROR_IF;
}

static lwcanerr_t mock_output(struct canif *canif, void *frame, uint8_t frame_size, uint32_t timeout, canif_sent_function sent, void *arg)
{
    (void)canif;
    (void)frame;
    (void)frame_size;
    (void)timeout;

    sent(arg, ERROR_OK);

    return ERROR_OK;
}

static lwcanerr_t mock_set_bitrate(struct canif *canif, uint32_t bitrate)
{
    (void)canif;
    (void)bitrate;
    return ERROR_OK;
}

static lwcanerr_t mock_set_filter(struct canif *canif, struct can_filter *filter)
{
    (void)canif;
    (void)filter;
    return ERROR_OK;
}

static void mock_sent(void *arg, lwcanerr_t error)
{
    (void)arg;
    (void)error;
}

/* Test CANIF add */
void test_canif_add(void)
{
    struct canif canif;
    lwcanerr_t err;

    /* Test successful add */
    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Cleanup */
    err = canif_remove(&canif);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
}

/* Test CANIF add with NULL canif */
void test_canif_add_null_canif(void)
{
    lwcanerr_t err;

    err = canif_add(NULL, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

/* Test CANIF add with NULL name */
void test_canif_add_null_name(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, NULL, mock_init);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

/* Test CANIF add with empty name */
void test_canif_add_empty_name(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "", mock_init);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

/* Test CANIF add with NULL init */
void test_canif_add_null_init(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

/* Test CANIF add duplicate */
void test_canif_add_duplicate(void)
{
    struct canif canif1;
    lwcanerr_t err;

    err = canif_add(&canif1, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_add(&canif1, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_CANIF, err);

    /* Cleanup */
    canif_remove(&canif1);
}

/* Test CANIF add multiple */
void test_canif_add_multiple(void)
{
    struct canif canif1, canif2, canif3;
    lwcanerr_t err;

    err = canif_add(&canif1, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_add(&canif2, "can1", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_add(&canif3, "can2", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Cleanup */
    canif_remove(&canif1);
    canif_remove(&canif2);
    canif_remove(&canif3);
}

/* Test CANIF remove */
void test_canif_remove(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_remove(&canif);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Verify removed */
    TEST_ASSERT_NULL(canif_get_by_index(canif_get_index(&canif)));
}

/* Test CANIF remove with NULL */
void test_canif_remove_null(void)
{
    lwcanerr_t err;

    err = canif_remove(NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

/* Test CANIF get by name */
void test_canif_get_by_name(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    struct canif *found = canif_get_by_name("can0");
    TEST_ASSERT_EQUAL(&canif, found);

    /* Test with NULL name */
    found = canif_get_by_name(NULL);
    TEST_ASSERT_NULL(found);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF get by index */
void test_canif_get_by_index(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    struct canif *found = canif_get_by_index(canif_get_index(&canif));
    TEST_ASSERT_EQUAL(&canif, found);

    /* Test with index 0 (invalid) */
    found = canif_get_by_index(0);
    TEST_ASSERT_NULL(found);

    /* Test with non-existent index */
    found = canif_get_by_index(99);
    TEST_ASSERT_NULL(found);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF name to index conversion */
void test_canif_name_to_index(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    uint8_t index = canif_name_to_index("can0");
    TEST_ASSERT_EQUAL(canif_get_index(&canif), index);

    /* Test with NULL name */
    index = canif_name_to_index(NULL);
    TEST_ASSERT_EQUAL(0, index);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF index to name conversion */
void test_canif_index_to_name(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    const char *name = canif_index_to_name(canif_get_index(&canif));
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_MEMORY("can0", name, 4);

    /* Test with index 0 (invalid) */
    name = canif_index_to_name(0);
    TEST_ASSERT_NULL(name);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF set bitrate */
void test_canif_set_bitrate(void)
{
    struct canif canif;
    lwcanerr_t err;

    canif.set_bitrate = mock_set_bitrate;

    err = canif_set_bitrate(&canif, 500000);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Test with NULL canif */
    err = canif_set_bitrate(NULL, 500000);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with zero bitrate */
    err = canif_set_bitrate(&canif, 0);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with no set_bitrate function */
    canif.set_bitrate = NULL;
    err = canif_set_bitrate(&canif, 500000);
    TEST_ASSERT_EQUAL(ERROR_CANIF, err);
}

/* Test CANIF set filter */
void test_canif_set_filter(void)
{
    struct canif canif;
    lwcanerr_t err;
    struct can_filter filter;

    canif.set_filter = mock_set_filter;

    err = canif_set_filter(&canif, &filter);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Test with NULL canif */
    err = canif_set_filter(NULL, &filter);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with NULL filter */
    err = canif_set_filter(&canif, NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test with no set_filter function */
    canif.set_filter = NULL;
    err = canif_set_filter(&canif, &filter);
    TEST_ASSERT_EQUAL(ERROR_CANIF, err);
}

/* Test CANIF get name */
void test_canif_get_name(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    const char *name = canif_get_name(&canif);
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_MEMORY("can0", name, 4);

    /* Test with NULL canif */
    name = canif_get_name(NULL);
    TEST_ASSERT_NULL(name);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF get index */
void test_canif_get_index(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    uint8_t index = canif_get_index(&canif);
    TEST_ASSERT_EQUAL(canif_name_to_index("can0"), index);

    /* Test with NULL canif */
    index = canif_get_index(NULL);
    TEST_ASSERT_EQUAL(0, index);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF remove from middle of list */
void test_canif_remove_from_middle(void)
{
    struct canif canif1, canif2, canif3;
    lwcanerr_t err;

    err = canif_add(&canif1, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_add(&canif2, "can1", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_add(&canif3, "can2", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Remove middle element */
    err = canif_remove(&canif2);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Verify remaining elements */
    TEST_ASSERT_NOT_NULL(canif_get_by_name("can0"));
    TEST_ASSERT_NOT_NULL(canif_get_by_name("can2"));

    /* Verify removed element is not found */
    TEST_ASSERT_NULL(canif_get_by_name("can1"));

    /* Cleanup */
    canif_remove(&canif1);
    canif_remove(&canif3);
}

/* Test CANIF multiple add with same name after remove */
void test_canif_add_same_name_after_remove(void)
{
    struct canif canif1, canif2;
    lwcanerr_t err;

    err = canif_add(&canif1, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_remove(&canif1);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    err = canif_add(&canif2, "can0", mock_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Cleanup */
    canif_remove(&canif2);
}

/* Test CANIF output function */
void test_canif_output_function(void)
{
    struct canif canif;
    lwcanerr_t err;
    struct can_frame frame;

    canif.output = mock_output;

    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x123;

    err = canif.output(&canif, &frame, sizeof(frame), 1000, mock_sent, NULL);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Cleanup */
    canif_remove(&canif);
}

/* Test CANIF init function return error */
void test_canif_init_error(void)
{
    struct canif canif;
    lwcanerr_t err;

    err = canif_add(&canif, "can0", mock_init_error);
    TEST_ASSERT_EQUAL(ERROR_IF, err);

    /* Verify canif was not added */
    TEST_ASSERT_NULL(canif_get_by_name("can0"));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_canif_add);

    RUN_TEST(test_canif_add_null_canif);

    RUN_TEST(test_canif_add_null_name);

    RUN_TEST(test_canif_add_empty_name);

    RUN_TEST(test_canif_add_null_init);

    RUN_TEST(test_canif_add_duplicate);

    RUN_TEST(test_canif_add_multiple);

    RUN_TEST(test_canif_remove);

    RUN_TEST(test_canif_remove_null);

    RUN_TEST(test_canif_get_by_name);

    RUN_TEST(test_canif_get_by_index);

    RUN_TEST(test_canif_name_to_index);

    RUN_TEST(test_canif_index_to_name);

    RUN_TEST(test_canif_set_bitrate);

    RUN_TEST(test_canif_set_filter);

    RUN_TEST(test_canif_get_name);

    RUN_TEST(test_canif_get_index);

    RUN_TEST(test_canif_remove_from_middle);

    RUN_TEST(test_canif_add_same_name_after_remove);

    RUN_TEST(test_canif_output_function);

    RUN_TEST(test_canif_init_error);

    return UNITY_END();
}
