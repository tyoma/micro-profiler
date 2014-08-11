@echo off

for /f "tokens=3" %%g in ('type %2 ^| findstr /L "Version Major: .*"') do set VerMajor=%%g
for /f "tokens=3" %%g in ('type %2 ^| findstr /L "Version Minor: .*"') do set VerMinor=%%g
for /f "tokens=2" %%g in ('svn info %3 ^| findstr /L "Revision: .*"') do set VerRevision=%%g
set "%1=%VerMajor%.%VerMinor%.%VerRevision%"
