/**
 * @file test_isotp.c
 * @brief Unit tests for ISOTP module
 */

#include "unity.h"

#include "lwcan/isotp.h"
#include "lwcan/canif.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/private/isotp_private.h"

#include <stdint.h>
#include <string.h>

/* Test setup and teardown */
void setUp(void)
{
    isotp_init();
}

void tearDown(void)
{
}

/* Mock callback functions */
static void mock_receive(void *arg, struct isotp_pcb *pcb, struct lwcan_buffer *buffer)
{
    (void)arg;
    (void)pcb;
    (void)buffer;
}

static void mock_receive_ff(void *arg, struct isotp_pcb *pcb)
{
    (void)arg;
    (void)pcb;
}

static void mock_sent(void *arg, struct isotp_pcb *pcb, uint32_t length)
{
    (void)arg;
    (void)pcb;
    (void)length;
}

static void mock_error(void *arg, lwcanerr_t error)
{
    (void)arg;
    (void)error;
}

static lwcanerr_t _canif_init(struct canif *canif)
{
    (void)canif;
    return ERROR_OK;
}

/* Test ISOTP PCB creation and destruction */
void test_isotp_new_remove(void)
{
    struct isotp_pcb *pcb;
    lwcanerr_t err;
    struct addr_can addr;

    /* Test successful PCB creation */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Test successful PCB removal */
    isotp_remove(pcb);
    pcb = NULL;

    /* Test PCB creation after removal */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);
    isotp_remove(pcb);
    pcb = NULL;

    /* Test PCB creation when max number reached */
    for (uint8_t i = 0; i < ISOTP_MAX_PCB_NUM; i++)
    {
        pcb = isotp_new();
        TEST_ASSERT_NOT_NULL(pcb);
    }

    pcb = isotp_new();
    TEST_ASSERT_NULL(pcb);

    /* Cleanup */
    pcb = isotp_get_pcb_list();
    while (pcb != NULL)
    {
        struct isotp_pcb *next = pcb->next;
        isotp_remove(pcb);
        pcb = next;
    }
}

/* Test ISOTP bind */
void test_isotp_bind(void)
{
    struct isotp_pcb *pcb;
    lwcanerr_t err;
    struct addr_can addr;
    struct canif canif;

    /* Add a CAN interface */
    err = canif_add(&canif, "can0", _canif_init);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Test successful bind */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    addr.can_ifindex = 1;
    addr.can_addr.tp.tx_id = 0x7E0;
    addr.can_addr.tp.rx_id = 0x7E8;

    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(1, pcb->if_index);
    TEST_ASSERT_EQUAL(0x7E0, pcb->tx_id);
    TEST_ASSERT_EQUAL(0x7E8, pcb->rx_id);

    /* Test bind with NULL PCB */
    err = isotp_bind(NULL, &addr);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test bind with NULL addr */
    err = isotp_bind(pcb, NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test bind with zero interface index */
    addr.can_ifindex = 0;
    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_CANIF, err);
    /* Debug: print actual value */
    TEST_ASSERT_EQUAL(0, addr.can_ifindex);

    /* Test bind with invalid CAN ID (EFF flag set) */
    addr.can_ifindex = 1;
    addr.can_addr.tp.tx_id = 0x2e8924; /* SFF as EFF */
    addr.can_addr.tp.rx_id = 0x7E8;
    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    isotp_remove(pcb);
    canif_remove(&canif);
}

/* Test ISOTP callback functions */
void test_isotp_callbacks(void)
{
    struct isotp_pcb *pcb;
    lwcanerr_t err;

    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Test setting receive callback */
    err = isotp_set_receive_callback(pcb, mock_receive);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(mock_receive, pcb->receive);

    /* Test setting receive FF callback */
    err = isotp_set_receive_ff_callback(pcb, mock_receive_ff);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(mock_receive_ff, pcb->receive_ff);

    /* Test setting sent callback */
    err = isotp_set_sent_callback(pcb, mock_sent);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(mock_sent, pcb->sent);

    /* Test setting error callback */
    err = isotp_set_error_callback(pcb, mock_error);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(mock_error, pcb->error);

    /* Test setting callback arg */
    err = isotp_set_callback_arg(pcb, (void *)0x12345678);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL((void *)0x12345678, pcb->callback_arg);

    /* Test callbacks with NULL PCB */
    err = isotp_set_receive_callback(NULL, mock_receive);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = isotp_set_receive_ff_callback(NULL, mock_receive_ff);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = isotp_set_sent_callback(NULL, mock_sent);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = isotp_set_error_callback(NULL, mock_error);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    err = isotp_set_callback_arg(NULL, (void *)0x12345678);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    isotp_remove(pcb);
}

/* Test ISOTP received */
void test_isotp_received(void)
{
    struct isotp_pcb *pcb;
    lwcanerr_t err;
    struct lwcan_buffer *buffer;

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Create a buffer */
    buffer = lwcan_buffer_new(10);
    TEST_ASSERT_NOT_NULL(buffer);

    /* Test isotp_received with valid parameters */
    err = isotp_received(pcb, buffer);
    TEST_ASSERT_EQUAL(ERROR_OK, err);

    /* Test isotp_received with NULL PCB */
    err = isotp_received(NULL, buffer);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test isotp_received with NULL buffer */
    err = isotp_received(pcb, NULL);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    isotp_remove(pcb);
}

/* Test ISOTP send with invalid parameters */
void test_isotp_send_invalid_params(void)
{
    struct isotp_pcb *pcb;
    lwcanerr_t err;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};

    /* Test send with NULL PCB */
    err = isotp_send(NULL, data, sizeof(data));
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test send with NULL data */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    err = isotp_send(pcb, NULL, sizeof(data));
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test send with zero length */
    err = isotp_send(pcb, data, 0);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    isotp_remove(pcb);
}

/* Test ISOTP get PCB list */
void test_isotp_get_pcb_list(void)
{
    struct isotp_pcb *pcb1, *pcb2, *pcb_list;

    /* Create first PCB */
    pcb1 = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb1);

    /* Create second PCB */
    pcb2 = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb2);

    /* Get PCB list */
    pcb_list = isotp_get_pcb_list();
    TEST_ASSERT_EQUAL(pcb2, pcb_list); /* Second PCB should be at head */
    TEST_ASSERT_EQUAL(pcb1, pcb_list->next); /* First PCB should be next */

    /* Cleanup */
    isotp_remove(pcb1);
    isotp_remove(pcb2);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_isotp_new_remove);

    RUN_TEST(test_isotp_bind);

    RUN_TEST(test_isotp_callbacks);

    RUN_TEST(test_isotp_received);

    RUN_TEST(test_isotp_send_invalid_params);

    RUN_TEST(test_isotp_get_pcb_list);

    return UNITY_END();
}
