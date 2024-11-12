#ifndef LWCAN_MEMORY_H
#define LWCAN_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

void *lwcan_malloc(size_t size);

void *lwcan_calloc(size_t number, size_t size);

void lwcan_free(void *p);

#ifdef __cplusplus
}
#endif

#endif
