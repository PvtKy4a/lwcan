#ifndef LWCAN_DEBUG_H
#define LWCAN_DEBUG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lwcan/options.h"
#include "lwcan/arch.h"

#ifndef LWCAN_NOASSERT
#define LWCAN_ASSERT(message, assertion)    \
    do                                      \
    {                                       \
        if (!(assertion))                   \
        {                                   \
            LWCAN_PLATFORM_ASSERT(message); \
        }                                   \
    } while (0)
#else /* LWCAN_NOASSERT */
#define LWCAN_ASSERT(message, assertion)
#endif /* LWCAN_NOASSERT */

#ifdef __cplusplus
}
#endif

#endif
