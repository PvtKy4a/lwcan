#include "lwcan/timeouts.h"
#include "lwcan/error.h"
#include "lwcan/system.h"
#include "lwcan/options.h"
#include "lwcan/debug.h"

#include <stdbool.h>
#include <string.h>

#define MAX_TIMEOUT 0x7fffffff

#define TIMEOUT_MEM_CHUNK_SIZE sizeof(struct lwcan_timeout)

#define TIMEOUT_MEM_POOL_SIZE (TIMEOUT_MEM_CHUNK_SIZE * LWCAN_TIMEOUTS_NUM)

#define TIMEOUT_MEM_POOL_BEGIN_ADDR (uint8_t *)(&timeout_mem_pool[0])

#define TIMEOUT_MEM_POOL_END_ADDR (uint8_t *)(&timeout_mem_pool[TIMEOUT_MEM_POOL_SIZE - 1])

#define TIMEOUT_MEM_POOL_SERVICE_BEGIN_IDX TIMEOUT_MEM_POOL_SIZE

#define TIMEOUT_MEM_POOL_SERVICE_END_IDX ((TIMEOUT_MEM_POOL_SIZE + LWCAN_TIMEOUTS_NUM) - 1)

struct lwcan_timeout
{
    struct lwcan_timeout *next;

    uint32_t time;

    lwcan_timeout_handler handler;

    void *arg;
};

static uint8_t timeout_mem_pool[TIMEOUT_MEM_POOL_SIZE + LWCAN_TIMEOUTS_NUM];

static struct lwcan_timeout *next_timeout;

static void *timeout_malloc(void)
{
    for (uint16_t i = TIMEOUT_MEM_POOL_SERVICE_BEGIN_IDX; i <= TIMEOUT_MEM_POOL_SERVICE_END_IDX; i++)
    {
        if (timeout_mem_pool[i] == 0)
        {
            timeout_mem_pool[i] = 0xAA;

            return (void *)&timeout_mem_pool[(i - TIMEOUT_MEM_POOL_SERVICE_BEGIN_IDX)  * TIMEOUT_MEM_CHUNK_SIZE];
        }
    }

    return NULL;
}

static void timeout_free(void *mem)
{
    uint16_t idx;

    uint8_t addr;

    /* Checking an address for inclusion in a memory pool */
    if (mem == NULL || (uint8_t *)mem < TIMEOUT_MEM_POOL_BEGIN_ADDR || (uint8_t *)mem > TIMEOUT_MEM_POOL_END_ADDR)
    {
        return;
    }

    /* get address within memory pool */
    addr = (uint8_t *)mem - TIMEOUT_MEM_POOL_BEGIN_ADDR;

    /* check address for multiple of chunk size */
    if ((addr % TIMEOUT_MEM_CHUNK_SIZE) != 0)
    {
        return;
    }

    /* get the index of the element which means that the chunk has been allocated */
    idx = TIMEOUT_MEM_POOL_SERVICE_BEGIN_IDX + (addr / TIMEOUT_MEM_CHUNK_SIZE);

    /* freeing the chunk */
    timeout_mem_pool[idx] = 0;
}

static bool time_less_than(uint32_t time, uint32_t compare_to)
{
    return (((uint32_t)(time - compare_to)) > MAX_TIMEOUT) ? true : false;
}

void lwcan_timeouts_init(void)
{
    memset(timeout_mem_pool, 0, sizeof(timeout_mem_pool));

    next_timeout = NULL;
}

void lwcan_check_timeouts(void)
{
    uint32_t now;

    struct lwcan_timeout *timeout;

    lwcan_timeout_handler handler;

    void *arg;

    now = system_now();

    do
    {
        timeout = next_timeout;

        if (timeout == NULL)
        {
            return;
        }

        if (time_less_than(now, timeout->time))
        {
            return;
        }

        next_timeout = timeout->next;

        handler = timeout->handler;

        arg = timeout->arg;

        timeout_free(timeout);

        if (handler != NULL)
        {
            handler(arg);
        }

    /* Repeat until all expired timers have been called */
    } while (1);
}

void lwcan_timeout(uint32_t time_ms, lwcan_timeout_handler handler, void *arg)
{
    uint32_t timeout_time;

    struct lwcan_timeout *new_timeout, *timeout;

    new_timeout = (struct lwcan_timeout *)timeout_malloc();

    if (new_timeout == NULL)
    {
        LWCAN_ASSERT("new_timeout != NULL", new_timeout != NULL);

        return;
    }

    timeout_time = (uint32_t)(system_now() + time_ms);

    new_timeout->next = NULL;
    new_timeout->handler = handler;
    new_timeout->arg = arg;
    new_timeout->time = timeout_time;

    if (next_timeout == NULL)
    {
        next_timeout = new_timeout;

        return;
    }

    if (time_less_than(new_timeout->time, next_timeout->time))
    {
        new_timeout->next = next_timeout;

        next_timeout = new_timeout;
    }
    else
    {
        for (timeout = next_timeout; timeout != NULL; timeout = timeout->next)
        {
            if ((timeout->next == NULL) || time_less_than(new_timeout->time, timeout->next->time))
            {
                new_timeout->next = timeout->next;

                timeout->next = new_timeout;

                break;
            }
        }
    }
}

void lwcan_untimeout(lwcan_timeout_handler handler, void *arg)
{
    struct lwcan_timeout *timeout, *previous_timeout;

    if (next_timeout == NULL)
    {
        goto exit;
    }

    for (timeout = next_timeout, previous_timeout = NULL; timeout != NULL; previous_timeout = timeout, timeout = timeout->next)
    {
        if ((timeout->handler == handler) && (timeout->arg == arg))
        {
            if (previous_timeout == NULL)
            {
                next_timeout = timeout->next;
            }
            else
            {
                previous_timeout->next = timeout->next;
            }

            timeout_free(timeout);

            goto exit;
        }
    }

exit:
    return;
}
