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

    pcb->output_flow.state = ISOTP_WAIT_FC;

    lwcan_timeout(ISOTP_N_BS, isotp_output_error_handler, pcb);
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

        lwcan_timeout(pcb->output_flow.st, isotp_out_flow_output, pcb);
    }
    else
    {
        pcb->output_flow.state = ISOTP_WAIT_FC;

        lwcan_timeout(ISOTP_N_BS, isotp_output_error_handler, pcb);
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

    lwcan_timeout(ISOTP_N_CR, isotp_input_error_handler, pcb);
}

void isotp_sent(void *arg, lwcanerr_t error)
{
    struct isotp_flow *flow;

    flow = (struct isotp_flow *)arg;

    if (error != ERROR_OK)
    {
        LWCAN_ASSERT("error == ERROR_OK", error == ERROR_OK);

        isotp_remove_buffer(flow, flow->buffer);

        flow->state = ISOTP_IDLE;

        if (flow->pcb->error != NULL)
        {
            flow->pcb->error(flow->pcb->callback_arg, error);
        }
    }

    switch (flow->state)
    {
    case ISOTP_TX_SF:
        sent_sf(flow->pcb);
        break;

    case ISOTP_TX_FF:
        sent_ff(flow->pcb);
        break;

    case ISOTP_TX_CF:
        sent_cf(flow->pcb);
        break;

    case ISOTP_TX_FC:
        sent_fc(flow->pcb);
        break;
    }
}

void isotp_out_flow_output(void *arg)
{
    struct isotp_pcb *pcb;

    struct canif *canif;

    lwcanerr_t ret;

    uint32_t timeout;

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
            timeout = ISOTP_N_AS;
            break;

        case ISOTP_TX_FF:
            isotp_fill_ff(&pcb->output_flow, &frame);
            timeout = ISOTP_N_AS;
            break;

        case ISOTP_TX_CF:
            isotp_fill_cf(&pcb->output_flow, &frame);
            timeout = ISOTP_N_CS;
            break;

        default:
            return;
    }

    ret = canif->output(canif, &frame, sizeof(frame), timeout, isotp_sent, &pcb->output_flow);

    if (ret == ERROR_OK)
    {
        return;
    }

    LWCAN_ASSERT("ret == ERROR_OK", ret == ERROR_OK);

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
        LWCAN_ASSERT("pcb->output_flow.state == ISOTP_IDLE", pcb->output_flow.state == ISOTP_IDLE);

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
#else
    if (pcb->output_flow.remaining_data > (CAN_MAX_DLEN - SF_DATA_OFFSET))
#endif
    {
        pcb->output_flow.state = ISOTP_TX_FF;

        pcb->output_flow.cf_sn = 1;

        pcb->output_flow.n_wft = ISOTP_N_WFT;
    }
    else
    {
        pcb->output_flow.state = ISOTP_TX_SF;
    }

    lwcan_timeout(0, isotp_out_flow_output, pcb);

    return ERROR_OK;
}

#endif
