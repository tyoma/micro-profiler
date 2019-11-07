@echo off

for /f "tokens=3" %%g in ('type "%2" ^| findstr "MP_VERSION_MAJOR.*[0-9][0-9]*"') do set v=%%g
for /f "tokens=3" %%g in ('type "%2" ^| findstr "MP_VERSION_MINOR.*[0-9][0-9]*"') do set v=%v%.%%g
for /f "tokens=3" %%g in ('type "%2" ^| findstr "MP_BUILD.*[0-9][0-9]*"') do set v=%v%.%%g
for /f "tokens=3" %%g in ('type "%2" ^| findstr "MP_BRANCH.*[0-9][0-9]*"') do set v=%v%.%%g

set "%1=%v%"
