@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 10.0.26100.0
set CMAKE_PREFIX_PATH=C:\Users\craio\Documents\Workspaces\FreeCrafter-main\qt\6.5.3\msvc2019_64
set CMAKE_MT=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\mt.exe
python scripts\bootstrap.py
