#ifndef LWCAN_BUFFER_H
#define LWCAN_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwcan/error.h"

#include <stdint.h>
#include <stdbool.h>

struct lwcan_buffer
{
    struct lwcan_buffer *next;

    uint8_t *payload;

    uint16_t length;
};

struct lwcan_buffer *lwcan_buffer_malloc(uint16_t length);

lwcanerr_t lwcan_buffer_free(struct lwcan_buffer *buffer);

lwcanerr_t lwcan_buffer_copy_to(struct lwcan_buffer *buffer, const uint8_t *source, uint16_t length);

lwcanerr_t lwcan_buffer_copy_to_offset(struct lwcan_buffer *buffer, const uint8_t *source, uint16_t length, uint16_t offset);

lwcanerr_t lwcan_buffer_copy_from(struct lwcan_buffer *buffer, uint8_t *distination, uint16_t length);

lwcanerr_t lwcan_buffer_copy_from_offset(struct lwcan_buffer *buffer, uint8_t *distination, uint16_t length, uint16_t offset);

#ifdef __cplusplus
}
#endif

#endif
