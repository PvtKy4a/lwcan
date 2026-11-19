#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"
#include "lwcan/debug.h"

#include <string.h>

static void store_sf_data(struct isotp_flow *flow, uint8_t *data, uint8_t length)
{
    (void)length;

#if ISOTP_CANFD
    if ((data[FD_SF_FLAG_OFFSET] & FD_SF_FLAG_MASK) == FD_SF_FLAG)
    {
        lwcan_buffer_copy_to(flow->buffer, (data + FD_SF_DATA_OFFSET), flow->remaining_data);
    }
    else
#endif
    {
        lwcan_buffer_copy_to(flow->buffer, (data + SF_DATA_OFFSET), flow->remaining_data);
    }
}

static void store_ff_data(struct isotp_flow *flow, uint8_t *data, uint8_t length)
{
#if ISOTP_CANFD
    if (((data[FF_DL_HI_OFFSET] & FF_DL_HI_MASK) == 0) && (data[FF_DL_LO_OFFSET] == 0))
    {
        lwcan_buffer_copy_to(flow->buffer, (data + FD_FF_DATA_OFFSET), (length - FD_FF_DATA_OFFSET));

        flow->remaining_data -= (length - FD_FF_DATA_OFFSET);
    }
    else
#endif
    {
        lwcan_buffer_copy_to(flow->buffer, (data + FF_DATA_OFFSET), (length - FF_DATA_OFFSET));

        flow->remaining_data -= (length - FF_DATA_OFFSET);
    }
}

static void store_cf_data(struct isotp_flow *flow, uint8_t *data, uint8_t length)
{
    if (flow->remaining_data < (uint32_t)(length - CF_DATA_OFFSET))
    {
        lwcan_buffer_copy_to_offset(flow->buffer, (data + CF_DATA_OFFSET), flow->remaining_data, (flow->buffer->length - flow->remaining_data));

        flow->remaining_data = 0;
    }
    else
    {
        lwcan_buffer_copy_to_offset(flow->buffer, (data + CF_DATA_OFFSET), (length - CF_DATA_OFFSET), (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= (length - CF_DATA_OFFSET);
    }
}

static void received_sf(struct isotp_pcb *pcb, void *frame)
{
    uint8_t length, data_offset;

    struct lwcan_buffer *buffer;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;

    if ((_frame->data[FD_SF_FLAG_OFFSET] & FD_SF_FLAG_MASK) == FD_SF_FLAG)
    {
        data_offset = FD_SF_DATA_OFFSET;
    }
    else
    {
        data_offset = SF_DATA_OFFSET;
    }
#else
    struct can_frame *_frame = (struct can_frame *)frame;

    data_offset = SF_DATA_OFFSET;
#endif

    if (pcb->input_flow.state != ISOTP_IDLE)
    {
        return;
    }

    length = isotp_get_sf_dl(_frame->data);

    if (length == 0 || length > (_frame->len - data_offset))
    {
        return;
    }

    buffer = lwcan_buffer_new(length);

    if (buffer == NULL)
    {
        pcb->input_flow.state = ISOTP_IDLE;

        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_MEMORY);
        }

        return;
    }

    buffer->next = pcb->input_flow.buffer;

    pcb->input_flow.buffer = buffer;

    pcb->input_flow.remaining_data = length;

    store_sf_data(&pcb->input_flow, _frame->data, _frame->len);

    if (pcb->receive != NULL)
    {
        pcb->receive(pcb->callback_arg, pcb, pcb->input_flow.buffer);
    }
    else
    {
        isotp_received(pcb, pcb->input_flow.buffer);
    }
}

static void received_ff(struct isotp_pcb *pcb, void *frame)
{
    uint32_t length;

    uint8_t data_offset;

    struct lwcan_buffer *buffer;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;

    if (((_frame->data[FF_DL_HI_OFFSET] & FF_DL_HI_MASK) == 0) && (_frame->data[FF_DL_LO_OFFSET] == 0))
    {
        data_offset = FD_FF_DATA_OFFSET;
    }
    else
    {
        data_offset = FF_DATA_OFFSET;
    }
#else
    struct can_frame *_frame = (struct can_frame *)frame;

    data_offset = FF_DATA_OFFSET;
#endif

    if (pcb->input_flow.state != ISOTP_IDLE)
    {
        return;
    }

    length = isotp_get_ff_dl(_frame->data);

    if (length <= (uint32_t)(_frame->len - data_offset))
    {
        return;
    }

    pcb->input_flow.state = ISOTP_TX_FC;

    buffer = lwcan_buffer_new(length);

    if (buffer == NULL)
    {
        pcb->input_flow.fs = FS_OVERFLOW;

        goto output;
    }

    if (pcb->receive_ff != NULL)
    {
        pcb->receive_ff(pcb->callback_arg, pcb);
    }

    buffer->next = pcb->input_flow.buffer;

    pcb->input_flow.buffer = buffer;

    pcb->input_flow.remaining_data = length;

    store_ff_data(&pcb->input_flow, _frame->data, _frame->len);

    pcb->input_flow.cf_count = 0;

    pcb->input_flow.cf_sn = 1;

    pcb->input_flow.fs = FS_READY;

output:
    pcb->input_flow.bs = ISOTP_RECEIVE_BS;

    pcb->input_flow.st = ISOTP_RECEIVE_ST;

    lwcan_timeout(&pcb->input_flow.output_timeout, 0, isotp_output, &pcb->input_flow);
}

static void received_cf(struct isotp_pcb *pcb, void *frame)
{
    uint8_t sn;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    if (pcb->input_flow.state != ISOTP_WAIT_CF)
    {
        return;
    }

    lwcan_untimeout(&pcb->input_flow.error_timeout);

    sn = _frame->data[CF_SN_OFFSET] & CF_SN_MASK;

    if (pcb->input_flow.cf_sn != sn || _frame->len <= CF_DATA_OFFSET)
    {
        isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

        pcb->input_flow.state = ISOTP_IDLE;

        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_SEQUENCE);
        }

        return;
    }

    store_cf_data(&pcb->input_flow, _frame->data, _frame->len);

    if (pcb->input_flow.remaining_data == 0)
    {
        pcb->input_flow.state = ISOTP_IDLE;

        if (pcb->receive != NULL)
        {
            pcb->receive(pcb->callback_arg, pcb, pcb->input_flow.buffer);
        }
        else
        {
            isotp_received(pcb, pcb->input_flow.buffer);
        }

        return;
    }

    pcb->input_flow.cf_count += 1;

    pcb->input_flow.cf_sn += 1;

    if (pcb->input_flow.cf_sn > CF_SN_MASK)
    {
        pcb->input_flow.cf_sn = 0;
    }

    if (pcb->input_flow.bs == 0 || pcb->input_flow.cf_count < pcb->input_flow.bs)
    {
        pcb->input_flow.state = ISOTP_WAIT_CF;

        lwcan_timeout(&pcb->input_flow.error_timeout, ISOTP_N_CR, isotp_timeout_error_handler, &pcb->input_flow);

        return;
    }

    pcb->input_flow.fs = FS_READY;

    pcb->input_flow.cf_count = 0;

    pcb->input_flow.state = ISOTP_TX_FC;

    lwcan_timeout(&pcb->input_flow.output_timeout, 0, isotp_output, &pcb->input_flow);
}

static void received_fc(struct isotp_pcb *pcb, void *frame)
{
    lwcanerr_t err;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    if (pcb->output_flow.state != ISOTP_WAIT_FC)
    {
        return;
    }

    lwcan_untimeout(&pcb->output_flow.error_timeout);

    if (_frame->len < FC_PADDING_OFFSET)
    {
        err = ERROR_ABORTED;

        goto error;
    }

    pcb->output_flow.fs = _frame->data[FC_FS_OFFSET] & FC_FS_MASK;

    if (pcb->output_flow.fs == FS_WAIT && pcb->output_flow.n_wft != 0)
    {
        pcb->output_flow.n_wft -= 1;

        lwcan_timeout(&pcb->output_flow.error_timeout, ISOTP_N_BS, isotp_timeout_error_handler, &pcb->output_flow);

        return;
    }

    if (pcb->output_flow.fs != FS_READY)
    {
        err = ERROR_ABORTED;

        goto error;
    }

    pcb->output_flow.bs = _frame->data[FC_BS_OFFSET];

    pcb->output_flow.st = _frame->data[FC_ST_OFFSET];

    if (pcb->output_flow.st > ST_MS_RANGE_MAX)
    {
        pcb->output_flow.st = 1;
    }

    pcb->output_flow.state = ISOTP_TX_CF;

    lwcan_timeout(&pcb->output_flow.output_timeout, 0, isotp_output, &pcb->output_flow);

    return;

error:
    isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

    pcb->output_flow.state = ISOTP_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb->callback_arg, err);
    }
}

void isotp_input(struct canif *canif, void *frame)
{
    uint8_t if_index;

    struct isotp_pcb *pcb;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
    if (_frame->len == 0 || _frame->len > CANFD_MAX_DLEN)
#else
    struct can_frame *_frame = (struct can_frame *)frame;
    if (_frame->len == 0 || _frame->len > CAN_MAX_DLEN)
#endif
    {
        return;
    }

    if_index = canif_get_index(canif);

    pcb = isotp_get_pcb_list();

    while (pcb != NULL)
    {
        if (pcb->if_index == if_index && pcb->rx_id == _frame->can_id)
        {
            break;
        }
        else
        {
            pcb = pcb->next;
        }
    }

    if (pcb == NULL)
    {
        return;
    }

    if (_frame->can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG))
    {
        return;
    }

    switch (_frame->data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK)
    {
    case SF:
        received_sf(pcb, frame);
        break;

    case FF:
        received_ff(pcb, frame);
        break;

    case CF:
        received_cf(pcb, frame);
        break;

    case FC:
        received_fc(pcb, frame);
        break;

    default:
        break;
    }
}

lwcanerr_t isotp_received(struct isotp_pcb *pcb, struct lwcan_buffer *buffer)
{
    if (pcb == NULL || buffer == NULL)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);
        LWCAN_ASSERT("buffer != NULL", buffer != NULL);

        return ERROR_ARG;
    }

    isotp_remove_buffer(&pcb->input_flow, buffer);

    return ERROR_OK;
}

#endif
