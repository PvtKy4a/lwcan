#include "lwcan/buffer.h"
#include "lwcan/error.h"
#include "lwcan/options.h"
#include "lwcan/memory.h"

#include <string.h>

struct lwcan_buffer *lwcan_buffer_new(uint32_t length)
{
    struct lwcan_buffer *buffer;

    if (length == 0)
    {
        return NULL;
    }

    buffer = (struct lwcan_buffer *)lwcan_malloc(sizeof(struct lwcan_buffer));

    if (buffer == NULL)
    {
        return NULL;
    }

    buffer->payload = (uint8_t *)lwcan_malloc(length);

    if (buffer->payload == NULL)
    {
        lwcan_free(buffer);

        return NULL;
    }

    buffer->length = length;

    return buffer;
}

void lwcan_buffer_delete(struct lwcan_buffer *buffer)
{
    lwcan_free(buffer->payload);

    lwcan_free(buffer);
}

lwcanerr_t lwcan_buffer_copy_to(struct lwcan_buffer *buffer, const uint8_t *source, uint32_t length)
{
    if (buffer == NULL || source == NULL || length == 0)
    {
        return ERROR_ARG;
    }

    memcpy(buffer->payload, source, length);

    return ERROR_OK;
}

lwcanerr_t lwcan_buffer_copy_to_offset(struct lwcan_buffer *buffer, const uint8_t *source, uint32_t length, uint32_t offset)
{
    if (buffer == NULL || source == NULL || length == 0)
    {
        return ERROR_ARG;
    }

    memcpy(buffer->payload + offset, source, length);

    return ERROR_OK;
}

lwcanerr_t lwcan_buffer_copy_from(struct lwcan_buffer *buffer, uint8_t *distination, uint32_t length)
{
    if (buffer == NULL || distination == NULL || length == 0)
    {
        return ERROR_ARG;
    }

    memcpy(distination, buffer->payload, length);

    return ERROR_OK;
}

lwcanerr_t lwcan_buffer_copy_from_offset(struct lwcan_buffer *buffer, uint8_t *distination, uint32_t length, uint32_t offset)
{
    if (buffer == NULL || distination == NULL || length == 0)
    {
        return ERROR_ARG;
    }

    memcpy(distination, buffer->payload + offset, length);

    return ERROR_OK;
}
