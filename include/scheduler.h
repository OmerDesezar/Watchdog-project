#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "task.h"
#include "priorityq.h"


typedef enum return_type
{
	success = 0,
	fail,
	stop_run
}return_type_t;

typedef struct scheduler scheduler_t;

/* DESCRIPTION:
 * Function creates an empty scheduler
 *
 * PARAMS:
 * void
 *         
 * RETURN:
 * Returns a pointer to the created scheduler
 *
 * COMPLEXITY:
 * time: best - O(1), worst - indeterminable
 * space: O(1)
 */
scheduler_t* SchedulerCreate(void);

/* DESCRIPTION:
 * Function destroys and performs cleanup on the given scheduler.
 * passing an invalid scheduler pointer would result in undefined behaviour
 * This function should be used when finshing using the scheduler.
 *
 * PARAMS:
 * scheduler - pointer to the scheduler to be destroyed
 *         
 * RETURN:
 * void
 *
 * COMPLEXITY:
 * time: O(n)
 * space: O(1)
 */
void SchedulerDestroy(scheduler_t *scheduler);

/* DESCRIPTION:
 * Function creates and inserts a new to the scheduler in the correct position.
 * 
 * PARAMS:
 * scheduler             - pointer to the scheduler to be destroyed
 * func, param, interval - for tasks creation     
 *
 * COMPLEXITY:
 * time: O(n) 
 * space: O(1)
 */
UID_t SchedulerAddTask(scheduler_t *scheduler, action_func *func, void* param, size_t interval_in_seconds);

/* DESCRIPTION:
 * Function removes a task from the scheduler and returns success\fail
 * trying to remove from an empty scheduler will result in undefined behavior
 *
 * PARAMS:
 * scheduler - pointer to the scheduler to be destroyed
 * uid       - a tasks identifier
 *         
 * RETURN:
 * success \ fail
 *
 * COMPLEXITY:
 * time: O(n) 
 * space: O(1)
 */
int SchedulerRemoveTask(scheduler_t *scheduler, UID_t uid);

/* DESCRIPTION:
 * Function clears the scheduler.
 * passing an invalid scheduler would result in undefined behaviour.
 * passing an empty scheduler would result in undefined behaviour.
 *
 * PARAMS:
 * scheduler - pointer to the scheduler to be destroyed
 *         
 *
 * COMPLEXITY:
 * time: O(n)
 * space: O(1)
 */
void SchedulerClear(scheduler_t *scheduler);

int SchedulerRun(scheduler_t *scheduler);

void SchedulerStop(scheduler_t *scheduler);

size_t SchedulerSize(scheduler_t *scheduler);

int SchedulerIsEmpty(scheduler_t *scheduler);

#endif /*__SCHEDULER_H__*/



