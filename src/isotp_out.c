#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"
#include "lwcan/memory.h"

#include <stdlib.h>
#include <string.h>

static void out_flow_output(void *arg)
{
    struct isotp_pcb *pcb;

    struct canif *canif;

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

    switch (pcb->output_flow.state)
    {
        case ISOTP_STATE_TX_SF:
            frame_type = SF;
            break;

        case ISOTP_STATE_TX_FF:
            frame_type = FF;
            break;

        case ISOTP_STATE_TX_CF:
            frame_type = CF;
            break;

        default:
            return;
    }

    isotp_fill_frame(&pcb->output_flow, frame_type, pcb->output_id, pcb->extended_id);

    lwcan_timeout(ISOTP_N_AS, isotp_output_timeout_error_handler, pcb);

    if (canif->output(canif, &pcb->output_flow.frame) != ERROR_OK)
    {
        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_IF);
        }
    }
}

lwcanerr_t isotp_send(struct isotp_pcb *pcb, const uint8_t *data, uint16_t length)
{
    struct lwcan_buffer *buffer;

    if (pcb == NULL || data == NULL || length == 0)
    {
        return ERROR_ARG;
    }

    if (pcb->output_flow.state != ISOTP_STATE_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    buffer = lwcan_buffer_malloc(length);

    if (buffer == NULL)
    {
        return ERROR_MEMORY;
    }

    buffer->next = pcb->output_flow.buffer;

    pcb->output_flow.buffer = buffer;

    lwcan_buffer_copy_to(pcb->output_flow.buffer, data, length);

    pcb->output_flow.remaining_data = length;

    if (pcb->output_flow.remaining_data > SF_DATA_LENGTH)
    {
        pcb->output_flow.state = ISOTP_STATE_TX_FF;

        pcb->output_flow.cf_sn = 1;
    }
    else
    {
        pcb->output_flow.state = ISOTP_STATE_TX_SF;
    }

    out_flow_output(pcb);

    return ERROR_OK;
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
            lwcan_timeout(pcb->output_flow.st, out_flow_output, pcb);
        }
        else
        {
            out_flow_output(pcb);
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
