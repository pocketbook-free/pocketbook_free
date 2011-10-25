#ifndef PLATFORM_LAB126_H
#define PLATFORM_LAB126_H

#ifdef CONFIG_ARCH_FIONA    //------------- Fiona (Linux 2.6.10) Build

#define THREAD_ID(i)                                    pid_t i = 0
#define THREAD_START(i, f, t, n)                        \
    i = kernel_thread(t, (void *)f, CLONE_KERNEL);      \
    if ( 0 > i )                                        \
        i = 0
#define THREAD_STOP(i, g, e)                            \
    g();                                                \
    if ( 0 < i )                                        \
        if ( 0 == kill_proc(i, SIGKILL, 1) )            \
            wait_for_completion(&e)

#define THREAD_HEAD(n)                                  \
    daemonize(n);                                       \
    allow_signal(SIGKILL)
#define THREAD_BODY(f, g)                               \
    if ( !(f) )                                         \
        g();                                            \
    else                                                \
        schedule_timeout_interruptible(MAX_SCHEDULE_TIMEOUT)
#define THREAD_TAIL(e)                                  complete_and_exit(&e, 0);
#ifdef  CONFIG_PM
#define TRY_TO_FREEZE()                                 \
    if ( current->flags & PF_FREEZE )                   \
        refrigerator(PF_FREEZE)
#else
#define TRY_TO_FREEZE()
#endif
#define THREAD_SHOULD_STOP()                            signal_pending(current)

#define schedule_timeout_interruptible(t)               \
{                                                       \
    set_current_state(TASK_INTERRUPTIBLE);              \
    schedule_timeout(t);                                \
}

#else   // -------------------------------- Mario (Linux 2.6.22) Build

#include <linux/freezer.h>
#include <linux/kthread.h>

#define THREAD_ID(i)                                    struct task_struct *i = NULL
#define THREAD_START(i, f, t, n)                        \
    i = kthread_run(t, (void *)f, n);                   \
    if ( IS_ERR(i) )                                    \
        i = NULL
#define THREAD_STOP(i, g, e)                            \
    g();                                                \
    if ( i )                                            \
        kthread_stop(i)
#define THREAD_HEAD(n)
#define THREAD_BODY(f, g)                               \
    if ( !(f) )                                         \
        g();                                            \
    else                                                \
        schedule_timeout_interruptible(MAX_SCHEDULE_TIMEOUT)
#define THREAD_TAIL(e)                                  return ( 0 )
#define TRY_TO_FREEZE()                                 try_to_freeze()
#define THREAD_SHOULD_STOP()                            kthread_should_stop()

#endif

#endif // PLATFORM_LAB126_H
