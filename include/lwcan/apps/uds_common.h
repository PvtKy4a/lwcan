#ifndef UDS_COMMON_H
#define UDS_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lwcan/error.h"

#include <stdint.h>

struct uds_context
{
    void (*request)(void *handle, uint8_t sid, uint8_t *data, uint32_t size); /* For server (not implemented yet) */

    void (*positive_response)(void *handle, uint8_t *data, uint32_t size); /* For client */

    void (*negative_response)(void *handle, uint8_t rejected_sid, uint8_t nrc); /* For client */

    void (*error)(void *handle, lwcanerr_t error); /* Common */
};

#ifdef __cplusplus
}
#endif

#endif
