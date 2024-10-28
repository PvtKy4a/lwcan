#ifndef LWCAN_FRAME_H
#define LWCAN_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#include <stdint.h>
#include <stdbool.h>

struct lwcan_frame
{
    uint32_t id;

    bool extended_id;

    uint8_t dlc;

    uint8_t data[LWCAN_DLC];
};

uint8_t get_frame_type(struct lwcan_frame *frame);

uint16_t get_frame_data_length(struct lwcan_frame *frame);

uint8_t get_frame_serial_number(struct lwcan_frame *frame);

uint8_t get_frame_flow_status(struct lwcan_frame *frame);

uint8_t get_frame_block_size(struct lwcan_frame *frame);

uint8_t get_frame_separation_time(struct lwcan_frame *frame);

#ifdef __cplusplus
}
#endif

#endif
