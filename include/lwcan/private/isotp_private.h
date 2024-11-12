#ifndef LWCAN_ISOTP_PRIVATE_H
#define LWCAN_ISOTP_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/options.h"

#if LWCAN_ISOTP /* don't build if not configured for use in lwcan_options.h */

#include "lwcan/isotp.h"
#include "lwcan/canif.h"
#include "lwcan/buffer.h"
#include "lwcan/can.h"

#include <stdint.h>

#define FRAME_TYPE_OFFSET   0
#define FRAME_TYPE_MASK     0xF0

#define SF                  0x00    /** Single frame */
#define SF_DL_OFFSET        0       /** Single frame data length offset */
#define SF_DL_MASK          0x0F    /** Single frame data length mask */
#define SF_DATA_OFFSET      1       /** Single frame data offset */
#define FD_SF_FLAG          0       /** CAN FD single frame flag */
#define FD_SF_FLAG_OFFSET   0       /** CAN FD single frame flag offset */
#define FD_SF_FLAG_MASK     0x0F    /** CAN FD single frame flag mask */
#define FD_SF_DL_OFFSET     1       /** CAN FD single frame data length offset */
#define FD_SF_DL_MASK       0xFF    /** CAN FD single frame data length mask */
#define FD_SF_DATA_OFFSET   2       /** CAN FD single frame data offset */

#define FF                  0x10    /** First frame */
#define FF_DL_HI_OFFSET     0       /** First frame data length hi offset */
#define FF_DL_HI_MASK       0x0F    /** First frame data length hi mask */
#define FF_DL_LO_OFFSET     1       /** First frame data length lo offset */
#define FF_DL_LO_MASK       0xFF    /** First frame data length LO mask */
#define FF_DATA_OFFSET      2       /** First frame data offset */
#define FD_FF_FLAG          0       /** CAN FD first frame flag */
#define FD_FF_FLAG_OFFSET   1       /** CAN FD first frame flag offset */
#define FD_FF_FLAG_MASK     0xFF    /** CAN FD first frame flag mask */
#define FD_FF_DL_OFFSET     2       /** CAN FD first frame data length offset */
#define FD_FF_DATA_OFFSET   6       /** CAN FD first frame data offset */

#define CF              0x20   /** Consecutive frame */
#define CF_SN_OFFSET    0      /** Consecutive frame serial number offset */
#define CF_SN_MASK      0x0F   /** Consecutive frame serial number mask */
#define CF_DATA_OFFSET  1      /** Consecutive frame data offset */

#define FC                  0x30    /** Flow control frame */
#define FC_FS_OFFSET        0       /** Flow control frame flow status offset */
#define FC_FS_MASK          0x0F    /** Flow control frame flow status mask */
#define FC_BS_OFFSET        1       /** Flow control frame block size offset */
#define FC_ST_OFFSET        2       /** Flow control frame separation size offset */
#define FC_PADDING_OFFSET   3       /** Flow control frame padding byte offset */

#define FS_READY    0       /** Flow status ready */
#define FS_WAIT     1       /** Flow status wait */
#define FS_OVERFLOW 2       /** Flow status owerflow */

#define ST_MS_RANGE_MIN         0       /** Separation time millisec range min */
#define ST_MS_RANGE_MAX         0x7F    /** Separation time millisec range max */

#define ST_RESERVED1_RANGE_MIN  0x80    /** Separation time reserved range min */
#define ST_RESERVED1_RANGE_MAX  0xF0    /** Separation time reserved range max */

#define ST_US_RANGE_MIN         0xF1    /** Separation time microsec range min */
#define ST_US_RANGE_MAX         0xF9    /** Separation time microsec range max */

#define ST_RESERVED2_RANGE_MIN  0xFA    /** Separation time reserved range min */
#define ST_RESERVED2_RANGE_MAX  0xFF    /** Separation time reserved range max */

typedef enum
{
    ISOTP_IDLE,       /** Idle state */

    ISOTP_TX_SF,      /** Sending single frame */

    ISOTP_TX_FF,      /** Sending first frame */

    ISOTP_TX_CF,      /** Sending consecutive frame */

    ISOTP_TX_FC,      /** Sending flow control frame */

    ISOTP_WAIT_CF,    /** Waiting for consecutive frame */

    ISOTP_WAIT_FC,    /** Waiting for flow control frame */
} isotp_state_t;

void isotp_init(void);

void isotp_input(struct canif *canif, void *frame);

void isotp_out_flow_output(void *arg);

void isotp_in_flow_output(void *arg);

void isotp_sent(struct isotp_pcb *pcb, uint8_t frame_type);

struct isotp_pcb *isotp_get_pcb_list(void);

uint8_t isotp_get_sf_dl(uint8_t *frame_data);

uint32_t isotp_get_ff_dl(uint8_t *frame_data);

void isotp_fill_sf(struct isotp_flow *flow, void *frame);

void isotp_fill_ff(struct isotp_flow *flow, void *frame);

void isotp_fill_cf(struct isotp_flow *flow, void *frame);

void isotp_fill_fc(struct isotp_flow *flow, void *frame);

void isotp_remove_buffer(struct isotp_flow *flow, struct lwcan_buffer *buffer);

void isotp_output_timeout_error_handler(void *arg);

void isotp_input_timeout_error_handler(void *arg);

#endif

#ifdef __cplusplus
}
#endif

#endif
