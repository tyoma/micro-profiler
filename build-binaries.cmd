set WSLENV=NDK/p

wsl bash ./build-linux.sh
call build-windows
call build-install
call build-vsix
