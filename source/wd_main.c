#include "watchdog.h"

int main(int argc, char *argv[])
{
    is_wd = 1;
    WDStart(argv);
    (void)argc;
    return 0;
}
