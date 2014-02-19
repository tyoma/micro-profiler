@echo off
setlocal
for /f "tokens=3" %%g in ('type %1 ^| findstr /L "Version Major: .*"') do set VerMajor=%%g
for /f "tokens=3" %%g in ('type %1 ^| findstr /L "Version Minor: .*"') do set VerMinor=%%g
for /f "tokens=2" %%g in ('svn info %2 ^| findstr /L "Revision: .*"') do set VerRevision=%%g
echo %VerMajor%.%VerMinor%.%VerRevision%
endlocal
