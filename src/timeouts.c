#include "lwcan/timeouts.h"
#include "lwcan/error.h"
#include "lwcan/system.h"
#include "lwcan/options.h"

#include <stdbool.h>
#include <string.h>

#define MAX_TIMEOUTS_NUM 10

#define MAX_TIMEOUT 0x7fffffff

#define TIMEOUT_MEM_CHUNK_SIZE sizeof(struct timeouts)

#define TIMEOUT_MEM_POOL_SIZE ((TIMEOUT_MEM_CHUNK_SIZE * MAX_TIMEOUTS_NUM) + MAX_TIMEOUTS_NUM)

struct timeouts
{
    struct timeouts *next;

    uint32_t time;

    void (*handler)(void *);

    void *arg;
};

static uint8_t timeout_mem_pool[TIMEOUT_MEM_POOL_SIZE] = {0};

static struct timeouts *next_timeout = NULL;

static void *timeout_malloc(void)
{
    for (uint16_t i = 0; i < TIMEOUT_MEM_POOL_SIZE; i += (TIMEOUT_MEM_CHUNK_SIZE + 1))
    {
        if (timeout_mem_pool[i + TIMEOUT_MEM_CHUNK_SIZE] == 0)
        {
            timeout_mem_pool[i + TIMEOUT_MEM_CHUNK_SIZE] = 0xAA;

            return &timeout_mem_pool[i];
        }
    }

    return NULL;
}

static void timeout_free(void *mem)
{
    if (mem == NULL)
    {
        return;
    }

    if (((uint8_t *)mem > &timeout_mem_pool[TIMEOUT_MEM_POOL_SIZE - TIMEOUT_MEM_CHUNK_SIZE - 1]) || ((uint8_t *)mem < &timeout_mem_pool[0]))
    {
        return;
    }

    ((uint8_t *)mem)[TIMEOUT_MEM_CHUNK_SIZE] = 0;
}

static bool time_less_than(uint32_t time, uint32_t compare_to)
{
    return (((uint32_t)(time - compare_to)) > MAX_TIMEOUT) ? true : false;
}

void lwcan_check_timeouts(void)
{
    struct timeouts *timeout;

    void (*handler)(void *);

    void *arg;

    uint32_t now = system_now();

    do
    {
        timeout = next_timeout;

        if (timeout == NULL)
        {
            goto exit;
        }

        if (time_less_than(now, timeout->time))
        {
            goto exit;
        }

        next_timeout = timeout->next;

        handler = timeout->handler;

        arg = timeout->arg;

        timeout_free(timeout);

        if (handler != NULL)
        {
            handler(arg);
        }

    } while (1);

exit:
    return;
}

void lwcan_timeout(uint32_t time_ms, void (*handler)(void *), void *arg)
{
    struct timeouts *new_timeout;

    struct timeouts *timeout;

    uint32_t now = system_now();

    uint32_t timeout_time = (uint32_t)(now + time_ms);

    new_timeout = (struct timeouts *)timeout_malloc();

    if (new_timeout == NULL)
    {
        return;
    }

    new_timeout->next = NULL;

    new_timeout->time = timeout_time;

    new_timeout->handler = handler;

    new_timeout->arg = arg;

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

void lwcan_untimeout(void (*handler)(void *), void *arg)
{
    struct timeouts *timeout;

    struct timeouts *previous_timeout;

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
