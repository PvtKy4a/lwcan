#include "lwcan/timeouts.h"
#include "lwcan/private/timeouts_private.h"
#include "lwcan/error.h"
#include "lwcan/system.h"
#include "lwcan/options.h"
#include "lwcan/debug.h"

#include <stdbool.h>
#include <string.h>

#define MAX_TIMEOUT 0x7fffffff

#define TIME_LESS_THAN(time, compare_to) ((((uint32_t)(time - compare_to)) > MAX_TIMEOUT) ? 1 : 0)

static struct lwcan_timeout *next_timeout = 0;

#if !LWCAN_NO_SYSTEM

static system_thread_t lwcan_thread;

static void lwcan_time(void *arg)
{
    (void)arg;

    uint32_t wait;

    while (1)
    {
        wait = lwcan_timeouts_handler();

        system_msleep(wait);
    }
}

#endif

static inline void timeout_add(struct lwcan_timeout *timeout)
{
    struct lwcan_timeout *timeout_temp;

    if (next_timeout == NULL)
    {
        next_timeout = timeout;

        return;
    }

    if (TIME_LESS_THAN(timeout->time, next_timeout->time))
    {
        timeout->next = next_timeout;

        next_timeout = timeout;
    }
    else
    {
        for (timeout_temp = next_timeout; timeout_temp != NULL; timeout_temp = timeout_temp->next)
        {
            if ((timeout_temp->next == NULL) || TIME_LESS_THAN(timeout->time, timeout_temp->next->time))
            {
                timeout->next = timeout_temp->next;

                timeout_temp->next = timeout;

                break;
            }
        }
    }
}

static inline void timeout_remove(struct lwcan_timeout *timeout)
{
    struct lwcan_timeout *timeout_current, *timeout_previous;

    if (next_timeout == NULL)
    {
        return;
    }

    for (timeout_current = next_timeout, timeout_previous = NULL; timeout_current != NULL; timeout_previous = timeout_current, timeout_current = timeout_current->next)
    {
        if (timeout_current == timeout)
        {
            if (timeout_previous == NULL)
            {
                next_timeout = timeout_current->next;
            }
            else
            {
                timeout_previous->next = timeout_current->next;
            }

            break;
        }
    }
}

void lwcan_timeouts_init(void)
{
#if !LWCAN_NO_SYSTEM
    lwcan_thread = system_thread_new(LWCAN_THREAD_NAME, lwcan_time, NULL, LWCAN_THREAD_STACKSIZE, LWCAN_THREAD_PRIO);
#endif
}

uint32_t lwcan_timeouts_handler(void)
{
    uint32_t now;

    struct lwcan_timeout *timeout;

    lwcan_timeout_handler handler;

    void *arg;

    SYSTEM_ARCH_DECL_PROTECT(lev);

    now = system_now();

    do
    {
        SYSTEM_ARCH_PROTECT(lev);

        timeout = next_timeout;

        if (timeout == NULL)
        {
            SYSTEM_ARCH_UNPROTECT(lev);

            return MAX_TIMEOUT;
        }

        if (TIME_LESS_THAN(now, timeout->time))
        {
            SYSTEM_ARCH_UNPROTECT(lev);

            return (uint32_t)(timeout->time - now);
        }

        next_timeout = timeout->next;

        handler = timeout->handler;

        arg = timeout->arg;

        SYSTEM_ARCH_UNPROTECT(lev);

        if (handler != NULL)
        {
            handler(arg);
        }

    /* Repeat until all expired timers have been called */
    } while (1);
}

void lwcan_timeout(struct lwcan_timeout *timeout, uint32_t time_ms, lwcan_timeout_handler handler, void *arg)
{
    if (timeout == NULL)
    {
        LWCAN_ASSERT("timeout != NULL", timeout != NULL);

        return;
    }

    SYSTEM_ARCH_DECL_PROTECT(lev);

    SYSTEM_ARCH_PROTECT(lev);

    timeout_remove(timeout);

    timeout->next = NULL;
    timeout->handler = handler;
    timeout->arg = arg;
    timeout->time = system_now() + time_ms;

    timeout_add(timeout);

    SYSTEM_ARCH_UNPROTECT(lev);

    system_wakeup(&lwcan_thread);
}

void lwcan_untimeout(struct lwcan_timeout *timeout)
{
    SYSTEM_ARCH_DECL_PROTECT(lev);

    SYSTEM_ARCH_PROTECT(lev);

    timeout_remove(timeout);

    SYSTEM_ARCH_UNPROTECT(lev);

    system_wakeup(&lwcan_thread);
}
