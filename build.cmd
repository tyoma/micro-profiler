@echo off

setlocal

set WSLENV=NDK/p

echo 1. Preparing repository...
git pull
git submodule update --init
git diff --exit-code
if %errorlevel% neq 0 goto :errornonstaged
git diff --cached --exit-code
if %errorlevel% neq 0 goto :errornoncommitted

echo 2. Incrementing build number...
call :incrementfield version.h MP_BUILD

echo 3. Committing updated build version and store its hash...
git add version.h
for /f "tokens=2 delims=[] " %%g in ('git commit -m "New build number..." ^| findstr /i "\[.*\].*"') do set commithash=%%g

echo 4. Pushing the updated version...
for /f %%g in ('git push 2^>^&1 ^| findstr /i "\[rejected\]"') do goto :pushrejected

echo 5. Resetting to a build revision (one before 'commithash')...
git reset --hard %commithash%~1

echo 6. Building micro-profiler...
wsl bash ./build.sh
call build-windows
call build-install
call build-vsix

echo 7. Tagging...
call scripts\make-version VERSION_TAG version.h
git tag -a -m "" v%VERSION_TAG% %commithash%~1
git push origin v%VERSION_TAG%

echo 8. Resetting to origin...
git reset --hard %commithash%

echo Build complete!

goto :end

:incrementfield
	del /q "%~1.tmp"
	for /f "tokens=1,2,3*" %%i in (%1) do call :incrementfieldline %2 "%%i" "%%j" "%%k" "%%l" >> "%~1.tmp"
	move /y "%~1.tmp" %1
	exit /b 0

:incrementfieldline
	@set value=%~4
	@if %~1==%~3 set /a value+=1
	@echo %~2 %~3 %value% %~5
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

