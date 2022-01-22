set PATH=%PATH%;%~dp0scripts;%WIX%bin
set OUTDIR=%~dp0_setup/

call make-version.cmd VERSION version.h

pushd deployment\wix

set OBJDIR=%~dp0_obj\installer.x86/
candle^
 *.wxs^
 -dROOTDIR="%~dp0\"^
 -dSOURCEDIR="%~dp0_build.windows.x86\_bin"^
 -dSOURCEDIRWX86="%~dp0_build.windows.x86\_bin"^
 -dSOURCEDIRWX64="%~dp0_build.windows.x64\_bin"^
 -dSOURCEDIRLINUXX86="%~dp0_build.linux.x86\_bin"^
 -dSOURCEDIRLINUXX64="%~dp0_build.linux.x64\_bin"^
 -arch x86^
 -ext "%WIX%bin\WixUIExtension.dll"^
 -ext "%WIX%bin\WixVSExtension.dll"^
 -o "%OBJDIR%"
light^
 "%OBJDIR%*.wixobj"^
 -out "%OUTDIR%micro-profiler_x86.v%VERSION%.msi"^
 -pdbout "%OUTDIR%micro-profiler_x86.pdbout"^
 -cultures:null^
 -ext "%WIX%bin\WixUIExtension.dll"^
 -ext "%WIX%bin\WixVSExtension.dll"

set OBJDIR=%~dp0_obj\installer.x64/
candle^
 *.wxs^
 -dROOTDIR="%~dp0\"^
 -dSOURCEDIR="%~dp0_build.windows.x64\_bin"^
 -dSOURCEDIRWX86="%~dp0_build.windows.x86\_bin"^
 -dSOURCEDIRWX64="%~dp0_build.windows.x64\_bin"^
 -dSOURCEDIRLINUXX86="%~dp0_build.linux.x86\_bin"^
 -dSOURCEDIRLINUXX64="%~dp0_build.linux.x64\_bin"^
 -arch x64^
 -ext "%WIX%bin\WixUIExtension.dll"^
 -ext "%WIX%bin\WixVSExtension.dll"^
 -o "%OBJDIR%"
light^
 "%OBJDIR%*.wixobj"^
 -out "%OUTDIR%micro-profiler_x64.v%VERSION%.msi"^
 -pdbout "%OUTDIR%micro-profiler_x64.pdbout"^
 -cultures:null^
 -ext "%WIX%bin\WixUIExtension.dll"^
 -ext "%WIX%bin\WixVSExtension.dll"

popd
