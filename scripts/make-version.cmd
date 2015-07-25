@echo off

for /f "tokens=3" %%g in ('type %2 ^| findstr /L "Version Major: .*"') do set VerMajor=%%g
for /f "tokens=3" %%g in ('type %2 ^| findstr /L "Version Minor: .*"') do set VerMinor=%%g
for /f "tokens=2" %%g in ('type %2 ^| findstr /L "Build: .*"') do set VerBuild=%%g
set "%1=%VerMajor%.%VerMinor%.%VerBuild%"
