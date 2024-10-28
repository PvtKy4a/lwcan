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
/** Illegal argument. */
    ERROR_ARG = -1,
/** CANIF. */
    ERROR_CANIF = -2,
    /** CANIF max. */
    ERROR_CANIF_MAX = -3,
/** Out of memory error. */
    ERROR_MEMORY = -4,
/** Transmit timeout. */
    ERROR_TRANSMIT_TIMEOUT = -5,
/** Receive timeout. */
    ERROR_RECEIVE_TIMEOUT = -6,
/** ISOTP state. */
    ERROR_INPROGRESS = -7,
/** Flow status. */
    ERROR_ISOTP_FLOW_STATUS = -8,
/** Frame sequence. */
    ERROR_ISOTP_FRAME_SEQUENCE = -9
} lwcan_error_enum_t;

typedef int8_t lwcanerr_t;

#ifdef __cplusplus
}
#endif

#endif
