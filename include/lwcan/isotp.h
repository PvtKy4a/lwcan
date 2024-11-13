#ifndef LWCAN_ISOTP_H
#define LWCAN_ISOTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/error.h"
#include "lwcan/buffer.h"
#include "lwcan/can.h"

#include <stdint.h>

struct isotp_pcb;

typedef lwcanerr_t (*isotp_receive_function)(void *arg, struct isotp_pcb *pcb, struct lwcan_buffer *buffer);

typedef lwcanerr_t (*isotp_sent_function)(void *arg, struct isotp_pcb *pcb, uint32_t length);

typedef void (*isotp_error_function)(void *arg, lwcanerr_t error);

struct isotp_flow
{
    uint8_t state;

    struct lwcan_buffer *buffer;

    uint32_t remaining_data;

    uint8_t cf_sn;  /** Consecutive frame serial number */

    uint8_t fs; /** Flow status */

    uint8_t bs; /** Block size */

    uint8_t st; /** Separation time */

    uint8_t n_wft; /** Number of receiver wait frames */
};

struct isotp_pcb
{
    struct isotp_pcb *next;

    uint8_t if_index;

    isotp_receive_function receive;

    isotp_sent_function sent;

    isotp_error_function error;

    void *callback_arg;

    canid_t tx_id;

    canid_t rx_id;

    struct isotp_flow output_flow;

    struct isotp_flow input_flow;
};

struct isotp_pcb *isotp_new(void);

lwcanerr_t isotp_bind(struct isotp_pcb *pcb, const struct addr_can *addr);

lwcanerr_t isotp_remove(struct isotp_pcb *pcb);

lwcanerr_t isotp_send(struct isotp_pcb *pcb, const uint8_t *data, uint32_t length);

lwcanerr_t isotp_received(struct isotp_pcb *pcb, struct lwcan_buffer *buffer);

lwcanerr_t isotp_set_receive_callback(struct isotp_pcb *pcb, isotp_receive_function receive);

lwcanerr_t isotp_set_sent_callback(struct isotp_pcb *pcb, isotp_sent_function sent);

lwcanerr_t isotp_set_error_callback(struct isotp_pcb *pcb, isotp_error_function error);

lwcanerr_t isotp_set_callback_arg(struct isotp_pcb *pcb, void *arg);

#endif

#ifdef __cplusplus
}
#endif

#endif
