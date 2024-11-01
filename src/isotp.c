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

struct isotp_pcb *isotp_new(struct canif *canif, uint32_t output_id, uint32_t input_id, bool extended_id)
{
    struct isotp_pcb *pcb;

    if (canif == NULL || isotp_pcb_num >= ISOTP_MAX_PCB_NUM)
    {
        goto error;
    }

    pcb = (struct isotp_pcb *)isotp_pcb_malloc();

    if (pcb == NULL)
    {
        goto error;
    }

    memset(pcb, 0, sizeof(struct isotp_pcb));

    pcb->canif_index = canif_get_index(canif);

    pcb->next = isotp_pcb_list;

    pcb->output_id = output_id;

    pcb->input_id = input_id;

    pcb->extended_id = extended_id;

    isotp_pcb_list = pcb;

    isotp_pcb_num += 1;

    return pcb;

error:
    return NULL;
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

uint16_t get_frame_data_length(struct lwcan_frame *frame)
{
    uint16_t data_length = 0;

    uint8_t frame_type = get_frame_type(frame);

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

void isotp_fill_frame(struct isotp_flow *flow, uint8_t frame_type, uint32_t id, bool extended_id)
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

        lwcan_buffer_free(buffer);
    }
    else
    {
        for (buffer_temp = flow->buffer; buffer_temp != NULL; buffer_temp = buffer_temp->next)
        {
            if (buffer_temp->next == buffer)
            {
                buffer_temp->next = buffer->next;

                lwcan_buffer_free(buffer);

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
