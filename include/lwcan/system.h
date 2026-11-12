#ifndef LWCAN_SYSTEM_H
#define LWCAN_SYSTEM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#if LWCAN_NO_SYSTEM

#define system_thread_new(n,t,a,s,p)

#define system_msleep(t)

#define system_wakeup(t)

#else

#include "lwcan/error.h"
#include "lwcanarch/system_arch.h"

/** Function prototype for thread functions */
typedef void (*lwcan_thread_fn)(void *arg);

/**
 * The only thread function:
 * Starts a new thread named "name" with priority "prio" that will begin its
 * execution in the function "thread()". The "arg" argument will be passed as an
 * argument to the thread() function. The stack size to used for this thread is
 * the "stacksize" parameter. The id of the new thread is returned. Both the id
 * and the priority are system dependent.
 * ATTENTION: although this function returns a value, it MUST NOT FAIL (ports have to assert this!)
 *
 * @param name human-readable name for the thread (used for debugging purposes)
 * @param thread thread-function
 * @param arg parameter passed to 'thread'
 * @param stacksize stack size in bytes for the new thread (may be ignored by ports)
 * @param prio priority of the new thread (may be ignored by ports) */
system_thread_t system_thread_new(const char *name, lwcan_thread_fn thread, void *arg, int stacksize, int prio);

#endif

uint32_t system_now(void);

system_prot_t system_arch_protect(void);

void system_arch_unprotect(system_prot_t pval);

#ifdef __cplusplus
}
#endif

#endif /* LWCAN_SYSTEM_H */
