@echo off
setlocal ENABLEDELAYEDEXPANSION

for /f "delims=" %%i in ('type %1') do (
	echo %%i%
 )
 