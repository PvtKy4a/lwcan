/**
 * @file test_isotp.c
 * @brief Unit tests for ISOTP module
 */

#include "unity.h"

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/canif.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/init.h"
#include "lwcanarch/arch.h"

#include <stdint.h>
#include <string.h>

static uint8_t mock_error_called;

/* Test setup and teardown */
void setUp(void)
{
    lwcan_init();

    mock_system_time_set(0);

    mock_error_called = 0;
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

    mock_error_called = 1;
}

static lwcanerr_t _canif_init(struct canif *canif)
{
    (void)canif;
    return ERROR_OK;
}

/* Test ISOTP PCB creation and destruction */
void test_isotp_new_remove(void)
{
    struct isotp_pcb *pcbs[ISOTP_MAX_PCB_NUM];
    struct isotp_pcb *pcb;
    lwcanerr_t err;
    struct addr_can addr;

    /* Test successful PCB creation */
    pcbs[0] = isotp_new();
    TEST_ASSERT_NOT_NULL(pcbs[0]);

    /* Test successful PCB removal */
    isotp_remove(pcbs[0]);

    /* Remove NULL pointer (should be safe) */
    isotp_remove(NULL); /* Should not crash */

    /* Test PCB creation after removal */
    pcbs[0] = isotp_new();
    TEST_ASSERT_NOT_NULL(pcbs[0]);
    isotp_remove(pcbs[0]);

    /* Test PCB maximum */
    for (uint8_t i = 0; i < ISOTP_MAX_PCB_NUM; i++)
    {
        pcbs[i] = isotp_new();
        TEST_ASSERT_NOT_NULL(pcbs[i]);
    }

    /* Test PCB creation when max number reached */
    pcb = isotp_new();
    TEST_ASSERT_NULL(pcb);
    isotp_remove(pcb);

    /* Cleanup */
    for (uint8_t i = 0; i < ISOTP_MAX_PCB_NUM; i++)
    {
        isotp_remove(pcbs[i]);
    }
}

/* Test ISOTP bind */
void test_isotp_bind(void)
{
    struct isotp_pcb *pcb;
    lwcanerr_t err;
    struct addr_can addr;

    /* Test successful bind with SFF */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);
    addr.can_ifindex = 1;
    addr.can_addr.tp.tx_id = 0x7E0;
    addr.can_addr.tp.rx_id = 0x7E8;
    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(addr.can_ifindex, pcb->if_index);
    TEST_ASSERT_EQUAL(addr.can_addr.tp.tx_id, pcb->tx_id);
    TEST_ASSERT_EQUAL(addr.can_addr.tp.rx_id, pcb->rx_id);

    /* Test successful bind with EFF */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);
    addr.can_ifindex = 1;
    addr.can_addr.tp.tx_id = 0x18F00102 | CAN_EFF_FLAG;
    addr.can_addr.tp.rx_id = 0x18F00201 | CAN_EFF_FLAG;
    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_EQUAL(addr.can_ifindex, pcb->if_index);
    TEST_ASSERT_EQUAL(addr.can_addr.tp.tx_id, pcb->tx_id);
    TEST_ASSERT_EQUAL(addr.can_addr.tp.rx_id, pcb->rx_id);

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

    /* Test bind with invalid tx_id EFF */
    addr.can_ifindex = 1;
    addr.can_addr.tp.tx_id = 0x18F00102; /* EFF without EFF flag */
    addr.can_addr.tp.rx_id = 0x7E8;
    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Test bind with invalid rx_id EFF */
    addr.can_ifindex = 1;
    addr.can_addr.tp.tx_id = 0x7E0;
    addr.can_addr.tp.rx_id = 0x18F00201; /* EFF without EFF flag */
    err = isotp_bind(pcb, &addr);
    TEST_ASSERT_EQUAL(ERROR_ARG, err);

    /* Cleanup */
    isotp_remove(pcb);
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
    buffer->next = NULL;

    /* Test isotp_received with valid parameters */
    pcb->input_flow.buffer = buffer;
    err = isotp_received(pcb, buffer);
    TEST_ASSERT_EQUAL(ERROR_OK, err);
    TEST_ASSERT_NULL(pcb->input_flow.buffer);

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

/* Test ISOTP private frame filling functions */
void test_isotp_fill_sf(void)
{
#if !ISOTP_CANFD
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct lwcan_buffer *buffer;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    struct can_frame frame;

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Create a buffer */
    buffer = lwcan_buffer_new(sizeof(data));
    TEST_ASSERT_NOT_NULL(buffer);
    lwcan_buffer_copy_to(buffer, data, sizeof(data));

    /* Setup flow */
    flow = &pcb->output_flow;
    flow->buffer = buffer;
    flow->remaining_data = sizeof(data);

    /* Fill single frame */
    isotp_fill_sf(flow, &frame);

    /* Verify frame length */
    TEST_ASSERT_EQUAL(CAN_MAX_DLEN, frame.len);

    /* Verify remaining data */
    TEST_ASSERT_EQUAL(0, flow->remaining_data);

    /* Verify frame type */
    TEST_ASSERT_EQUAL(SF, frame.data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);

    /* Verify data length */
    TEST_ASSERT_EQUAL(sizeof(data), (frame.data[SF_DL_OFFSET] & SF_DL_MASK));

    /* Verify data content */
    TEST_ASSERT_EQUAL(0x01, frame.data[SF_DATA_OFFSET]);
    TEST_ASSERT_EQUAL(0x02, frame.data[SF_DATA_OFFSET + 1]);
    TEST_ASSERT_EQUAL(0x03, frame.data[SF_DATA_OFFSET + 2]);
    TEST_ASSERT_EQUAL(0x04, frame.data[SF_DATA_OFFSET + 3]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[SF_DATA_OFFSET + 4]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[SF_DATA_OFFSET + 5]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[SF_DATA_OFFSET + 6]);

    /* Cleanup */
    isotp_remove(pcb);
    lwcan_buffer_delete(buffer);
#else
    TEST_IGNORE();
#endif
}

void test_isotp_fill_ff(void)
{
#if !ISOTP_CANFD
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct lwcan_buffer *buffer;
    uint8_t data[10];
    struct can_frame frame;

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Create a buffer with larger data */
    buffer = lwcan_buffer_new(sizeof(data));
    TEST_ASSERT_NOT_NULL(buffer);

    /* Fill data */
    for (int i = 0; i < sizeof(data); i++)
    {
        data[i] = (uint8_t)(i & 0xFF);
    }
    lwcan_buffer_copy_to(buffer, data, sizeof(data));

    /* Setup flow */
    flow = &pcb->output_flow;
    flow->buffer = buffer;
    flow->remaining_data = sizeof(data);

    /* Fill first frame */
    isotp_fill_ff(flow, &frame);

    /* Verify frame length */
    TEST_ASSERT_EQUAL(CAN_MAX_DLEN, frame.len);

    /* Verify remaining data */
    TEST_ASSERT_EQUAL(sizeof(data) - 6, flow->remaining_data);

    /* Verify frame type */
    TEST_ASSERT_EQUAL(FF, frame.data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);

    /* Verify data length */
    TEST_ASSERT_EQUAL(0x00, (frame.data[FF_DL_HI_OFFSET] & FF_DL_HI_MASK));
    TEST_ASSERT_EQUAL(sizeof(data), frame.data[FF_DL_LO_OFFSET]);

    /* Verify data content */
    TEST_ASSERT_EQUAL(0x00, frame.data[FF_DATA_OFFSET]);
    TEST_ASSERT_EQUAL(0x01, frame.data[FF_DATA_OFFSET + 1]);
    TEST_ASSERT_EQUAL(0x02, frame.data[FF_DATA_OFFSET + 2]);
    TEST_ASSERT_EQUAL(0x03, frame.data[FF_DATA_OFFSET + 3]);
    TEST_ASSERT_EQUAL(0x04, frame.data[FF_DATA_OFFSET + 4]);
    TEST_ASSERT_EQUAL(0x05, frame.data[FF_DATA_OFFSET + 5]);

    /* Cleanup */
    isotp_remove(pcb);
    lwcan_buffer_delete(buffer);
#else
    TEST_IGNORE();
#endif
}

void test_isotp_fill_cf(void)
{
#if !ISOTP_CANFD
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct lwcan_buffer *buffer;
    uint8_t data[10];
    struct can_frame frame;

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Create a buffer with larger data */
    buffer = lwcan_buffer_new(sizeof(data));
    TEST_ASSERT_NOT_NULL(buffer);
    for (int i = 0; i < sizeof(data); i++)
    {
        data[i] = (uint8_t)(i & 0xFF);
    }
    lwcan_buffer_copy_to(buffer, data, sizeof(data));

    /* Setup flow */
    flow = &pcb->output_flow;
    flow->buffer = buffer;
    flow->remaining_data = sizeof(data);
    flow->cf_sn = 1;

    /* Fill consecutive frame */
    isotp_fill_cf(flow, &frame);

    /* Verify frame length */
    TEST_ASSERT_EQUAL(CAN_MAX_DLEN, frame.len);

    /* Verify remaining data */
    TEST_ASSERT_EQUAL(sizeof(data) - 7, flow->remaining_data);

    /* Verify frame type */
    TEST_ASSERT_EQUAL(CF, frame.data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);

    /* Verify sequence number */
    TEST_ASSERT_EQUAL(1, (frame.data[CF_SN_OFFSET] & CF_SN_MASK));

    /* Verify data content */
    TEST_ASSERT_EQUAL(0x00, frame.data[CF_DATA_OFFSET]);
    TEST_ASSERT_EQUAL(0x01, frame.data[CF_DATA_OFFSET + 1]);
    TEST_ASSERT_EQUAL(0x02, frame.data[CF_DATA_OFFSET + 2]);
    TEST_ASSERT_EQUAL(0x03, frame.data[CF_DATA_OFFSET + 3]);
    TEST_ASSERT_EQUAL(0x04, frame.data[CF_DATA_OFFSET + 4]);
    TEST_ASSERT_EQUAL(0x05, frame.data[CF_DATA_OFFSET + 5]);
    TEST_ASSERT_EQUAL(0x06, frame.data[CF_DATA_OFFSET + 6]);

    /* Setup next sequence number */
    flow->cf_sn = 2;

    /* Fill consecutive frame */
    isotp_fill_cf(flow, &frame);

    /* Verify remaining data */
    TEST_ASSERT_EQUAL(0, flow->remaining_data);

    /* Verify frame type */
    TEST_ASSERT_EQUAL(CF, frame.data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);

    /* Verify sequence number */
    TEST_ASSERT_EQUAL(2, (frame.data[CF_SN_OFFSET] & CF_SN_MASK));

    /* Verify data content */
    TEST_ASSERT_EQUAL(0x07, frame.data[CF_DATA_OFFSET]);
    TEST_ASSERT_EQUAL(0x08, frame.data[CF_DATA_OFFSET + 1]);
    TEST_ASSERT_EQUAL(0x09, frame.data[CF_DATA_OFFSET + 2]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[CF_DATA_OFFSET + 3]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[CF_DATA_OFFSET + 4]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[CF_DATA_OFFSET + 5]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[CF_DATA_OFFSET + 6]);

    /* Cleanup */
    isotp_remove(pcb);
    lwcan_buffer_delete(buffer);
#else
    TEST_IGNORE();
#endif
}

void test_isotp_fill_fc(void)
{
#if !ISOTP_CANFD
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct can_frame frame;

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Setup flow */
    flow = &pcb->output_flow;
    flow->fs = FS_READY;
    flow->bs = 0;
    flow->st = 0;

    /* Fill flow control frame */
    isotp_fill_fc(flow, &frame);

    /* Verify frame type */
    TEST_ASSERT_EQUAL(FC, frame.data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);

    /* Verify flow status */
    TEST_ASSERT_EQUAL(FS_READY, (frame.data[FC_FS_OFFSET] & FC_FS_MASK));

    /* Verify block size */
    TEST_ASSERT_EQUAL(0, frame.data[FC_BS_OFFSET]);

    /* Verify separation time */
    TEST_ASSERT_EQUAL(0, frame.data[FC_ST_OFFSET]);

    /* Verify padding bytes */
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[FC_PADDING_OFFSET]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[FC_PADDING_OFFSET + 1]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[FC_PADDING_OFFSET + 2]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[FC_PADDING_OFFSET + 3]);
    TEST_ASSERT_EQUAL(ISOTP_PADDING_BYTE, frame.data[FC_PADDING_OFFSET + 4]);

    /* Cleanup */
    isotp_remove(pcb);
#else
    TEST_IGNORE();
#endif
}

/* Test ISOTP private frame parsing functions */
void test_isotp_get_sf_dl(void)
{
#if !ISOTP_CANFD
    uint8_t frame_data[8];

    frame_data[FRAME_TYPE_OFFSET] = SF;
    frame_data[SF_DL_OFFSET] |= (0x04 & SF_DL_MASK); /* DL = 4 */
    TEST_ASSERT_EQUAL(4, isotp_get_sf_dl(frame_data));

    frame_data[FRAME_TYPE_OFFSET] = SF;
    frame_data[SF_DL_OFFSET] |= (0x08 & SF_DL_MASK); /* DL = 8 */
    TEST_ASSERT_EQUAL(8, isotp_get_sf_dl(frame_data));
#else
    TEST_IGNORE();
#endif
}

void test_isotp_get_ff_dl(void)
{
#if !ISOTP_CANFD
    uint8_t frame_data[8];

    frame_data[FRAME_TYPE_OFFSET] = FF;
    frame_data[FF_DL_HI_OFFSET] = (0x00 & FF_DL_HI_MASK); /* DL hi = 0 */
    frame_data[FF_DL_LO_OFFSET] = 0x4; /* DL lo = 4 */
    TEST_ASSERT_EQUAL(4, isotp_get_ff_dl(frame_data));

    frame_data[FRAME_TYPE_OFFSET] = FF;
    frame_data[FF_DL_HI_OFFSET] = (0x00 & FF_DL_HI_MASK); /* DL hi = 0 */
    frame_data[FF_DL_LO_OFFSET] = 0x8; /* DL lo = 8 */
    TEST_ASSERT_EQUAL(8, isotp_get_ff_dl(frame_data));

    frame_data[FRAME_TYPE_OFFSET] = FF;
    frame_data[FF_DL_HI_OFFSET] = (0x00 & FF_DL_HI_MASK); /* DL hi = 0 */
    frame_data[FF_DL_LO_OFFSET] = 0x14; /* DL lo = 20 */
    TEST_ASSERT_EQUAL(20, isotp_get_ff_dl(frame_data));

    frame_data[FRAME_TYPE_OFFSET] = FF;
    frame_data[FF_DL_HI_OFFSET] = (0x01 & FF_DL_HI_MASK); /* DL hi = 1 */
    frame_data[FF_DL_LO_OFFSET] = 0x00; /* DL lo = 0 */
    TEST_ASSERT_EQUAL(256, isotp_get_ff_dl(frame_data));
#else
    TEST_IGNORE();
#endif
}

/* Test ISOTP buffer management */
void test_isotp_remove_buffer(void)
{
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct lwcan_buffer *buffer1, *buffer2, *buffer3;
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t data2[] = {0x05, 0x06, 0x07, 0x08};
    uint8_t data3[] = {0x09, 0x0A, 0x0B, 0x0C};

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Create a chain of buffers */
    buffer1 = lwcan_buffer_new(4);
    buffer2 = lwcan_buffer_new(4);
    buffer3 = lwcan_buffer_new(4);

    TEST_ASSERT_NOT_NULL(buffer1);
    TEST_ASSERT_NOT_NULL(buffer2);
    TEST_ASSERT_NOT_NULL(buffer3);

    lwcan_buffer_copy_to(buffer1, data1, 4);
    lwcan_buffer_copy_to(buffer2, data2, 4);
    lwcan_buffer_copy_to(buffer3, data3, 4);

    buffer1->next = buffer2;
    buffer2->next = buffer3;
    buffer3->next = NULL;

    flow = &pcb->output_flow;
    flow->buffer = buffer1;

    /* Test removing first buffer */
    isotp_remove_buffer(flow, buffer1);

    TEST_ASSERT_EQUAL(buffer2, flow->buffer);

    /* Test removing middle buffer */
    isotp_remove_buffer(flow, buffer2);

    TEST_ASSERT_EQUAL(buffer3, flow->buffer);

    /* Test removing last buffer */
    isotp_remove_buffer(flow, buffer3);

    TEST_ASSERT_NULL(flow->buffer);

    /* Cleanup */
    isotp_remove(pcb);
}

void test_isotp_remove_buffer_null_args(void)
{
    struct isotp_flow flow;
    struct lwcan_buffer buffer;

    /* Test with NULL flow */
    isotp_remove_buffer(NULL, &buffer);
    TEST_IGNORE_MESSAGE("NULL flow test - should not crash");

    /* Test with NULL buffer */
    isotp_remove_buffer(&flow, NULL);
    TEST_IGNORE_MESSAGE("NULL buffer test - should not crash");
}

/* Test ISOTP timeout error handler */
void test_isotp_timeout_error_handler_output_flow(void)
{
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct lwcan_buffer *buffer;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    lwcanerr_t error_result = ERROR_OK;

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Set error callback to capture error */
    isotp_set_error_callback(pcb, mock_error);

    /* Create a buffer */
    buffer = lwcan_buffer_new(sizeof(data));
    TEST_ASSERT_NOT_NULL(buffer);
    buffer->next = NULL;
    lwcan_buffer_copy_to(buffer, data, sizeof(data));

    /* Setup output flow */
    flow = &pcb->output_flow;
    flow->buffer = buffer;
    flow->state = ISOTP_TX_CF;

    /* Call timeout error handler */
    isotp_timeout_error_handler(flow);

    /* Verify buffer was removed */
    TEST_ASSERT_NULL(flow->buffer);

    /* Verify state was reset */
    TEST_ASSERT_EQUAL(ISOTP_IDLE, flow->state);

    /* Verify error callback was called */
    TEST_ASSERT_EQUAL(1, mock_error_called);

    /* Cleanup */
    isotp_remove(pcb);
    lwcan_buffer_delete(buffer);
}

void test_isotp_timeout_error_handler_input_flow(void)
{
    struct isotp_pcb *pcb;
    struct isotp_flow *flow;
    struct lwcan_buffer *buffer;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};

    /* Create PCB */
    pcb = isotp_new();
    TEST_ASSERT_NOT_NULL(pcb);

    /* Set error callback to capture error */
    isotp_set_error_callback(pcb, mock_error);

    /* Create a buffer */
    buffer = lwcan_buffer_new(sizeof(data));
    TEST_ASSERT_NOT_NULL(buffer);
    buffer->next = NULL;
    lwcan_buffer_copy_to(buffer, data, sizeof(data));

    /* Setup input flow */
    flow = &pcb->input_flow;
    flow->buffer = buffer;
    flow->state = ISOTP_WAIT_CF;

    /* Call timeout error handler */
    isotp_timeout_error_handler(flow);

    /* Verify buffer was removed */
    TEST_ASSERT_NULL(flow->buffer);

    /* Verify state was reset */
    TEST_ASSERT_EQUAL(ISOTP_IDLE, flow->state);

    /* Verify error callback was called */
    TEST_ASSERT_EQUAL(1, mock_error_called);

    /* Cleanup */
    isotp_remove(pcb);
    lwcan_buffer_delete(buffer);
}

void test_isotp_timeout_error_handler_null_arg(void)
{
    /* Test with NULL arg - should not crash */
    isotp_timeout_error_handler(NULL);
    TEST_IGNORE_MESSAGE("NULL arg test - should not crash");
}

/* Test ISOTP state enumeration */
void test_isotp_states(void)
{
    /* Test that all states are defined */
    TEST_ASSERT_EQUAL(0, ISOTP_IDLE);
    TEST_ASSERT_EQUAL(1, ISOTP_TX_SF);
    TEST_ASSERT_EQUAL(2, ISOTP_TX_FF);
    TEST_ASSERT_EQUAL(3, ISOTP_TX_CF);
    TEST_ASSERT_EQUAL(4, ISOTP_TX_FC);
    TEST_ASSERT_EQUAL(5, ISOTP_WAIT_CF);
    TEST_ASSERT_EQUAL(6, ISOTP_WAIT_FC);
}

/* Test ISOTP frame type constants */
void test_isotp_frame_types(void)
{
    /* Test frame type constants */
    TEST_ASSERT_EQUAL(0x00, SF);
    TEST_ASSERT_EQUAL(0x10, FF);
    TEST_ASSERT_EQUAL(0x20, CF);
    TEST_ASSERT_EQUAL(0x30, FC);

    /* Test frame type masks */
    TEST_ASSERT_EQUAL(0xF0, FRAME_TYPE_MASK);
    TEST_ASSERT_EQUAL(0x0F, SF_DL_MASK);
    TEST_ASSERT_EQUAL(0x0F, CF_SN_MASK);
    TEST_ASSERT_EQUAL(0x0F, FC_FS_MASK);
}

/* Test ISOTP flow control constants */
void test_isotp_flow_control(void)
{
    /* Test flow status constants */
    TEST_ASSERT_EQUAL(0, FS_READY);
    TEST_ASSERT_EQUAL(1, FS_WAIT);
    TEST_ASSERT_EQUAL(2, FS_OVERFLOW);

    /* Test offset constants */
    TEST_ASSERT_EQUAL(0, FC_FS_OFFSET);
    TEST_ASSERT_EQUAL(1, FC_BS_OFFSET);
    TEST_ASSERT_EQUAL(2, FC_ST_OFFSET);
    TEST_ASSERT_EQUAL(3, FC_PADDING_OFFSET);
}

/* Test ISOTP separation time ranges */
void test_isotp_separation_time_ranges(void)
{
    /* Test millisecond range */
    TEST_ASSERT_EQUAL(0, ST_MS_RANGE_MIN);
    TEST_ASSERT_EQUAL(0x7F, ST_MS_RANGE_MAX);

    /* Test reserved range 1 */
    TEST_ASSERT_EQUAL(0x80, ST_RESERVED1_RANGE_MIN);
    TEST_ASSERT_EQUAL(0xF0, ST_RESERVED1_RANGE_MAX);

    /* Test microsecond range */
    TEST_ASSERT_EQUAL(0xF1, ST_US_RANGE_MIN);
    TEST_ASSERT_EQUAL(0xF9, ST_US_RANGE_MAX);

    /* Test reserved range 2 */
    TEST_ASSERT_EQUAL(0xFA, ST_RESERVED2_RANGE_MIN);
    TEST_ASSERT_EQUAL(0xFF, ST_RESERVED2_RANGE_MAX);
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

    RUN_TEST(test_isotp_fill_sf);

    RUN_TEST(test_isotp_fill_ff);

    RUN_TEST(test_isotp_fill_cf);

    RUN_TEST(test_isotp_fill_fc);

    RUN_TEST(test_isotp_get_sf_dl);

    RUN_TEST(test_isotp_get_ff_dl);

    RUN_TEST(test_isotp_remove_buffer);

    RUN_TEST(test_isotp_remove_buffer_null_args);

    RUN_TEST(test_isotp_timeout_error_handler_output_flow);

    RUN_TEST(test_isotp_timeout_error_handler_input_flow);

    RUN_TEST(test_isotp_timeout_error_handler_null_arg);

    RUN_TEST(test_isotp_states);

    RUN_TEST(test_isotp_frame_types);

    RUN_TEST(test_isotp_flow_control);

    RUN_TEST(test_isotp_separation_time_ranges);

    return UNITY_END();
}
