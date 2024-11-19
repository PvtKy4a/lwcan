#ifndef LWCAN_ARCH_H
#define LWCAN_ARCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lwcanarch/cc.h"

#ifndef LWCAN_PLATFORM_ASSERT
#define LWCAN_PLATFORM_ASSERT(x)                             \
    do                                                       \
    {                                                        \
        printf("Assertion \"%s\" failed at line %d in %s\n", \
               x, __LINE__, __FILE__);                       \
    } while (0)
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef __cplusplus
}
#endif

#endif
