#ifndef LWCAN_RAW_H
#define LWCAN_RAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#if LWCAN_RAW /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/error.h"
#include "lwcan/can.h"

#include <stdint.h>

struct canraw_pcb;

/** Function prototype for raw pcb receive callback functions.
 * @param arg user supplied argument
 * @param pcb the canraw_pcb which received data
 * @param frame the frame that was received
 * @return 1 if the frame was 'eaten',
 *         0 if the frame lives on
 */
typedef uint8_t (*canraw_receive_function)(void *arg, struct canraw_pcb *pcb, void *frame);

struct canraw_pcb
{
    struct canraw_pcb *next;

    uint8_t if_index;

    canraw_receive_function receive;

    void *callback_arg;
};

struct canraw_pcb *canraw_new(void);

lwcanerr_t canraw_bind(struct canraw_pcb *pcb, struct addr_can *addr);

lwcanerr_t canraw_close(struct canraw_pcb *pcb);

lwcanerr_t canraw_send(struct canraw_pcb *pcb, void *frame, uint8_t size);

lwcanerr_t canraw_set_receive_callback(struct canraw_pcb *pcb, canraw_receive_function receive);

lwcanerr_t canraw_set_callback_arg(struct canraw_pcb *pcb, void *arg);

#endif

#ifdef __cplusplus
}
#endif

#endif
