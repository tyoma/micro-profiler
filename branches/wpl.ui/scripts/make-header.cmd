@echo off

for /f "delims=`" %%i in ('%2 %3 %4 %5') do (call :process-info %1 "%%i")
goto :end

:process-info
setlocal
set x=%~2
set resplitted=%x:: =`%
for /f "delims=` tokens=1,2" %%i in ("%resplitted%") do (call :dump-macro-resplitted %1 "%%i" "%%j")
endlocal
exit /b 0

:dump-macro-resplitted
setlocal
set name=%~2
call :make-upper-case-macro name
echo #define %1_%name% %~3
echo #define %1_%name%_STR "%~3"
endlocal
exit /b 0

:make-upper-case-macro
for %%i in (" =_" "a=A" "b=B" "c=C" "d=D" "e=E" "f=F" "g=G" "h=H" "i=I" "j=J" "k=K" "l=L" "m=M" "n=N" "o=O" "p=P" "q=Q" "r=R" "s=S" "t=T" "u=U" "v=V" "w=W" "x=X" "y=Y" "z=Z") do call set "%1=%%%1:%%~i%%"
exit /b 0

:end
