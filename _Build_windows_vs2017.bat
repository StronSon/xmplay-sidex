

@echo OFF 
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
echo "Starting Build for all Projects with proposed changes"
echo .  
rem devenv "C:\xmplaysid\xmp-sid\xmp-sid.sln" /rebuild release
devenv "C:\xmplaysid\xmp-sid\xmp-sid.sln" /build release

echo . 
echo "All builds completed." 
pause
pause
