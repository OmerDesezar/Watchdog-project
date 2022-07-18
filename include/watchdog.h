#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#include <stddef.h> /* size_t */

typedef enum exit_status
{
    SEM_ERROR = 1,
    FORK_ERROR,
    SCHED_ERROR,
    THREAD_ERROR,
    HANDLER_ERROR
} exit_status_t;

extern int is_wd;

void WDStart(char **);
void WDStop(size_t);

#endif /* __WATCHDOG_H__ */
