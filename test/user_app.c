#define _XOPEN_SOURCE 700 /* kill */
#include <stdlib.h>       /* atoi */
#include <time.h>         /* time */
#include <stdio.h>        /* printf */
#include <unistd.h>       /* fork/exec */
#include <sys/wait.h>     /* SIGSEGV, waitpid */

#include "watchdog.h"

static void DreamSleep(int sec);
static void TestWD(char **argv);

int main(int argc, char **argv)
{
    (void)argc;
    TestWD(argv);
    return (0);
}

static void DreamSleep(int sec)
{
    size_t counter = 0;
    time_t now = time(NULL);
    while (time(NULL) < now + sec)
    {
        printf("in loop, %ld\n", counter++);
        sleep(1);
    }
    return;
}

static void TestWD(char **argv)
{
    pid_t pid;
    int status;

    if (NULL == getenv("WD_ON"))
    {
        pid = fork();
        if (0 == pid)
        {
            printf(" ~ User App running ~\n");
            sleep(1);
            WDStart(argv);
            DreamSleep(10);
            WDStop(5);
            sleep(2);
            printf("Finished, kill did not take place\n");
        }
        else
        {
            sleep(5);
            printf(" ~ User app killed ~\n");
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
        }
    }
    else
    {
        printf("\n ~ User app revived by WD ~\n");
        system("ps");
        WDStart(argv);
        sleep(3);
        printf(" ~ WD process killed ~\n");
        kill(getppid(), SIGINT);
        system("ps");
        sleep(10);
        printf(" ~ WD process revived by User app ~\n");
        system("ps");
        WDStop(5);
        sleep(2);
        printf(" ~ User app finished executing ~\n");
    }
}
