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

#ifdef __cplusplus
}
#endif

#endif
