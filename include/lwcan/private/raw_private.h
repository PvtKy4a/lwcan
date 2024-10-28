#ifndef LWCAN_RAW_PRIVATE_H
#define LWCAN_RAW_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#if LWCAN_RAW /* don't build if not configured for use in lwcan_options.h */

typedef enum
{
    RAW_IO_NONE = 0,    /* frame did not match any pcbs */

    RAW_IO_EATEN,       /* frame handed off and delivered to pcb */

    RAW_IO_SENT,        /* frame sent by pcb */
} canraw_io_state_t;

void canraw_init(void);

canraw_io_state_t canraw_input(struct canif *canif, struct lwcan_frame *frame);

canraw_io_state_t canraw_sent(struct canif *canif, struct lwcan_frame *frame);

#endif

#ifdef __cplusplus
}
#endif

#endif
