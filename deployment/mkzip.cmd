if exist "%ProgramFiles%\7-Zip\7z.exe" (
	"%ProgramFiles%\7-Zip\7z.exe" a -tzip "%2" "%1"
) else (
	if exist "%ProgramFiles%\WinRar\rar.exe" (
		"%ProgramFiles%\WinRar\rar.exe" a -ep1 "%2" "%1"
	)
)