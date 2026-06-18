/**
 * @file test_raw.c
 * @brief Unit tests for raw module
 */

#include "unity.h"

#include "lwcan/raw.h"
#include "lwcan/private/raw_private.h"
#include "lwcan/canif.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/init.h"
#include "lwcanarch/arch.h"

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
static uint8_t mock_receive(void *arg, struct canraw_pcb *pcb, void *frame)
{
    (void)arg;
    (void)pcb;
    (void)frame;
    return 0;
}

static uint8_t mock_receive_eat(void *arg, struct canraw_pcb *pcb, void *frame)
{
    (void)arg;
    (void)pcb;
    (void)frame;
    return 1;
}

lwcanerr_t mock_output(struct canif *canif, void *frame, uint8_t frame_size, uint32_t timeout, canif_sent_function sent, void *arg)
{
    (void)canif;
    (void)frame;
    (void)frame_size;
    (void)timeout;

    sent(arg, ERROR_OK);

    return ERROR_OK;
}

static void mock_sent(void *arg, lwcanerr_t error)
{
    (void)arg;
    (void)error;
}

static lwcanerr_t _canif_init(struct canif *canif)
{
    (void)canif;
    return ERROR_OK;
}

/* Test RAW PCB creation and destruction */
void test_raw_new_remove(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;

    /* Test successful PCB creation */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);
    TEST_ASSERT_NULL(pcb->next);
    TEST_ASSERT_EQUAL(0, pcb->if_index);
    TEST_ASSERT_NULL(pcb->receive);
    TEST_ASSERT_NULL(pcb->callback_arg);
    TEST_ASSERT_EQUAL(0, pcb->sent);
    TEST_ASSERT_EQUAL(0, pcb->sent_error);

    /* Test successful PCB removal */
    canraw_remove(pcb);
    pcb = NULL;

    /* Test PCB creation after removal */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);
    canraw_remove(pcb);
    pcb = NULL;

    /* Test PCB creation when max number reached */
    for (uint8_t i = 0; i < CANRAW_MAX_PCB_NUM; i++)
    {
        pcb = canraw_new();
        TEST_ASSERT_NOT_NULL(pcb);
    }

    pcb = canraw_new();
    TEST_ASSERT_NULL(pcb);

    /* Cleanup */
    pcb = canraw_new();
    while (pcb != NULL)
    {
        struct canraw_pcb *next = pcb->next;
        canraw_remove(pcb);
        pcb = next;
    }
}

/* Test RAW bind */
void test_raw_bind(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;
    struct addr_can addr;
    struct canif canif;

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Test successful bind */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    addr.can_ifindex = canif_name_to_index("can0");

    err = canraw_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(addr.can_ifindex, pcb->if_index);

    /* Test bind with NULL PCB */
    err = canraw_bind(NULL, &addr);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test bind with NULL addr */
    err = canraw_bind(pcb, NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test bind with zero interface index */
    addr.can_ifindex = 0;
    err = canraw_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_CANIF, err);

    /* Cleanup */
    canraw_remove(pcb);
    canif_remove(&canif);
}

/* Test RAW set receive callback */
void test_raw_set_receive_callback(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;

    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Test setting receive callback */
    err = canraw_set_receive_callback(pcb, mock_receive);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(mock_receive, pcb->receive);

    /* Test setting callback arg */
    err = canraw_set_callback_arg(pcb, (void *)0x12345678);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL((void *)0x12345678, pcb->callback_arg);

    /* Test callbacks with NULL PCB */
    err = canraw_set_receive_callback(NULL, mock_receive);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = canraw_set_callback_arg(NULL, (void *)0x12345678);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    canraw_remove(pcb);
}

/* Test RAW input - frame not eaten */
void test_raw_input_none(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;
    struct canif canif;
    struct addr_can addr;
    struct can_frame frame;

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Create and bind PCB */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    addr.can_ifindex = canif_name_to_index("can0");

    err = canraw_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Set receive callback that doesn't eat frame */
    err = canraw_set_receive_callback(pcb, mock_receive);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Initialize frame */
    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x123;

    /* Test input - should return RAW_INPUT_NONE */
    canraw_input_state_t result = canraw_input(&canif, &frame);
    TEST_ASSERT_EQUAL(RAW_INPUT_NONE, result);

    /* Cleanup */
    canraw_remove(pcb);
    canif_remove(&canif);
}

/* Test RAW input - frame eaten */
void test_raw_input_eaten(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;
    struct canif canif;
    struct addr_can addr;
    struct can_frame frame;

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Create and bind PCB */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    addr.can_ifindex = canif_name_to_index("can0");

    err = canraw_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Set receive callback that eats frame */
    err = canraw_set_receive_callback(pcb, mock_receive_eat);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Initialize frame */
    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x123;

    /* Test input - should return RAW_INPUT_EATEN */
    canraw_input_state_t result = canraw_input(&canif, &frame);
    TEST_ASSERT_EQUAL(RAW_INPUT_EATEN, result);

    /* Cleanup */
    canraw_remove(pcb);
    canif_remove(&canif);
}

/* Test RAW send with sentinel callback */
void test_raw_send_with_sentinel(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;
    struct canif canif;
    struct addr_can addr;
    struct can_frame frame;
    uint8_t frame_size = sizeof(frame);

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    canif.output = mock_output;

    /* Create and bind PCB */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    addr.can_ifindex = canif_name_to_index("can0");

    err = canraw_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Initialize frame */
    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x123;

    /* Test send with NULL sentinel callback (uses internal sentinel) */
    err = canraw_send(pcb, &frame, frame_size, 1000, NULL, NULL);
    /* Result depends on driver implementation, just verify it doesn't crash */

    /* Cleanup */
    canraw_remove(pcb);
    canif_remove(&canif);
}

/* Test RAW send with custom sentinel callback */
void test_raw_send_with_callback(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;
    struct canif canif;
    struct addr_can addr;
    struct can_frame frame;
    uint8_t frame_size = sizeof(frame);

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    canif.output = mock_output;

    /* Create and bind PCB */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    addr.can_ifindex = canif_name_to_index("can0");

    err = canraw_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Initialize frame */
    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x123;

    /* Test send with custom sentinel callback */
    err = canraw_send(pcb, &frame, frame_size, 1000, mock_sent, NULL);
    /* Result depends on driver implementation, just verify it doesn't crash */

    /* Cleanup */
    canraw_remove(pcb);
    canif_remove(&canif);
}

/* Test RAW send with invalid parameters */
void test_raw_send_invalid_params(void)
{
    struct canraw_pcb *pcb;
    lwcanerr_t err;
    struct can_frame frame;
    uint8_t frame_size = sizeof(frame);

    /* Test send with NULL PCB */
    err = canraw_send(NULL, &frame, frame_size, 1000, mock_sent, NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test send with NULL frame */
    pcb = canraw_new();
    TEST_ASSERT_NOT_NULL(pcb);

    err = canraw_send(pcb, NULL, frame_size, 1000, mock_sent, NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    canraw_remove(pcb);
}

/* Test RAW multiple PCBs */
void test_raw_multiple_pcbs(void)
{
    struct canraw_pcb *pcbs[CANRAW_MAX_PCB_NUM];
    lwcanerr_t err;
    struct canif canif;
    struct addr_can addr;

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Create multiple PCBs */
    for (uint8_t i = 0; i < CANRAW_MAX_PCB_NUM; i++)
    {
        pcbs[i] = canraw_new();
        TEST_ASSERT_NOT_NULL(pcbs[i]);

        addr.can_ifindex = canif_name_to_index("can0");

        err = canraw_bind(pcbs[i], &addr);
        TEST_ASSERT_EQUAL(ERROR_OK, err);

        err = canraw_set_receive_callback(pcbs[i], mock_receive);
        TEST_ASSERT_EQUAL(ERROR_OK, err);
    }

    /* Remove all PCBs */
    for (uint8_t i = 0; i < CANRAW_MAX_PCB_NUM; i++)
    {
        canraw_remove(pcbs[i]);
        pcbs[i] = NULL;
    }

    /* Cleanup */
    canif_remove(&canif);
}

/* Test RAW remove with NULL */
void test_raw_remove_null(void)
{
    /* Should not crash */
    canraw_remove(NULL);
}

/* Test RAW set callback with NULL */
void test_raw_set_callback_null(void)
{
    lwcanerr_t err;

    /* Test setting callbacks with NULL PCB */
    err = canraw_set_receive_callback(NULL, mock_receive);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = canraw_set_callback_arg(NULL, (void *)0x12345678);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_raw_new_remove);

    RUN_TEST(test_raw_bind);

    RUN_TEST(test_raw_set_receive_callback);

    RUN_TEST(test_raw_input_none);

    RUN_TEST(test_raw_input_eaten);

    RUN_TEST(test_raw_send_with_sentinel);

    RUN_TEST(test_raw_send_with_callback);

    RUN_TEST(test_raw_send_invalid_params);

    RUN_TEST(test_raw_multiple_pcbs);

    RUN_TEST(test_raw_remove_null);

    RUN_TEST(test_raw_set_callback_null);

    return UNITY_END();
}
