#include "lwcanarch/system_arch.h"

#include "lwcan/system.h"

static uint32_t mock_platform_time = 0;

uint32_t system_now(void)
{
    return mock_platform_time;
}

void mock_system_time_set(uint32_t time)
{
    mock_platform_time = time;
}
