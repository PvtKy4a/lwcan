#include "lwcan/init.h"
#include "lwcan/private/memory_private.h"
#include "lwcan/private/timeouts_private.h"

void lwcan_init(void)
{
    memory_init();

    lwcan_timeouts_init();
}
