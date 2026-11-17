#ifndef LWCANARCH_SYSTEM_ARCH_H
#define LWCANARCH_SYSTEM_ARCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef uint32_t system_prot_t;

void mock_system_time_set(uint32_t time);

#ifdef __cplusplus
}
#endif

#endif /* LWCANARCH_SYSTEM_ARCH_H */
