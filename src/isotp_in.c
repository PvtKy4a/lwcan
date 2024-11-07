#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"

#include <stdlib.h>
#include <string.h>

static void store_sf_data(struct isotp_flow *flow, struct lwcan_frame *frame)
{
    uint8_t offset;

    if ((frame->data[FD_SF_FLAG_OFFSET] & FD_SF_FLAG_MASK) != FD_SF_FLAG)
    {
        offset = SF_DATA_OFFSET;
    }
    else
    {
        offset = FD_SF_DATA_OFFSET;
    }

    lwcan_buffer_copy_to(flow->buffer, (frame->data + offset), flow->remaining_data);
}

static void store_ff_data(struct isotp_flow *flow, struct lwcan_frame *frame)
{
    uint8_t offset;

    uint8_t length = lwcan_dlc_to_length(frame->dlc);

    if ((frame->data[FD_FF_FLAG_OFFSET] & FD_FF_FLAG_MASK) != FD_FF_FLAG)
    {
        offset = FF_DATA_OFFSET;
    }
    else
    {
        offset = FD_FF_DATA_OFFSET;
    }

    lwcan_buffer_copy_to(flow->buffer, (frame->data + offset), (length - offset));

    flow->remaining_data -= (length - offset);
}

static void store_cf_data(struct isotp_flow *flow, struct lwcan_frame *frame)
{
    uint8_t offset;

    uint8_t length = lwcan_dlc_to_length(frame->dlc);

    offset = CF_DATA_OFFSET;

    if (flow->remaining_data < (uint8_t)(length - offset))
    {
        lwcan_buffer_copy_to_offset(flow->buffer, (frame->data + offset), flow->remaining_data, (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= flow->remaining_data;
    }
    else
    {
        lwcan_buffer_copy_to_offset(flow->buffer, (frame->data + offset), (length - offset), (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= (length - offset);
    }
}

void isotp_in_flow_output(void *arg)
{
    struct isotp_pcb *pcb;

    struct canif *canif;

    lwcanerr_t ret;

    uint8_t frame_type;

    if (arg == NULL)
    {
        return;
    }

    pcb = (struct isotp_pcb *)arg;

    canif = canif_get_by_index(pcb->canif_index);

    if (canif == NULL)
    {
        return;
    }

    switch (pcb->input_flow.state)
    {
        case ISOTP_STATE_TX_FC:
            isotp_fill_fc(&pcb->input_flow, pcb->output_id, pcb->extended_id);
            frame_type = FC;
            break;

        default:
            return;
    }

    ret = canif->output(canif, &pcb->input_flow.frame);

    lwcan_untimeout(isotp_input_timeout_error_handler, pcb);

    if (ret == ERROR_OK)
    {
        isotp_sent(pcb, frame_type);

        return;
    }

    isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

    pcb->input_flow.state = ISOTP_STATE_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb->callback_arg, ERROR_IF);
    }
}

static void received_sf(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    uint8_t length;

    struct lwcan_buffer *buffer;

    if (pcb->input_flow.state != ISOTP_STATE_IDLE)
    {
        return;
    }

    length = isotp_get_sf_data_length(frame);

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

    store_sf_data(&pcb->input_flow, frame);

    if (pcb->receive != NULL)
    {
        pcb->receive(pcb->callback_arg, pcb, pcb->input_flow.buffer);
    }
}

static void received_ff(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    uint32_t length;

    struct lwcan_buffer *buffer;

    if (pcb->input_flow.state != ISOTP_STATE_IDLE)
    {
        return;
    }

    length = isotp_get_ff_data_length(frame);

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

    store_ff_data(&pcb->input_flow, frame);

    pcb->input_flow.cf_sn = 1;

    pcb->input_flow.fs = FS_READY;

output:
    pcb->input_flow.bs = ISOTP_RECEIVE_BLOCK_SIZE;

    pcb->input_flow.st = ISOTP_RECEIVE_SEPARATION_TIME;

    pcb->input_flow.state = ISOTP_STATE_TX_FC;

    lwcan_timeout(ISOTP_N_BR, isotp_input_timeout_error_handler, pcb);

    lwcan_timeout(0, isotp_in_flow_output, pcb);
}

static void received_cf(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    uint8_t sn;

    int8_t cf_left;

    if (pcb->input_flow.state != ISOTP_STATE_WAIT_CF)
    {
        return;
    }

    lwcan_untimeout(isotp_input_timeout_error_handler, pcb);

    sn = isotp_get_frame_serial_number(frame);

    if (pcb->input_flow.cf_sn != sn)
    {
        isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

        pcb->input_flow.fs = FS_OVERFLOW;

        goto output;
    }

    store_cf_data(&pcb->input_flow, frame);

    if (pcb->input_flow.remaining_data == 0)
    {
        pcb->input_flow.state = ISOTP_STATE_IDLE;

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
        pcb->input_flow.state = ISOTP_STATE_WAIT_CF;

        lwcan_timeout(ISOTP_N_CR, isotp_input_timeout_error_handler, pcb);

        return;
    }

    pcb->input_flow.fs = FS_READY;

output:
    pcb->input_flow.state = ISOTP_STATE_TX_FC;

    lwcan_timeout(ISOTP_N_BR, isotp_input_timeout_error_handler, pcb);

    lwcan_timeout(0, isotp_in_flow_output, pcb);
}

static void received_fc(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    if (pcb->output_flow.state != ISOTP_STATE_WAIT_FC)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    pcb->output_flow.fs = isotp_get_frame_flow_status(frame);

    if (pcb->output_flow.fs == FS_WAIT && ISOTP_FC_WAIT_ALLOW)
    {
        pcb->output_flow.state = ISOTP_STATE_WAIT_FC;

        lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);

        return;
    }

    if (pcb->output_flow.fs != FS_READY)
    {
        isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

        pcb->output_flow.state = ISOTP_STATE_IDLE;

        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_ABORTED);
        }

        return;
    }

    pcb->output_flow.bs = isotp_get_frame_block_size(frame);

    pcb->output_flow.st = isotp_get_frame_separation_time(frame);

    pcb->output_flow.state = ISOTP_STATE_TX_CF;

    lwcan_timeout(ISOTP_N_CS, isotp_output_timeout_error_handler, pcb);

    lwcan_timeout(0, isotp_out_flow_output, pcb);
}

void isotp_input(struct canif *canif, struct lwcan_frame *frame)
{
    struct isotp_pcb *pcb;

    uint8_t canif_index;
    
    uint8_t frame_type;

    if (canif == NULL || frame == NULL)
    {
        return;
    }

    canif_index = canif_get_index(canif);

    if (canif_index == 0)
    {
        return;
    }

    pcb = get_isotp_pcb_list();

    while (pcb != NULL)
    {
        if (pcb->canif_index == canif_index && pcb->input_id == frame->id)
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

    frame_type = isotp_get_frame_type(frame);

    switch (frame_type)
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
