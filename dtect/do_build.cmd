@echo off
set PATH=%PATH%;C:\appman\apps\ninja\
echo "Building"

buildconsole /COMMAND="ninja -j70" /PROFILE=CMakeModules/increadibuild_profile.xml
