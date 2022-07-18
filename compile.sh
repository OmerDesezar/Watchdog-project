#!/bin/bash

gcc -ansi -I include -pedantic-errors -Wall -Wextra -g source/watchdog.c source/wd_main.c -fPIC -lsched -L. -Wl,-rpath="\$ORIGIN" -lpthread -o watchdog.out

gcc -ansi -I include -pedantic-errors -Wall -Wextra -g source/watchdog.c test/user_app.c -fPIC -lsched -L. -Wl,-rpath="\$ORIGIN" -lpthread -o user.out
