#ifndef LWCAN_ERROR_H
#define LWCAN_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum
{
/** No error, everything OK. */
    ERROR_OK = 0,
/** Transmit timeout. */
    ERROR_TRANSMIT_TIMEOUT = -1,
/** Receive timeout. */
    ERROR_RECEIVE_TIMEOUT = -2,
/** PCB. */
    ERROR_ISOTP_PCB = -3,
/** CANIF. */
    ERROR_CANIF = -4,
/** CANIF max. */
    ERROR_CANIF_MAX = -5,
/** ISOTP memory out. */
    ERROR_ISOTP_MEM = -6,
/** ISOTP max. */
    ERROR_ISOTP_MAX = -7,
/** ISOTP state. */
    ERROR_ISOTP_BUSY = -8,
/** Illegal argument. */
    ERROR_ARG = -9,
/** Flow status. */
    ERROR_FLOW_STATUS = -10,
/** Frame sequence. */
    ERROR_FRAME_SEQUENCE = -11
} lwcanperr_enum_t;

typedef int8_t lwcanerr_t;

#ifdef __cplusplus
}
#endif

#endif
