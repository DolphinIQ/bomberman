SET compiler=gcc
@REM %compiler% -Wall -Wextra -g src/main.c -o bomberic | /.bomberic
gcc -Wall -Wextra -ggdb src/main.c -l"gdi32" -l"winmm" -o bomberic | /.bomberic

@REM bomberic

timeout /t 30