SET compiler=gcc
@REM %compiler% -Wall -Wextra -g src/main_win32.c -o bomberman | ./bomberman
gcc -Wall -Wextra -ggdb src/main_win32.c -l"gdi32" -l"winmm" -o bomberman | ./bomberman

@REM bomberman

timeout /t 30