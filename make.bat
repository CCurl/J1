@echo off

if "--%1%--" == "----" goto make-j1
if "--%1%--" == "--fc--" goto make-fc
if "--%1%--" == "--j1--" goto make-j1
goto unknown

:make-fc
set output=forth-compiler
set c-files=forth-compiler.c 
set c-files=%c-files% forth-vm.c
echo making %output% ...
echo gcc -g -o %output% %c-files%
del forth-compiler.exe
gcc -g -o %output% %c-files%
if "--%2%--" == "--1--" forth-compiler
goto done

:make-all
call make fc
call make j1

if "--%2%--" == "----" goto done
j1
goto done

:make-j1
set output=j1
set c-files=j1-emu.c
set c-files=%c-files% j1-main.c
echo making %output% ...
echo gcc -o %output% %c-files%
gcc -o %output% %c-files%
if "--%2%--" == "--1--" j1
goto done

:unknown
echo Unknown make. I know how to make these:
echo.
echo    fc - makes forth-compiler.exe
echo    j1 - makes j1.exe
echo.
echo    NOTE: if arg2 is given it then runs the program

:done
