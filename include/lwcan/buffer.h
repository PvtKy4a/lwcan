#ifndef LWCAN_BUFFER_H
#define LWCAN_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/error.h"

#include <stdint.h>

struct lwcan_buffer
{
    struct lwcan_buffer *next;

    uint8_t *payload;

    uint32_t length;
};

struct lwcan_buffer *lwcan_buffer_new(uint32_t length);

void lwcan_buffer_delete(struct lwcan_buffer *buffer);

lwcanerr_t lwcan_buffer_copy_to(struct lwcan_buffer *buffer, const uint8_t *source, uint32_t length);

lwcanerr_t lwcan_buffer_copy_to_offset(struct lwcan_buffer *buffer, const uint8_t *source, uint32_t length, uint32_t offset);

lwcanerr_t lwcan_buffer_copy_from(struct lwcan_buffer *buffer, uint8_t *distination, uint32_t length);

lwcanerr_t lwcan_buffer_copy_from_offset(struct lwcan_buffer *buffer, uint8_t *distination, uint32_t length, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif
