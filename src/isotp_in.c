#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"

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
    if ((data[FD_FF_FLAG_OFFSET] & FD_FF_FLAG_MASK) == FD_FF_FLAG)
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
    if (flow->remaining_data < (uint8_t)(length - CF_DATA_OFFSET))
    {
        lwcan_buffer_copy_to_offset(flow->buffer, (data + CF_DATA_OFFSET), flow->remaining_data, (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= flow->remaining_data;
    }
    else
    {
        lwcan_buffer_copy_to_offset(flow->buffer, (data + CF_DATA_OFFSET), (length - CF_DATA_OFFSET), (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= (length - CF_DATA_OFFSET);
    }
}

void isotp_in_flow_output(void *arg)
{
    struct isotp_pcb *pcb;

    struct canif *canif;

    lwcanerr_t ret;

    uint8_t frame_type;

#if ISOTP_CANFD
    struct canfd_frame frame;
#else
    struct can_frame frame;
#endif

    if (arg == NULL)
    {
        return;
    }

    pcb = (struct isotp_pcb *)arg;

    canif = canif_get_by_index(pcb->if_index);

    if (canif == NULL)
    {
        return;
    }

    frame.can_id = pcb->tx_id;

#if ISOTP_CANFD
    if (ISOTP_CANFD_BRS)
    {
        frame.flags = CANFD_BRS;
    }
#endif

    switch (pcb->input_flow.state)
    {
        case ISOTP_TX_FC:
            isotp_fill_fc(&pcb->input_flow, &frame);
            frame_type = FC;
            break;

        default:
            return;
    }

    ret = canif->output(canif, &frame);

    lwcan_untimeout(isotp_input_timeout_error_handler, pcb);

    if (ret == ERROR_OK)
    {
        isotp_sent(pcb, frame_type);

        return;
    }

    isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

    pcb->input_flow.state = ISOTP_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb->callback_arg, ret);
    }
}

static void received_sf(struct isotp_pcb *pcb, void *frame)
{
    uint8_t length;

    struct lwcan_buffer *buffer;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    if (pcb->input_flow.state != ISOTP_IDLE)
    {
        return;
    }

    length = isotp_get_sf_dl(_frame->data);

    buffer = lwcan_buffer_new(length);

    if (buffer == NULL)
    {
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
}

static void received_ff(struct isotp_pcb *pcb, void *frame)
{
    uint32_t length;

    struct lwcan_buffer *buffer;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    if (pcb->input_flow.state != ISOTP_IDLE)
    {
        return;
    }

    length = isotp_get_ff_dl(_frame->data);

    buffer = lwcan_buffer_new(length);

    if (buffer == NULL)
    {
        pcb->input_flow.fs = FS_OVERFLOW;

        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_MEMORY);
        }

        goto output;
    }

    buffer->next = pcb->input_flow.buffer;

    pcb->input_flow.buffer = buffer;

    pcb->input_flow.remaining_data = length;

    store_ff_data(&pcb->input_flow, _frame->data, _frame->len);

    pcb->input_flow.cf_sn = 1;

    pcb->input_flow.fs = FS_READY;

output:
    pcb->input_flow.bs = ISOTP_RECEIVE_BS;

    pcb->input_flow.st = ISOTP_RECEIVE_ST;

    pcb->input_flow.state = ISOTP_TX_FC;

    lwcan_timeout(ISOTP_N_BR, isotp_input_timeout_error_handler, pcb);

    isotp_in_flow_output(pcb);
}

static void received_cf(struct isotp_pcb *pcb, void *frame)
{
    uint8_t sn;

    int8_t cf_left;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    if (pcb->input_flow.state != ISOTP_WAIT_CF)
    {
        return;
    }

    lwcan_untimeout(isotp_input_timeout_error_handler, pcb);

    sn = _frame->data[CF_SN_OFFSET] & CF_SN_MASK;

    if (pcb->input_flow.cf_sn != sn)
    {
        isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

        pcb->input_flow.fs = FS_OVERFLOW;

        goto output;
    }

    store_cf_data(&pcb->input_flow, _frame->data, _frame->len);

    if (pcb->input_flow.remaining_data == 0)
    {
        pcb->input_flow.state = ISOTP_IDLE;

        if (pcb->receive != NULL)
        {
            pcb->receive(pcb->callback_arg, pcb, pcb->input_flow.buffer);
        }

        return;
    }

    cf_left = pcb->input_flow.bs - pcb->input_flow.cf_sn;

    pcb->input_flow.cf_sn += 1;

    if (pcb->input_flow.cf_sn > CF_SN_MASK)
    {
        pcb->input_flow.cf_sn = 0;
    }

    if (pcb->input_flow.bs == 0 || cf_left > 0)
    {
        pcb->input_flow.state = ISOTP_WAIT_CF;

        lwcan_timeout(ISOTP_N_CR, isotp_input_timeout_error_handler, pcb);

        return;
    }

    pcb->input_flow.fs = FS_READY;

output:
    pcb->input_flow.state = ISOTP_TX_FC;

    lwcan_timeout(ISOTP_N_BR, isotp_input_timeout_error_handler, pcb);

    isotp_in_flow_output(pcb);
}

static void received_fc(struct isotp_pcb *pcb, void *frame)
{
#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    if (pcb->output_flow.state != ISOTP_WAIT_FC)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    pcb->output_flow.fs = _frame->data[FC_FS_OFFSET] & FC_FS_MASK;

    if (pcb->output_flow.fs == FS_WAIT && pcb->output_flow.n_wft != 0)
    {
        pcb->output_flow.n_wft -= 1;

        lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);

        return;
    }

    if (pcb->output_flow.fs != FS_READY)
    {
        isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

        pcb->output_flow.state = ISOTP_IDLE;

        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_ABORTED);
        }

        return;
    }

    pcb->output_flow.bs = _frame->data[FC_BS_OFFSET];

    pcb->output_flow.st = _frame->data[FC_ST_OFFSET];

    if (pcb->output_flow.st > ST_MS_RANGE_MAX)
    {
        pcb->output_flow.st = 1;
    }

    pcb->output_flow.state = ISOTP_TX_CF;

    lwcan_timeout(ISOTP_N_CS, isotp_output_timeout_error_handler, pcb);

    isotp_out_flow_output(pcb);
}

void isotp_input(struct canif *canif, void *frame)
{
    struct isotp_pcb *pcb;

#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    uint8_t if_index;

    if (canif == NULL || frame == NULL)
    {
        return;
    }

    if_index = canif_get_index(canif);

    if (if_index == 0)
    {
        return;
    }

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
        return ERROR_ARG;
    }

    isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

    return ERROR_OK;
}

#endif
