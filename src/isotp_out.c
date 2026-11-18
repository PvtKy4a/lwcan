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

    lwcan_timeout(&pcb->output_flow.error_timeout, ISOTP_N_BS, isotp_timeout_error_handler, &pcb->output_flow);
}

static void sent_cf(struct isotp_pcb *pcb)
{
    uint32_t length;

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

    pcb->output_flow.cf_count += 1;

    pcb->output_flow.cf_sn += 1;

    if (pcb->output_flow.cf_sn > CF_SN_MASK)
    {
        pcb->output_flow.cf_sn = 0;
    }

    if (pcb->output_flow.bs == 0 || pcb->output_flow.cf_count < pcb->output_flow.bs)
    {
        pcb->output_flow.state = ISOTP_TX_CF;

        lwcan_timeout(&pcb->output_flow.output_timeout, pcb->output_flow.st, isotp_output, &pcb->output_flow);
    }
    else
    {
        pcb->output_flow.cf_count = 0;

        pcb->output_flow.state = ISOTP_WAIT_FC;

        lwcan_timeout(&pcb->output_flow.error_timeout, ISOTP_N_BS, isotp_timeout_error_handler, &pcb->output_flow);
    }
}

static void sent_fc(struct isotp_pcb *pcb)
{
    switch (pcb->input_flow.fs)
    {
    case FS_READY:
        pcb->input_flow.state = ISOTP_WAIT_CF;
        lwcan_timeout(&pcb->input_flow.error_timeout, ISOTP_N_CR, isotp_timeout_error_handler, &pcb->input_flow);
        break;
    case FS_WAIT: /* Currently not supported */
        pcb->input_flow.state = ISOTP_IDLE;
        LWCAN_ASSERT("pcb->input_flow.fs != FS_WAIT", pcb->input_flow.fs != FS_WAIT);
        break;
    case FS_OVERFLOW:
        pcb->input_flow.state = ISOTP_IDLE;
        if (pcb->error != NULL)
        {
            pcb->error(pcb->callback_arg, ERROR_MEMORY);
        }
        break;
    }
}

void isotp_sent(void *arg, lwcanerr_t error)
{
    struct isotp_flow *flow;

    flow = (struct isotp_flow *)arg;

    if (error != ERROR_OK)
    {
        isotp_remove_buffer(flow, flow->buffer);

        flow->state = ISOTP_IDLE;

        if (flow->pcb->error != NULL)
        {
            flow->pcb->error(flow->pcb->callback_arg, error);
        }

        return;
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

void isotp_output(void *arg)
{
    struct isotp_flow *flow;

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

    flow = (struct isotp_flow *)arg;

    pcb = flow->pcb;

    canif = canif_get_by_index(pcb->if_index);

    if (canif == NULL || canif->output == NULL)
    {
        ret = ERROR_CANIF;

        goto error;
    }

    memset(&frame, 0, sizeof(frame));

    frame.can_id = pcb->tx_id;

#if ISOTP_CANFD
    if (ISOTP_CANFD_BRS)
    {
        frame.flags = CANFD_BRS;
    }
#endif

    switch (flow->state)
    {
        case ISOTP_TX_SF:
            isotp_fill_sf(flow, &frame);
            timeout = ISOTP_N_AS;
            break;

        case ISOTP_TX_FF:
            isotp_fill_ff(flow, &frame);
            timeout = ISOTP_N_AS;
            break;

        case ISOTP_TX_CF:
            isotp_fill_cf(flow, &frame);
            timeout = ISOTP_N_CS;
            break;

        case ISOTP_TX_FC:
            isotp_fill_fc(flow, &frame);
            timeout = ISOTP_N_BR;
            break;

        default:
            return;
    }

    ret = canif->output(canif, &frame, sizeof(frame), timeout, isotp_sent, flow);

    if (ret == ERROR_OK)
    {
        return;
    }

error:
    isotp_remove_buffer(flow, flow->buffer);

    flow->state = ISOTP_IDLE;

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

        return ERROR_ARG;
    }

    if (pcb->output_flow.state != ISOTP_IDLE)
    {
        return ERROR_INPROGRESS;
    }

#if !ISOTP_CANFD
    if (length > 4095)
    {
        return ERROR_ARG;
    }
#endif

    if (length > LWCAN_MEM_SIZE)
    {
        return ERROR_MEMORY;
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

        pcb->output_flow.cf_count = 0;

        pcb->output_flow.cf_sn = 1;

        pcb->output_flow.n_wft = ISOTP_N_WFT;
    }
    else
    {
        pcb->output_flow.state = ISOTP_TX_SF;
    }

    lwcan_timeout(&pcb->output_flow.output_timeout, 0, isotp_output, &pcb->output_flow);

    return ERROR_OK;
}

#endif
