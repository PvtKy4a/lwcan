#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"

#include <stdlib.h>
#include <string.h>

static void store_frame_data(struct isotp_flow *flow, struct lwcan_frame *frame, uint8_t frame_type)
{
    switch (frame_type)
    {
    case SF:
        lwcan_buffer_copy_to(flow->buffer, frame->data + SF_DATA_OFFSET, flow->remaining_data);

        break;

    case FF:
        lwcan_buffer_copy_to(flow->buffer, frame->data + FF_DATA_OFFSET, FF_DATA_LENGTH);

        flow->remaining_data -= FF_DATA_LENGTH;

        break;

    case CF:
        if (flow->remaining_data < CF_DATA_LENGTH)
        {
            lwcan_buffer_copy_to_offset(flow->buffer, frame->data + CF_DATA_OFFSET, flow->remaining_data, flow->buffer->length - flow->remaining_data);

            flow->remaining_data -= flow->remaining_data;
        }
        else
        {
            lwcan_buffer_copy_to_offset(flow->buffer, frame->data + CF_DATA_OFFSET, CF_DATA_LENGTH, flow->buffer->length - flow->remaining_data);

            flow->remaining_data -= CF_DATA_LENGTH;
        }

        break;

    default:
        break;
    }
}

static void received_sf(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    uint16_t length;

    struct lwcan_buffer *buffer;

    if (pcb->input_flow.state != ISOTP_STATE_IDLE)
    {
        return;
    }

    length = get_frame_data_length(frame);

    buffer = lwcan_buffer_malloc(length);

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

    store_frame_data(&pcb->input_flow, frame, SF);

    if (pcb->receive != NULL)
    {
        pcb->receive(pcb->callback_arg, pcb, pcb->input_flow.buffer, ERROR_OK);
    }
}

static void received_ff(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    uint16_t length;

    struct lwcan_buffer *buffer;

    if (pcb->input_flow.state != ISOTP_STATE_IDLE)
    {
        return;
    }

    length = get_frame_data_length(frame);

    buffer = lwcan_buffer_malloc(length);

    if (buffer == NULL)
    {
        pcb->input_flow.fs = FS_OVERFLOW;

        goto output;
    }

    buffer->next = pcb->input_flow.buffer;

    pcb->input_flow.buffer = buffer;

    pcb->input_flow.remaining_data = length;

    store_frame_data(&pcb->input_flow, frame, FF);

    pcb->input_flow.cf_sn = 1;

    pcb->input_flow.fs = FS_READY;

output:
    pcb->input_flow.bs = ISOTP_RECEIVE_BLOCK_SIZE;

    pcb->input_flow.st = ISOTP_RECEIVE_SEPARATION_TIME;

    pcb->input_flow.state = ISOTP_STATE_TX_FC;

    isotp_output(pcb);

    if (pcb->input_flow.fs != FS_READY)
    {
        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_MEMORY);
        }
    }
}

static void received_cf(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    uint8_t sn;

    int8_t cf_left;

    lwcanerr_t error;

    if (pcb->input_flow.state != ISOTP_STATE_WAIT_CF)
    {
        return;
    }

    lwcan_untimeout(isotp_input_timeout_error_handler, pcb);

    sn = get_frame_serial_number(frame);

    if (pcb->input_flow.cf_sn != sn)
    {
        isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

        error = ERROR_ABORTED;

        pcb->input_flow.fs = FS_OVERFLOW;

        goto send_fc;
    }

    store_frame_data(&pcb->input_flow, frame, CF);

    if (pcb->input_flow.remaining_data == 0)
    {
        pcb->input_flow.state = ISOTP_STATE_IDLE;

        if (pcb->receive != NULL)
        {
            pcb->receive(pcb->callback_arg, pcb, pcb->input_flow.buffer, ERROR_OK);
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

send_fc:
    pcb->input_flow.state = ISOTP_STATE_TX_FC;

    isotp_output(pcb);

    if (pcb->input_flow.fs != FS_READY)
    {
        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, error);
        }
    }
    
}

static void received_fc(struct isotp_pcb *pcb, struct lwcan_frame *frame)
{
    lwcanerr_t error;

    if (pcb->output_flow.state != ISOTP_STATE_WAIT_FC)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    pcb->output_flow.fs = get_frame_flow_status(frame);

    if (pcb->output_flow.fs == FS_OVERFLOW)
    {
        error = ERROR_ABORTED;

        goto error_exit;
    }

    if (pcb->output_flow.fs == FS_WAIT)
    {
        if (ISOTP_FC_WAIT_ALLOW)
        {
            goto wait_next;
        }
        else
        {
            error = ERROR_ABORTED;

            goto error_exit;
        }
    }

    pcb->output_flow.bs = get_frame_block_size(frame);

    pcb->output_flow.st = get_frame_separation_time(frame);

    pcb->output_flow.state = ISOTP_STATE_TX_CF;

    isotp_output(pcb);

    return;

wait_next:
    pcb->output_flow.state = ISOTP_STATE_WAIT_FC;

    lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);

    return;

error_exit:
    isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

    pcb->output_flow.state = ISOTP_STATE_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb->callback_arg, error);
    }
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

    frame_type = get_frame_type(frame);

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

#endif
