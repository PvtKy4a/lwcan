#ifndef LWCAN_RAW_H
#define LWCAN_RAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#if LWCAN_RAW /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/error.h"
#include "lwcan/buffer.h"
#include "lwcan/frame.h"
#include "lwcan/canif.h"

#include <stdint.h>
#include <stdbool.h>

struct canraw_pcb;

typedef uint8_t (*canraw_receive_function)(void *arg, struct canraw_pcb *pcb, struct lwcan_frame *frame);

typedef void (*canraw_sent_function)(void *arg, struct canraw_pcb *pcb);

typedef void (*canraw_error_function)(void *arg, lwcanerr_t error);

struct canraw_pcb
{
    struct canraw_pcb *next;

    uint8_t canif_index;

    canraw_receive_function receive;

    canraw_sent_function sent;

    canraw_error_function error;

    void *callback_arg;

    struct lwcan_frame frame;
};

struct canraw_pcb *canraw_new(struct canif *canif);

lwcanerr_t canraw_close(struct canraw_pcb *pcb);

lwcanerr_t canraw_send(struct canraw_pcb *pcb, struct lwcan_frame *frame);

lwcanerr_t canraw_set_receive_callback(struct canraw_pcb *pcb, canraw_receive_function receive);

lwcanerr_t canraw_set_sent_callback(struct canraw_pcb *pcb, canraw_sent_function sent);

lwcanerr_t canraw_set_error_callback(struct canraw_pcb *pcb, canraw_error_function error);

lwcanerr_t canraw_set_callback_arg(struct canraw_pcb *pcb, void *arg);

#endif

#ifdef __cplusplus
}
#endif

#endif
