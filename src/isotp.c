#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/timeouts.h"

#include <string.h>

#define ISOTP_PCB_MEM_CHUNK_SIZE sizeof(struct isotp_pcb)

#define ISOTP_PCB_MEM_POOL_SIZE ((ISOTP_PCB_MEM_CHUNK_SIZE * ISOTP_MAX_PCB_NUM) + ISOTP_MAX_PCB_NUM)

static uint8_t isotp_pcb_mem_pool[ISOTP_PCB_MEM_POOL_SIZE];

static struct isotp_pcb *isotp_pcb_list;

static uint8_t isotp_pcb_num = 0;

#if ISOTP_CANFD
static const uint8_t padding_length[] = {
    8, 8, 8, 8, 8, 8, 8, 8, 8,	    /* 0 - 8 */
    12, 12, 12, 12,			        /* 9 - 12 */
    16, 16, 16, 16,			        /* 13 - 16 */
    20, 20, 20, 20,			        /* 17 - 20 */
    24, 24, 24, 24,			        /* 21 - 24 */
    32, 32, 32, 32, 32, 32, 32, 32, /* 25 - 32 */
    48, 48, 48, 48, 48, 48, 48, 48,	/* 33 - 40 */
    48, 48, 48, 48, 48, 48, 48, 48	/* 41 - 48 */
};
#endif

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
    if (mem == NULL)
    {
        return;
    }

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

struct isotp_pcb *isotp_new(void)
{
    struct isotp_pcb *pcb;

    if (isotp_pcb_num >= ISOTP_MAX_PCB_NUM)
    {
        return NULL;
    }

    pcb = (struct isotp_pcb *)isotp_pcb_malloc();

    if (pcb == NULL)
    {
        return NULL;
    }

    memset(pcb, 0, sizeof(struct isotp_pcb));

    pcb->next = isotp_pcb_list;

    isotp_pcb_list = pcb;

    isotp_pcb_num += 1;

    return pcb;
}

lwcanerr_t isotp_bind(struct isotp_pcb *pcb, const struct addr_can *addr)
{
    canid_t tx_id, rx_id;

    if (pcb == NULL ||addr == NULL)
    {
        return ERROR_ARG;
    }

    if (addr->can_ifindex == 0)
    {
        return ERROR_CANIF;
    }

    tx_id = addr->can_addr.tp.tx_id;

    rx_id = addr->can_addr.tp.rx_id;

    if (tx_id & CAN_EFF_FLAG)
    {
        tx_id &= (CAN_EFF_FLAG | CAN_EFF_MASK);
    }
    else
    {
        tx_id &= CAN_SFF_MASK;
    }

    if (rx_id & CAN_EFF_FLAG)
    {
        rx_id &= (CAN_EFF_FLAG | CAN_EFF_MASK);
    }
    else
    {
        rx_id &= CAN_SFF_MASK;
    }

    if (tx_id != addr->can_addr.tp.tx_id || rx_id != addr->can_addr.tp.rx_id)
    {
        return ERROR_ARG;
    }

    pcb->if_index = addr->can_ifindex;

    pcb->tx_id = tx_id;

    pcb->rx_id = rx_id;

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

struct isotp_pcb *isotp_get_pcb_list(void)
{
    return isotp_pcb_list;
}

uint8_t isotp_get_sf_dl(uint8_t *frame_data)
{
    uint8_t length = 0;

#if ISOTP_CANFD
    if ((frame_data[FD_SF_FLAG_OFFSET] & FD_SF_FLAG_MASK) == FD_SF_FLAG)
    {
        length = frame_data[FD_SF_DL_OFFSET];
    }
    else
#endif
    {
        length = (frame_data[SF_DL_OFFSET] & SF_DL_MASK);
    }

    return length;
}

uint32_t isotp_get_ff_dl(uint8_t *frame_data)
{
    uint32_t length = 0;

#if ISOTP_CANFD
    if (frame_data[FD_FF_FLAG_OFFSET] == FD_FF_FLAG)
    {
        length = frame_data[FD_FF_DL_OFFSET] << 24;

        length = frame_data[FD_FF_DL_OFFSET + 1] << 16;

        length = frame_data[FD_FF_DL_OFFSET + 2] << 8;

        length = frame_data[FD_FF_DL_OFFSET + 3];
    }
    else
#endif
    {
        length = (frame_data[FF_DL_HI_OFFSET] & FF_DL_HI_MASK) << 8;

        length |= frame_data[FF_DL_LO_OFFSET];
    }

    return length;
}

#if ISOTP_CANFD
static uint8_t get_padding_length(uint32_t length)
{
    if (length > 48)
    {
        return 64;
    }
	
    return padding_length[length];
}
#endif

static void add_padding(uint8_t *data, uint8_t num)
{
    memset(data, ISOTP_PADDING_BYTE, num);
}

void isotp_fill_sf(struct isotp_flow *flow, void *frame)
{
#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;

    _frame->len = get_padding_length(flow->remaining_data);

    _frame->data[FRAME_TYPE_OFFSET] = SF;

    _frame->data[FD_SF_DL_OFFSET] = (flow->remaining_data & FD_SF_DL_MASK);

    lwcan_buffer_copy_from(flow->buffer, (_frame->data + FD_SF_DATA_OFFSET), flow->remaining_data);

    if (flow->remaining_data < (uint8_t)(_frame->len - FD_SF_DATA_OFFSET))
    {
        add_padding((_frame->data + FD_SF_DATA_OFFSET + flow->remaining_data), (_frame->len - (FD_SF_DATA_OFFSET + flow->remaining_data)));
    }
#else
    struct can_frame *_frame = (struct can_frame *)frame;

    _frame->len = CAN_MAX_DLEN;

    _frame->data[FRAME_TYPE_OFFSET] = SF;

    _frame->data[SF_DL_OFFSET] |= (flow->remaining_data & SF_DL_MASK);

    lwcan_buffer_copy_from(flow->buffer, (_frame->data + SF_DATA_OFFSET), flow->remaining_data);

    if (flow->remaining_data < (uint8_t)(_frame->len - SF_DATA_OFFSET))
    {
        add_padding((_frame->data + SF_DATA_OFFSET + flow->remaining_data), (_frame->len - (SF_DATA_OFFSET + flow->remaining_data)));
    }
#endif

    flow->remaining_data -= flow->remaining_data;
}

void isotp_fill_ff(struct isotp_flow *flow, void *frame)
{
#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;

    _frame->len = get_padding_length(flow->remaining_data);

    _frame->data[FRAME_TYPE_OFFSET] = FF;

    _frame->data[FD_FF_FLAG_OFFSET] = FD_FF_FLAG;

    *(uint32_t *)(_frame->data + FD_FF_DL_OFFSET) = flow->remaining_data;

    _frame->data[FD_FF_DL_OFFSET] = (uint8_t)(flow->remaining_data >> 24) & (uint8_t)0xFF;

    _frame->data[FD_FF_DL_OFFSET + 1] = (uint8_t)(flow->remaining_data >> 16) & (uint8_t)0xFF;

    _frame->data[FD_FF_DL_OFFSET + 2] = (uint8_t)(flow->remaining_data >> 8) & (uint8_t)0xFF;

    _frame->data[FD_FF_DL_OFFSET + 3] = (uint8_t)flow->remaining_data & (uint8_t)0xFF;

    lwcan_buffer_copy_from(flow->buffer, (_frame->data + FD_FF_DATA_OFFSET), (_frame->len - FD_FF_DATA_OFFSET));

    flow->remaining_data -= (_frame->len - FD_FF_DATA_OFFSET);
#else
    struct can_frame *_frame = (struct can_frame *)frame;

    _frame->len = CAN_MAX_DLEN;

    _frame->data[FRAME_TYPE_OFFSET] = FF;

    _frame->data[FF_DL_HI_OFFSET] |= (uint8_t)(flow->remaining_data >> 8) & (uint8_t)FF_DL_HI_MASK;

    _frame->data[FF_DL_LO_OFFSET] = (uint8_t)flow->remaining_data & (uint8_t)FF_DL_LO_MASK;

    lwcan_buffer_copy_from(flow->buffer, (_frame->data + FF_DATA_OFFSET), (_frame->len - FF_DATA_OFFSET));

    flow->remaining_data -= (_frame->len - FF_DATA_OFFSET);
#endif
}

void isotp_fill_cf(struct isotp_flow *flow, void *frame)
{
#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;

    _frame->len = get_padding_length(flow->remaining_data);
#else
    struct can_frame *_frame = (struct can_frame *)frame;

    _frame->len = CAN_MAX_DLEN;
#endif

    _frame->data[FRAME_TYPE_OFFSET] = CF;

    _frame->data[CF_SN_OFFSET] |= (flow->cf_sn & CF_SN_MASK);

    if (flow->remaining_data < (uint8_t)(_frame->len - CF_DATA_OFFSET))
    {
        lwcan_buffer_copy_from_offset(flow->buffer, (_frame->data + CF_DATA_OFFSET), flow->remaining_data, (flow->buffer->length - flow->remaining_data));

        add_padding((_frame->data + flow->remaining_data + CF_DATA_OFFSET), (_frame->len - (CF_DATA_OFFSET + flow->remaining_data)));

        flow->remaining_data -= flow->remaining_data;
    }
    else
    {
        lwcan_buffer_copy_from_offset(flow->buffer, (_frame->data + CF_DATA_OFFSET), (_frame->len - CF_DATA_OFFSET), (flow->buffer->length - flow->remaining_data));

        flow->remaining_data -= (_frame->len - CF_DATA_OFFSET);
    }
}

void isotp_fill_fc(struct isotp_flow *flow, void *frame)
{
#if ISOTP_CANFD
    struct canfd_frame *_frame = (struct canfd_frame *)frame;
#else
    struct can_frame *_frame = (struct can_frame *)frame;
#endif

    _frame->len = CAN_MAX_DLEN;

    _frame->data[FRAME_TYPE_OFFSET] = FC;

    _frame->data[FC_FS_OFFSET] |= (flow->fs & FC_FS_MASK);

    _frame->data[FC_BS_OFFSET] = flow->bs;

    _frame->data[FC_ST_OFFSET] = flow->st;

    if (_frame->len >= FC_PADDING_OFFSET)
    {
        add_padding((_frame->data + FC_PADDING_OFFSET), (_frame->len - FC_PADDING_OFFSET));
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

    pcb->output_flow.state = ISOTP_IDLE;

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

    pcb->input_flow.state = ISOTP_IDLE;

    if (pcb->error != NULL)
    {
        pcb->error(pcb, ERROR_RECEIVE_TIMEOUT);
    }
}

#endif
