if exist "7z.exe" (
	set arcihver="7z.exe"
	goto use7z
)
if exist "%ProgramFiles%\7-Zip\7z.exe" (
	set arcihver="%ProgramFiles%\7-Zip\7z.exe"
	goto use7z
)
if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" (
	set arcihver="%ProgramFiles(x86)%\7-Zip\7z.exe"
	goto use7z
)
if exist "%ProgramW6432%\7-Zip\7z.exe" (
	set arcihver="%ProgramW6432%\7-Zip\7z.exe"
	goto use7z
)
if exist "rar.exe" (
	set arcihver="rar.exe"
	goto userar
)
if exist "%ProgramFiles%\WinRar\rar.exe" (
	set arcihver="%ProgramFiles%\WinRar\rar.exe"
	goto userar
)
if exist "%ProgramFiles(x86)%\WinRar\rar.exe" (
	set arcihver="%ProgramFiles(x86)%\WinRar\rar.exe"
	goto userar
)
if exist "%ProgramW6432%\WinRar\rar.exe" (
	set arcihver="%ProgramW6432%\WinRar\rar.exe"
	goto userar
)
goto noarc

:use7z
%arcihver% a -tzip -mx9 %2 %1
goto exit

:userar
%arcihver% a -ep1 -m5 %2 %1
goto exit

:noarc
:exit
