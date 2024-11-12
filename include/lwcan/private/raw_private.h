#ifndef LWCAN_RAW_PRIVATE_H
#define LWCAN_RAW_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#if LWCAN_RAW /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/canif.h"
#include "lwcan/can.h"

typedef enum
{
    RAW_INPUT_NONE = 0,    /* frame did not match any pcbs */

    RAW_INPUT_EATEN,       /* frame handed off and delivered to pcb */
} canraw_input_state_t;

void canraw_init(void);

canraw_input_state_t canraw_input(struct canif *canif, void *frame);

#endif

#ifdef __cplusplus
}
#endif

#endif
