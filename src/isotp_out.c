#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"

#include <stdlib.h>
#include <string.h>

static void add_padding(uint8_t *data, uint8_t num)
{
    for (uint8_t i = 0; i < num; i++)
    {
        data[i] = ISOTP_PADDING_BYTE;
    }
}

static void fill_sf(struct isotp_flow *flow)
{
    flow->frame.data[FRAME_TYPE_OFFSET] = SF & FRAME_TYPE_MASK;

    flow->frame.data[SF_DL_OFFSET] |= flow->buffer->length & SF_DL_MASK;

    lwcan_buffer_copy_from(flow->buffer, flow->frame.data + SF_DATA_OFFSET, flow->remaining_data);

    if (flow->remaining_data < SF_DATA_LENGTH)
    {
        add_padding(flow->frame.data + SF_DATA_OFFSET + flow->remaining_data, SF_DATA_LENGTH - flow->remaining_data);
    }

    flow->remaining_data -= flow->remaining_data;
}

static void fill_ff(struct isotp_flow *flow)
{
    flow->frame.data[FRAME_TYPE_OFFSET] = FF & FRAME_TYPE_MASK;

    flow->frame.data[FF_DL_HI_OFFSET] |= (uint8_t)((flow->buffer->length << 8) & FF_DL_HI_MASK);

    flow->frame.data[FF_DL_LO_OFFSET] |= (uint8_t)(flow->buffer->length & FF_DL_LO_MASK);

    lwcan_buffer_copy_from(flow->buffer, flow->frame.data + FF_DATA_OFFSET, FF_DATA_LENGTH);

    flow->remaining_data -= FF_DATA_LENGTH;
}

static void fill_cf(struct isotp_flow *flow)
{
    flow->frame.data[FRAME_TYPE_OFFSET] = CF & FRAME_TYPE_MASK;

    flow->frame.data[CF_SN_OFFSET] |= flow->cf_sn & CF_SN_MASK;

    if (flow->remaining_data < CF_DATA_LENGTH)
    {
        lwcan_buffer_copy_from_offset(flow->buffer, flow->frame.data + CF_DATA_OFFSET, flow->remaining_data, flow->buffer->length - flow->remaining_data);

        add_padding(flow->frame.data + CF_DATA_OFFSET + flow->remaining_data, CF_DATA_LENGTH - flow->remaining_data);

        flow->remaining_data -= flow->remaining_data;
    }
    else
    {
        lwcan_buffer_copy_from_offset(flow->buffer, flow->frame.data + CF_DATA_OFFSET, CF_DATA_LENGTH, flow->buffer->length - flow->remaining_data);

        flow->remaining_data -= CF_DATA_LENGTH;
    }
}

static void fill_fc(struct isotp_flow *flow)
{
    flow->frame.data[FRAME_TYPE_OFFSET] = FC & FRAME_TYPE_MASK;

    flow->frame.data[FC_FS_OFFSET] |= flow->fs & FC_FS_MASK;

    flow->frame.data[FC_BS_OFFSET] |= flow->bs;

    flow->frame.data[FC_ST_OFFSET] |= flow->st;

    add_padding(flow->frame.data + FC_PADDING_OFFSET, flow->frame.dlc - FC_PADDING_OFFSET);
}

static void fill_frame(struct isotp_flow *flow, uint8_t frame_type, uint32_t id, bool extended_id)
{
    flow->frame.id = id;

    flow->frame.extended_id = extended_id;

    flow->frame.dlc = LWCAN_DLC;

    switch (frame_type)
    {
    case SF:
        fill_sf(flow);

        break;

    case FF:
        fill_ff(flow);

        break;

    case CF:
        fill_cf(flow);

        break;

    case FC:
        fill_fc(flow);

        break;
    }
}

void isotp_output(void *arg)
{
    struct isotp_pcb *pcb;

    struct canif *canif;

    bool output_flow_ready = false;

    bool input_flow_ready = false;

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

    switch (pcb->output_flow.state)
    {
        case ISOTP_STATE_TX_SF:

            fill_frame(&pcb->output_flow, SF, pcb->output_id, pcb->extended_id);

            output_flow_ready = true;

            break;

        case ISOTP_STATE_TX_FF:

            fill_frame(&pcb->output_flow, FF, pcb->output_id, pcb->extended_id);

            output_flow_ready = true;

            break;

        case ISOTP_STATE_TX_CF:

            fill_frame(&pcb->output_flow, CF, pcb->output_id, pcb->extended_id);

            output_flow_ready = true;

            break;

        default:
            break;
    }

    if (output_flow_ready)
    {
        lwcan_timeout(ISOTP_N_AS, isotp_output_timeout_error_handler, pcb);

        if (canif->output(canif, &pcb->output_flow.frame) != ERROR_OK)
        {
            pcb->error(pcb->callback_arg, ERROR_CANIF);
        }
    }

    switch (pcb->input_flow.state)
    {
        case ISOTP_STATE_TX_FC:

            fill_frame(&pcb->input_flow, FC, pcb->output_id, pcb->extended_id);

            input_flow_ready = true;

            break;

        default:
            break;
    }

    if (input_flow_ready)
    {
        lwcan_timeout(ISOTP_N_AS, isotp_input_timeout_error_handler, pcb);

        if (canif->output(canif, &pcb->input_flow.frame) != ERROR_OK)
        {
            pcb->error(pcb->callback_arg, ERROR_CANIF);
        }
    }

}

static void sent_sf(struct isotp_pcb *pcb)
{
    uint16_t length;

    if (pcb->output_flow.state != ISOTP_STATE_TX_SF)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    length = pcb->output_flow.buffer->length;

    isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

    pcb->output_flow.state = ISOTP_STATE_IDLE;

    if (pcb->sent != NULL)
    {
        pcb->sent(pcb->callback_arg, pcb, length);
    }
    
}

static void sent_ff(struct isotp_pcb *pcb)
{
    if (pcb->output_flow.state != ISOTP_STATE_TX_FF)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    pcb->output_flow.state = ISOTP_STATE_WAIT_FC;

    lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);
}

static void sent_cf(struct isotp_pcb *pcb)
{
    uint16_t length;

    int8_t cf_left;

    if (pcb->output_flow.state != ISOTP_STATE_TX_CF)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    if (pcb->output_flow.remaining_data == 0)
    {
        length = pcb->output_flow.buffer->length;

        isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

        pcb->output_flow.state = ISOTP_STATE_IDLE;

        if (pcb->sent != NULL)
        {
            pcb->sent(pcb->callback_arg, pcb, length);
        }

        return;
    }

    cf_left = pcb->output_flow.bs - pcb->output_flow.cf_sn;

    pcb->output_flow.cf_sn += 1;

    if (pcb->output_flow.cf_sn > CF_SN_MASK)
    {
        pcb->output_flow.cf_sn = 0;
    }

    if (pcb->output_flow.bs == 0 || cf_left > 0)
    {
        pcb->output_flow.state = ISOTP_STATE_TX_CF;

        if (pcb->output_flow.st != 0)
        {
            lwcan_timeout(pcb->output_flow.st, isotp_output, pcb);
        }
        else
        {
            isotp_output(pcb);
        }
    }
    else
    {
        pcb->output_flow.state = ISOTP_STATE_WAIT_FC;

        lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);
    }
}

static void sent_fc(struct isotp_pcb *pcb)
{
    if (pcb->input_flow.state != ISOTP_STATE_TX_FC)
    {
        return;
    }

    lwcan_untimeout(isotp_input_timeout_error_handler, pcb);

    if (pcb->input_flow.fs != FS_READY)
    {
        pcb->input_flow.state = ISOTP_STATE_IDLE;

        return;
    }

    pcb->input_flow.state = ISOTP_STATE_WAIT_CF;

    lwcan_timeout(ISOTP_N_CR, isotp_input_timeout_error_handler, pcb);
}

void isotp_sent(struct canif *canif, struct lwcan_frame *frame)
{
    struct isotp_pcb *pcb;

    uint8_t canif_index;

    uint8_t frame_type;

    if (canif == NULL)
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
        if (pcb->canif_index == canif_index && (&pcb->output_flow.frame == frame || &pcb->input_flow.frame == frame))
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
        sent_sf(pcb);

        break;

    case FF:
        sent_ff(pcb);

        break;

    case CF:
        sent_cf(pcb);

        break;

    case FC:
        sent_fc(pcb);
        
        break;
    }
}

#endif
