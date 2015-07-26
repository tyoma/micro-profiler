rem 1.+pull
rem 2.+increment build number
rem 3.+commit / store hash
rem 4. push
rem 5.+checkout at one step before stored hash
rem 6. tag
rem 7. build

rem 1. Prepare repository
git pull
git diff --exit-code
if %errorlevel% neq 0 goto :errornonstaged
git diff --cached --exit-code
if %errorlevel% neq 0 goto :errornoncommitted

rem 2. Increment build number...
call :incrementfield version-info.txt "Build"

rem 3. Make a commit and store its hash...
git add version-info.txt
for /f "tokens=2 delims=[] " %%g in ('git commit -m "New build number..." ^| findstr /i "\[.*\].*"') do set commithash=%%g

rem 4. Push the updated version...

rem 5. Checkout a build revision (one before 'commithash')...
git checkout %commithash%~1

rem 7. Start the build...

goto :end

:incrementfield
	set tmp="%~1.tmp"
	del /q %tmp%
	for /f "tokens=1,2 delims=:" %%i in (%1) do call :incrementfieldline "%%i" %%j %2 >> %tmp%
	move /y %tmp% %1
exit /b 0

:incrementfieldline
	@echo off
	set /a value=%2
	if %1==%3 set /a value=value+1
	echo %~1: %value%
exit /b 0

:errornonstaged
echo Local repository contains unstaged changes...
goto :end

:errornoncommitted
echo Local repository contains not-committed changes...
goto :end

:end

