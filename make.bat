@echo off

if "--%1%--" == "----" goto make-j1
if "--%1%--" == "--j1--" goto make-j1
goto unknown

:make-j1
set output=j1
set c-files=j1-emu.c
set c-files=%c-files% j1-main.c io-ports.c
echo making %output% ...
echo gcc -o %output% %c-files%
gcc -o %output% %c-files%
if "--%2%--" == "----" goto done
j1 -f:j1 -c:1000
goto done

:unknown
echo Unknown make. I know how to make these:
echo.
echo    j1 - makes j1.exe
echo.
echo    NOTE: if arg2 is given it then runs the program

:done
