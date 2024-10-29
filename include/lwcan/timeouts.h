#ifndef LWCAN_TIMEOUTS_H
#define LWCAN_TIMEOUTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

void lwcan_check_timeouts(void);

void lwcan_timeout(uint32_t time_ms, void (*handler)(void*), void *arg);

void lwcan_untimeout(void (*handler)(void *), void *arg);

#ifdef __cplusplus
}
#endif

#endif
