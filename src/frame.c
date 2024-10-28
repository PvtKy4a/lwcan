#include "lwcan/frame.h"
#include "lwcan/private/isotp_private.h"

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
