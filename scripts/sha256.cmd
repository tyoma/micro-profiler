@echo off

for /f "skip=1 delims=" %%i in ('certutil -hashfile %2 SHA256') do set "%1=%%i" & goto :end
:end
