#include "lwcanarch/system_arch.h"
#include "lwcanarch/cc.h"

#include "lwcan/system.h"

static uint32_t mock_platform_time = 0;

uint32_t system_now(void)
{
    return mock_platform_time;
}

system_prot_t system_arch_protect(void)
{
    return 1;
}

void system_arch_unprotect(system_prot_t pval)
{
    (void)pval;
}

void mock_system_time_set(uint32_t time)
{
    mock_platform_time = time;
}
