rem 6. tag
rem 7. build

@echo off

echo 1. Preparing repository...
git pull
git diff --exit-code
if %errorlevel% neq 0 goto :errornonstaged
git diff --cached --exit-code
if %errorlevel% neq 0 goto :errornoncommitted

echo 2. Incrementing build number...
call :incrementfield version-info.txt "Build"

echo 3. Committing updated build version and store its hash...
git add version-info.txt
for /f "tokens=2 delims=[] " %%g in ('git commit -m "New build number..." ^| findstr /i "\[.*\].*"') do set commithash=%%g

echo 4. Pushing the updated version...
for /f %%g in ('git push 2^>^&1 ^| findstr /i "\[rejected\]"') do goto :pushrejected

echo 5. Checking out a build revision (one before 'commithash')...
git checkout %commithash%~1

rem 6. Tagging...

echo 7. Starting the build...

goto :end

:incrementfield
	set tmp="%~1.tmp"
	del /q %tmp%
	for /f "tokens=1,2 delims=:" %%i in (%1) do call :incrementfieldline "%%i" %%j %2 >> %tmp%
	move /y %tmp% %1
	exit /b 0

:incrementfieldline
	@set /a value=%2
	@if %1==%3 set /a value=value+1
	@echo %~1: %value%
	@exit /b 0

:pushrejected
	echo Remote repository was updated while incrementing the build version...
	goto :end

:errornonstaged
	echo Local repository contains unstaged changes...
	goto :end

:errornoncommitted
	echo Local repository contains not-committed changes...
	goto :end

:end

