/*=========================== LIBRARIES & MACROS ============================*/

#define _XOPEN_SOURCE 700 /* struct sigaction */
#include <stdlib.h>       /* getenv, setenv */
#include <stdatomic.h>    /* atomic_int */
#include <sys/sem.h>      /* semaphore */
#include <signal.h>       /* sigaction */
#include <pthread.h>      /* threads */
#include <stdio.h>        /* printf */
#include <string.h>       /* strcat */
#include <sys/types.h>    /* pid_t */

#include "scheduler.h"
#include "watchdog.h"

#define POST 1
#define FAIL 1
#define WAIT -1
#define CYCLIC 0
#define SUCCESS 0
#define RW_PERMS 0666
#define SEND_INTERVAL 1
#define CHECK_INTERVAL 5
#define MIN_REC_SIGNALS 1
#define EXPECTED_SIGNALS (CHECK_INTERVAL / SEND_INTERVAL)

/*============================== DECLARATIONS ===============================*/

typedef enum logger_level
{
    INFO,
    WARN,
    ERR
} logger_level_t;

typedef void (*handler_func)(int, siginfo_t *, void *);

static void CloseSem();
static void SetHandlers();
static void SetSemId(char *);
static int SignalTask(void *);
static int CheckSig2Task(void *);
static int CheckSig1Task(void *);
static void LogEvent(int, char *);
static int ChangeSemVal(int, int);
static int SetUpScheduler(char **);
static void Revive(char **, char *);
static void *RunAndDestroySched(void *);
static void SetSignalHandler(int, handler_func);
static void ExitOnCondition(int, exit_status_t);
static void Sigusr1Handler(int, siginfo_t *, void *);
static void Sigusr2Handler(int, siginfo_t *, void *);

int is_wd = 0;
static int sem_id;
static pid_t other_pid;
static scheduler_t *sched;
static pthread_t sched_thread;
static atomic_int sig1_counter = 0;
static atomic_int sig2_counter = 0;

/*=========================== FUNCTION DEFINITION ===========================*/

/* Main function of the library, being called explicitly at the beginning
 * of the part of the code that needs to be supported \ restored when crashing,
 * & implicitly every time either users process or watchdog process crashes. */
void WDStart(char **argv)
{
    sigset_t set = {0};
    SetHandlers();

    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);

    SetSemId(argv[0]);
    ExitOnCondition(-1 == sem_id, SEM_ERROR);
    ExitOnCondition(FAIL == SetUpScheduler(argv), SCHED_ERROR);
    /* if will be entered on the first run when being explicitly called
     * by the user, and else will be entered on every revive. */
    if (NULL == getenv("WD_ON"))
    {
        setenv("WD_ON", "1", 1);
        ExitOnCondition(-1 == (other_pid = fork()), FORK_ERROR);

        if (0 == other_pid) /* child process */
        {
            execv("./watchdog.out", argv);
        }
        else /* parent process */
        {
            /* parent calls wait on the semaphore & stops execution
             * untill child calls post and they run scheduler synced */
            ExitOnCondition(-1 == ChangeSemVal(WAIT, sem_id), SEM_ERROR);
        }
    }
    else
    {
        other_pid = getppid();
        /* child calls post on the semaphore & lets parent continue execution */
        ExitOnCondition(-1 == ChangeSemVal(POST, sem_id), SEM_ERROR);
    }
    /* using is_wd to differ between processes, needed b/c watchdog needs
     * to run scheduler on his main thread, while users process needs to
     * run scheduler on another thread, w/o interfering w/ its own code. */
    if (is_wd)
    {
        RunAndDestroySched(sched);
    }
    else
    {
        ExitOnCondition(SUCCESS != pthread_create(&sched_thread, NULL, RunAndDestroySched, sched), THREAD_ERROR);
        /* blocking users process from SIGUSR1/2 to not interfere w/ its execution. */
        pthread_sigmask(SIG_BLOCK, &set, NULL);
    }
}

/* Secondary function of the library, being called explicitly at the end
 * of the part of the code that needs to be supported \ restored when crashing. */
void WDStop(size_t timeout)
{
    time_t start = time(NULL);
    LogEvent(INFO, "Stopping WatchDog");
    if (NULL != sched)
    {
        SchedulerStop(sched);
    }
    CloseSem();
    do
    {
        kill(other_pid, SIGUSR2);
    } while (0 == sig2_counter && (size_t)(time(NULL) - start) < timeout);
    pthread_join(sched_thread, NULL);
}

static void SetHandlers()
{
    SetSignalHandler(SIGUSR1, Sigusr1Handler);
    SetSignalHandler(SIGUSR2, Sigusr2Handler);
    LogEvent(INFO, "Handlers are set");
}

static void SetSignalHandler(int signum, handler_func func)
{
    struct sigaction action = {0};
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = func;
    ExitOnCondition(0 > sigaction(signum, &action, NULL), HANDLER_ERROR);
}

static void Sigusr1Handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;
    /* authenticating pid of sender */
    if (info->si_pid == other_pid)
    {
        atomic_fetch_add(&sig1_counter, 1);
    }
}

static void Sigusr2Handler(int sig, siginfo_t *info, void *context)
{
    (void)sig;
    (void)context;
    /* authenticating pid of sender */
    if (info->si_pid == other_pid)
    {
        atomic_fetch_add(&sig2_counter, 1);
        kill(other_pid, SIGUSR2);
    }
}

static int SignalTask(void *arg)
{
    (void)arg;
    kill(other_pid, SIGUSR1);
    LogEvent(INFO, "SIGUSR1 sent");
    return (CYCLIC);
}

static int CheckSig1Task(void *argv)
{
    if (EXPECTED_SIGNALS != sig1_counter)
    {
        LogEvent(WARN, "Unexpected amount of signals recieved");
    }
    if (MIN_REC_SIGNALS > sig1_counter)
    {
        ExitOnCondition(-1 == (other_pid = fork()), FORK_ERROR);

        if (0 == other_pid) /* child process */
        {
            /* using is_wd to differ between processes, needed b/c whichever
             * proccess is currently in this function will need to revive
             * the other process and become parent. */
            LogEvent(ERR, "Reviving other process");
            if (is_wd)
            {
                Revive((char **)argv, ((char **)argv)[0]);
            }
            else
            {
                Revive((char **)argv, "./watchdog.out");
            }
        }
        /* parent calls wait on the semaphore & stops execution
         * untill child calls post and they run scheduler synced */
        ExitOnCondition(-1 == ChangeSemVal(WAIT, sem_id), SEM_ERROR);
    }
    atomic_fetch_sub(&sig1_counter, sig1_counter);
    return (CYCLIC);
}

static int CheckSig2Task(void *arg)
{
    (void)arg;
    if (sig2_counter > 0)
    {
        SchedulerStop(sched);
    }
    return (CYCLIC);
}

static void Revive(char **argv, char *str)
{
    execv(str, argv);
}

static int SetUpScheduler(char **argv)
{
    sched = SchedulerCreate();

    if (NULL == sched)
    {
        return (FAIL);
    }
    if (FAIL == UIDIsSame(SchedulerAddTask(sched, SignalTask, NULL, SEND_INTERVAL), badUID))
    {
        return (FAIL);
    }
    if (FAIL == UIDIsSame(SchedulerAddTask(sched, CheckSig1Task, argv, CHECK_INTERVAL), badUID))
    {
        return (FAIL);
    }
    if (FAIL == UIDIsSame(SchedulerAddTask(sched, CheckSig2Task, NULL, CHECK_INTERVAL), badUID))
    {
        return (FAIL);
    }
    LogEvent(INFO, "Scheduler is set");
    return (SUCCESS);
}

static void *RunAndDestroySched(void *sched)
{
    SchedulerRun((scheduler_t *)sched);
    SchedulerDestroy((scheduler_t *)sched);
    return (NULL);
}

static int ChangeSemVal(int inc_or_dec, int sem_id)
{
    struct sembuf action = {0};
    action.sem_num = 0;
    action.sem_op = inc_or_dec;
    return (semop(sem_id, &action, 1));
}

static void SetSemId(char *path)
{
    sem_id = semget(ftok(path, 'D'), 1, RW_PERMS | IPC_CREAT);
}

static void CloseSem()
{
    ExitOnCondition(-1 == semctl(sem_id, 0, IPC_RMID), SEM_ERROR);
}

static void ExitOnCondition(int cond, exit_status_t status)
{
    if (cond)
    {
        LogEvent(ERR, "Stopping WatchDog on error");
        WDStop(0);
        exit(status);
    }
}

static void LogEvent(int level, char *msg)
{
    time_t now = time(0);
    struct tm *time = localtime(&now);
    char *identifier = ((is_wd == 1) ? "WatchDog" : "UserProc");
    char *type = (level == ERR) ? "ERROR  " : (level == WARN) ? "WARNING"
                                                              : "INFO   ";

    FILE *logger = fopen("logger.txt", "a");
    fprintf(logger, "[%2d:%2d:%2d] %s | %s | %s\n", time->tm_hour, time->tm_min, time->tm_sec, identifier, type, msg);
    fclose(logger);
}