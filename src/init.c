#include "lwcan/init.h"
#include "lwcan/options.h"
#include "lwcan/private/isotp_private.h"
#include "lwcan/private/raw_private.h"
#include "lwcan/private/timeouts_private.h"

void lwcan_init(void)
{
#if LWCAN_ISOTP
    isotp_init();
#endif

#if LWCAN_RAW
    canraw_init();
#endif

    lwcan_timeouts_init();
}
