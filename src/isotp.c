#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"

#include <stdlib.h>
#include <string.h>

#define ISOTP_PCB_MEM_CHUNK_SIZE sizeof(struct isotp_pcb)

#define ISOTP_PCB_MEM_POOL_SIZE ((ISOTP_PCB_MEM_CHUNK_SIZE * ISOTP_MAX_PCB_NUM) + ISOTP_MAX_PCB_NUM)

static uint8_t isotp_pcb_mem_pool[ISOTP_PCB_MEM_POOL_SIZE];

static struct isotp_pcb *isotp_pcb_list;

static uint8_t isotp_pcb_num = 0;

static void *isotp_pcb_malloc(void)
{
    for (uint16_t i = 0; i < ISOTP_PCB_MEM_POOL_SIZE; i += (ISOTP_PCB_MEM_CHUNK_SIZE + 1))
    {
        if (isotp_pcb_mem_pool[i + ISOTP_PCB_MEM_CHUNK_SIZE] == 0)
        {
            isotp_pcb_mem_pool[i + ISOTP_PCB_MEM_CHUNK_SIZE] = 0xAA;

            return &isotp_pcb_mem_pool[i];
        }
    }

    return NULL;
}

static void isotp_pcb_free(void *mem)
{
    if (((uint8_t *)mem > &isotp_pcb_mem_pool[ISOTP_PCB_MEM_POOL_SIZE - ISOTP_PCB_MEM_CHUNK_SIZE - 1]) || ((uint8_t *)mem < &isotp_pcb_mem_pool[0]))
    {
        return;
    }

    ((uint8_t *)mem)[ISOTP_PCB_MEM_CHUNK_SIZE] = 0;
}

void isotp_init(void)
{
    memset(isotp_pcb_mem_pool, 0, sizeof(isotp_pcb_mem_pool));

    isotp_pcb_list = NULL;
}

struct isotp_pcb *isotp_new(uint8_t canif_index, uint32_t output_id, uint32_t input_id, bool extended_id, bool can_fd)
{
    struct isotp_pcb *pcb;

    if (canif_index == 0 || isotp_pcb_num >= ISOTP_MAX_PCB_NUM)
    {
        return NULL;
    }

    pcb = (struct isotp_pcb *)isotp_pcb_malloc();

    if (pcb == NULL)
    {
        return NULL;
    }

    memset(pcb, 0, sizeof(struct isotp_pcb));

    pcb->canif_index = canif_index;

    pcb->next = isotp_pcb_list;

    pcb->output_id = output_id;

    pcb->input_id = input_id;

    pcb->extended_id = extended_id;

    pcb->can_fd = can_fd;

    isotp_pcb_list = pcb;

    isotp_pcb_num += 1;

    return pcb;
}

lwcanerr_t isotp_bind(struct isotp_pcb *pcb, uint8_t canif_index)
{
    if (pcb == NULL ||canif_index == 0)
    {
        return ERROR_ARG;
    }

    pcb->canif_index = canif_index;

    return ERROR_OK;
}

lwcanerr_t isotp_set_address(struct isotp_pcb *pcb, uint32_t output_id, uint32_t input_id, bool extended_id)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->output_id = output_id;

    pcb->input_id = input_id;

    pcb->extended_id = extended_id;

    return ERROR_OK;
}

lwcanerr_t isotp_close(struct isotp_pcb *pcb)
{
    struct isotp_pcb *pcb_temp;

    lwcanerr_t ret;

    if (pcb == NULL)
    {
        ret = ERROR_ARG;

        goto exit;
    }

    if (isotp_pcb_list == pcb)
    {
        isotp_pcb_list = pcb->next;
    }
    else
    {
        for (pcb_temp = isotp_pcb_list; pcb_temp != NULL; pcb_temp = pcb_temp->next)
        {
            if (pcb_temp->next == pcb)
            {
                pcb_temp->next = pcb->next;

                ret = ERROR_OK;

                break;
            }
        }
        if (pcb_temp == NULL)
        {
            ret = ERROR_OK;

            goto exit;
        }
    }

    isotp_pcb_free(pcb);

    isotp_pcb_num -= 1;

exit:
    return ret;
}

lwcanerr_t isotp_set_receive_callback(struct isotp_pcb *pcb, isotp_receive_function receive)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->receive = receive;

    return ERROR_OK;
}

lwcanerr_t isotp_set_sent_callback(struct isotp_pcb *pcb, isotp_sent_function sent)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->sent = sent;

    return ERROR_OK;
}

lwcanerr_t isotp_set_error_callback(struct isotp_pcb *pcb, isotp_error_function error)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->error = error;

    return ERROR_OK;
}

lwcanerr_t isotp_set_callback_arg(struct isotp_pcb *pcb, void *arg)
{
    if (pcb == NULL)
    {
        return ERROR_ARG;
    }

    pcb->callback_arg = arg;

    return ERROR_OK;
}

struct isotp_pcb *get_isotp_pcb_list(void)
{
    return isotp_pcb_list;
}

uint8_t get_frame_type(struct lwcan_frame *frame)
{
    return (frame->data[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK);
}

uint32_t get_frame_data_length(struct lwcan_frame *frame, uint8_t frame_type)
{
    uint32_t data_length = 0;

    switch (frame_type)
    {
    case SF:
        data_length = (frame->data[SF_DL_OFFSET] & SF_DL_MASK);
        break;

    case FF:
        data_length = (((frame->data[FF_DL_HI_OFFSET] & FF_DL_HI_MASK) << 8) | frame->data[FF_DL_LO_OFFSET]);
        break;

    default:
        break;
    }

    return data_length;
}

uint8_t isotp_get_sf_data_length(struct lwcan_frame *frame)
{
    uint8_t length = 0;

    if ((frame->data[FD_SF_FLAG_OFFSET] & FD_SF_FLAG_MASK) != FD_SF_FLAG)
    {
        length = (frame->data[SF_DL_OFFSET] & SF_DL_MASK);
    }
    else
    {
        length = frame->data[FD_SF_DL_OFFSET];
    }

    return length;
}

uint32_t isotp_get_ff_data_length(struct lwcan_frame *frame)
{
    uint32_t length = 0;

    if (frame->data[FD_FF_FLAG_OFFSET] != FD_SF_FLAG)
    {
        length = (((frame->data[FF_DL_HI_OFFSET] & FF_DL_HI_MASK) << 8) | frame->data[FF_DL_LO_OFFSET]);
    }
    else
    {
        length = *(uint32_t *)(frame->data + FD_FF_DL_OFFSET);
    }

    return length;
}

uint8_t get_frame_serial_number(struct lwcan_frame *frame)
{
    return (frame->data[CF_SN_OFFSET] & CF_SN_MASK);
}

uint8_t get_frame_flow_status(struct lwcan_frame *frame)
{
    return (frame->data[FC_FS_OFFSET] & FC_FS_MASK);
}

uint8_t get_frame_block_size(struct lwcan_frame *frame)
{
    return frame->data[FC_BS_OFFSET];
}

uint8_t get_frame_separation_time(struct lwcan_frame *frame)
{
    uint8_t time;

    if (frame->data[FC_ST_OFFSET] > ST_MS_RANGE_MIN && frame->data[FC_ST_OFFSET] <= ST_MS_RANGE_MAX)
    {
        time = frame->data[FC_ST_OFFSET];
    }
    else if (frame->data[FC_ST_OFFSET] >= ST_US_RANGE_MIN && frame->data[FC_ST_OFFSET] <= ST_US_RANGE_MAX)
    {
        time = 1;
    }
    else
    {
        time = 0;
    }

    return time;
}

static void add_padding(uint8_t *data, uint8_t num)
{
    for (uint8_t i = 0; i < num; i++)
    {
        data[i] = ISOTP_PADDING_BYTE;
    }
}

void isotp_fill_sf(struct isotp_flow *flow, uint32_t id, bool extended_id, bool can_fd)
{
    uint8_t offset, length;

    flow->frame.id = id;

    flow->frame.extended_id = extended_id;

    flow->frame.dlc = lwcan_length_to_dlc(flow->remaining_data);

    flow->frame.data[FRAME_TYPE_OFFSET] = SF;

    if (!can_fd)
    {
        flow->frame.dlc = ISOTP_MIN_DLC;

        flow->frame.data[SF_DL_OFFSET] |= flow->remaining_data & SF_DL_MASK;

        offset = SF_DATA_OFFSET;
    }
    else
    {
        flow->frame.dlc = lwcan_length_to_dlc(flow->remaining_data);

        if (flow->frame.dlc < ISOTP_MIN_DLC)
        {
            flow->frame.dlc = ISOTP_MIN_DLC;
        }

        flow->frame.data[FD_SF_DL_OFFSET] = flow->remaining_data & SF_DL_MASK;

        offset = FD_SF_DATA_OFFSET;
    }

    length = lwcan_dlc_to_length(flow->frame.dlc);

    lwcan_buffer_copy_from(flow->buffer, flow->frame.data + offset, flow->remaining_data);

    if (flow->remaining_data < (uint8_t)(length - offset))
    {
        offset += flow->remaining_data;

        add_padding(flow->frame.data + offset, (length - offset));
    }

    flow->remaining_data -= flow->remaining_data;
}

void isotp_fill_ff(struct isotp_flow *flow, uint32_t id, bool extended_id, bool can_fd)
{
    uint8_t offset, length;

    flow->frame.id = id;

    flow->frame.extended_id = extended_id;

    flow->frame.dlc = lwcan_length_to_dlc(flow->remaining_data);

    flow->frame.data[FRAME_TYPE_OFFSET] = FF;

    if (!can_fd)
    {
        flow->frame.dlc = ISOTP_MIN_DLC;

        flow->frame.data[FF_DL_HI_OFFSET] |= (uint8_t)((flow->remaining_data << 8) & FF_DL_HI_MASK);

        flow->frame.data[FF_DL_LO_OFFSET] = (uint8_t)(flow->remaining_data & FF_DL_LO_MASK);

        offset = FF_DATA_OFFSET;
    }
    else
    {
        flow->frame.dlc = lwcan_length_to_dlc(flow->remaining_data);

        if (flow->frame.dlc < ISOTP_MIN_DLC)
        {
            flow->frame.dlc = ISOTP_MIN_DLC;
        }

        flow->frame.data[FD_FF_FLAG_OFFSET] = FD_FF_FLAG;

        *(uint32_t *)(flow->frame.data + FD_FF_DL_OFFSET) = flow->remaining_data;

        offset = FD_FF_DATA_OFFSET;
    }

    length = lwcan_dlc_to_length(flow->frame.dlc);

    lwcan_buffer_copy_from(flow->buffer, flow->frame.data + offset, (length - offset));

    flow->remaining_data -= (length - offset);
}

void isotp_fill_cf(struct isotp_flow *flow, uint32_t id, bool extended_id, bool can_fd)
{
    uint8_t offset, length;

    flow->frame.id = id;

    flow->frame.extended_id = extended_id;

    if (!can_fd)
    {
        flow->frame.dlc = ISOTP_MIN_DLC;
    }
    else
    {
        flow->frame.dlc = lwcan_length_to_dlc(flow->remaining_data);

        if (flow->frame.dlc < ISOTP_MIN_DLC)
        {
            flow->frame.dlc = ISOTP_MIN_DLC;
        }
    }

    length = lwcan_dlc_to_length(flow->frame.dlc);

    flow->frame.data[FRAME_TYPE_OFFSET] = CF;

    flow->frame.data[CF_SN_OFFSET] |= flow->cf_sn & CF_SN_MASK;

    offset = CF_DATA_OFFSET;

    if (flow->remaining_data < (uint8_t)(length - offset))
    {
        lwcan_buffer_copy_from_offset(flow->buffer, flow->frame.data + offset, flow->remaining_data, (flow->buffer->length - flow->remaining_data));

        offset += ((flow->buffer->length - flow->remaining_data) + 1);

        add_padding(flow->frame.data + offset, (length - offset));

        flow->remaining_data -= flow->remaining_data;
    }
    else
    {
        lwcan_buffer_copy_from_offset(flow->buffer, flow->frame.data + offset, (length - offset), (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= (length - offset);
    }
}

void isotp_fill_fc(struct isotp_flow *flow, uint32_t id, bool extended_id)
{
    uint8_t length;

    flow->frame.id = id;

    flow->frame.extended_id = extended_id;

    flow->frame.dlc = ISOTP_MIN_DLC;

    flow->frame.data[FRAME_TYPE_OFFSET] = FC;

    flow->frame.data[FC_FS_OFFSET] |= flow->fs & FC_FS_MASK;

    flow->frame.data[FC_BS_OFFSET] = flow->bs;

    flow->frame.data[FC_ST_OFFSET] = flow->st;

    length = lwcan_dlc_to_length(flow->frame.dlc);

    if (length >= FC_PADDING_OFFSET)
    {
        add_padding(flow->frame.data + FC_PADDING_OFFSET, (length - FC_PADDING_OFFSET));
    }
}

void isotp_remove_buffer(struct isotp_flow *flow, struct lwcan_buffer *buffer)
{
    struct lwcan_buffer *buffer_temp;

    if (flow == NULL || buffer == NULL)
    {
        return;
    }

    if (flow->buffer == buffer)
    {
        flow->buffer = flow->buffer->next;

        lwcan_buffer_delete(buffer);
    }
    else
    {
        for (buffer_temp = flow->buffer; buffer_temp != NULL; buffer_temp = buffer_temp->next)
        {
            if (buffer_temp->next == buffer)
            {
                buffer_temp->next = buffer->next;

                lwcan_buffer_delete(buffer);

                break;
            }
        }
    }
}

void isotp_output_timeout_error_handler(void *arg)
{
    struct isotp_pcb *pcb;

    if (arg == NULL)
    {
        return;
    }

    pcb = (struct isotp_pcb *)arg;

    isotp_remove_buffer(&pcb->output_flow, pcb->output_flow.buffer);

    pcb->output_flow.state = ISOTP_STATE_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb, ERROR_TRANSMIT_TIMEOUT);
    }
}

void isotp_input_timeout_error_handler(void *arg)
{
    struct isotp_pcb *pcb;

    if (arg == NULL)
    {
        return;
    }

    pcb = (struct isotp_pcb *)arg;

    isotp_remove_buffer(&pcb->input_flow, pcb->input_flow.buffer);

    pcb->input_flow.state = ISOTP_STATE_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb, ERROR_RECEIVE_TIMEOUT);
    }
}

#endif
