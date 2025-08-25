#ifndef LWCAN_TIMEOUTS_H
#define LWCAN_TIMEOUTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/** Function prototype for a timeout callback function. Register such a function
 * using lwcan_timeout().
 *
 * @param arg Additional argument to pass to the function - set up by lwcan_timeout()
 */
typedef void (*lwcan_timeout_handler)(void *arg);

void lwcan_timeouts_handler(void);

void lwcan_timeout(uint32_t time_ms, lwcan_timeout_handler handler, void *arg);

void lwcan_untimeout(lwcan_timeout_handler handler, void *arg);

#ifdef __cplusplus
}
#endif

#endif
