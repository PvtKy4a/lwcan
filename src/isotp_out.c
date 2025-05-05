#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"
#include "lwcan/debug.h"

#include <string.h>

static void sent_sf(struct isotp_pcb *pcb)
{
    uint32_t length;

    length = pcb->output_flow.buffer->length;

    isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

    pcb->output_flow.state = ISOTP_IDLE;

    if (pcb->sent != NULL)
    {
        pcb->sent(pcb->callback_arg, pcb, length);
    }
    
}

static void sent_ff(struct isotp_pcb *pcb)
{
    if (pcb->output_flow.state != ISOTP_TX_FF)
    {
        return;
    }

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    pcb->output_flow.state = ISOTP_WAIT_FC;

    lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);
}

static void sent_cf(struct isotp_pcb *pcb)
{
    uint32_t length;

    int8_t cf_left;

    if (pcb->output_flow.remaining_data == 0)
    {
        length = pcb->output_flow.buffer->length;

        isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

        pcb->output_flow.state = ISOTP_IDLE;

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
        pcb->output_flow.state = ISOTP_TX_CF;

        lwcan_timeout(ISOTP_N_CS + pcb->output_flow.st, isotp_output_timeout_error_handler, pcb);

        lwcan_timeout(pcb->output_flow.st, isotp_out_flow_output, pcb);
    }
    else
    {
        pcb->output_flow.state = ISOTP_WAIT_FC;

        lwcan_timeout(ISOTP_N_BS, isotp_output_timeout_error_handler, pcb);
    }
}

static void sent_fc(struct isotp_pcb *pcb)
{
    if (pcb->input_flow.fs != FS_READY)
    {
        pcb->input_flow.state = ISOTP_IDLE;

        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_ABORTED);
        }

        return;
    }

    pcb->input_flow.state = ISOTP_WAIT_CF;

    lwcan_timeout(ISOTP_N_CR, isotp_input_timeout_error_handler, pcb);
}

void isotp_sent(struct isotp_pcb *pcb, uint8_t frame_type)
{
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

void isotp_out_flow_output(void *arg)
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
        LWCAN_ASSERT("arg != NULL", arg != NULL);

        return;
    }

    pcb = (struct isotp_pcb *)arg;

    canif = canif_get_by_index(pcb->if_index);

    if (canif == NULL)
    {
        LWCAN_ASSERT("canif != NULL", canif != NULL);

        return;
    }

    frame.can_id = pcb->tx_id;

#if ISOTP_CANFD
    if (ISOTP_CANFD_BRS)
    {
        frame.flags = CANFD_BRS;
    }
#endif

    switch (pcb->output_flow.state)
    {
        case ISOTP_TX_SF:
            isotp_fill_sf(&pcb->output_flow, &frame);
            frame_type = SF;
            break;

        case ISOTP_TX_FF:
            isotp_fill_ff(&pcb->output_flow, &frame);
            frame_type = FF;
            break;

        case ISOTP_TX_CF:
            isotp_fill_cf(&pcb->output_flow, &frame);
            frame_type = CF;
            break;

        default:
            return;
    }

    ret = canif->output(canif, &frame, sizeof(frame));

    lwcan_untimeout(isotp_output_timeout_error_handler, pcb);

    if (ret == ERROR_OK)
    {
        isotp_sent(pcb, frame_type);

        return;
    }

    LWCAN_ASSERT("ret == 0", ret == 0);

    isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

    pcb->output_flow.state = ISOTP_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb->callback_arg, ret);
    }
}

lwcanerr_t isotp_send(struct isotp_pcb *pcb, const uint8_t *data, uint32_t length)
{
    struct lwcan_buffer *buffer;

    if (pcb == NULL || data == NULL || length == 0)
    {
        LWCAN_ASSERT("pcb != NULL", pcb != NULL);
        LWCAN_ASSERT("data != NULL", data != NULL);
        LWCAN_ASSERT("length != 0", length != 0);

        return ERROR_ARG;
    }

    if (pcb->output_flow.state != ISOTP_IDLE)
    {
        return ERROR_INPROGRESS;
    }

    buffer = lwcan_buffer_new(length);

    if (buffer == NULL)
    {
        LWCAN_ASSERT("buffer != NULL", buffer != NULL);

        return ERROR_MEMORY;
    }

    lwcan_buffer_copy_to(buffer, data, length);

    buffer->next = pcb->output_flow.buffer;

    pcb->output_flow.buffer = buffer;

    pcb->output_flow.remaining_data = length;

#if ISOTP_CANFD
    if (pcb->output_flow.remaining_data > (CANFD_MAX_DLEN - FD_SF_DATA_OFFSET))
    {
        pcb->output_flow.state = ISOTP_TX_FF;

        pcb->output_flow.cf_sn = 1;

        pcb->output_flow.n_wft = ISOTP_MAX_N_WFT;
    }
    else
    {
        pcb->output_flow.state = ISOTP_TX_SF;
    }
#else
    if (pcb->output_flow.remaining_data > (CAN_MAX_DLEN - SF_DATA_OFFSET))
    {
        pcb->output_flow.state = ISOTP_TX_FF;

        pcb->output_flow.cf_sn = 1;

        pcb->output_flow.n_wft = ISOTP_N_WFT;
    }
    else
    {
        pcb->output_flow.state = ISOTP_TX_SF;
    }
#endif

    lwcan_timeout(ISOTP_N_AS, isotp_output_timeout_error_handler, pcb);

    isotp_out_flow_output(pcb);

    return ERROR_OK;
}

#endif
