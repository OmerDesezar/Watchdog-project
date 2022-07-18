#ifndef __TASK_H__
#define __TASK_H__

#include "UID.h"


typedef int(action_func)(void *param);

typedef struct task task_t;

/* DESCRIPTION:
 * Function creates a new task
 *
 * PARAMS:
 * func                - the tasks action
 * interval_in_seconds - interval between runs when scheduler is running
 * param               - some input for func
 *         
 * RETURN:
 * Returns a pointer to the created task
 *
 * COMPLEXITY:
 * time: best - O(1), worst - indeterminable
 * space: O(1)
 */
task_t* TaskCreate(action_func *func, size_t interval_in_seconds, void *param);

/* DESCRIPTION:
 * Function destroys the given task.
 * passing an invalid task pointer would result in undefined behaviour
 * This function should be used when finished using the task.
 *
 * PARAMS:
 * task - pointer to the task to be destroyed
 *         
 * RETURN:
 * void
 *
 * COMPLEXITY:
 * time: O(1)
 * space: O(1)
 */
void TaskDestroy(task_t *task);

int TaskRun(task_t *task);

UID_t TaskGetUID(const task_t *task);

int TaskCompare(const task_t *task, UID_t uid);

time_t TaskGetNextRunTime(const task_t *task);

void TaskUpdateNextRunTime(task_t *task);

#endif /*__TASK_H__*/


