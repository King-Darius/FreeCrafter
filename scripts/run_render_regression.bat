@echo off
setlocal
set ROOT=%~dp0..
set BUILD=%ROOT%\build
if not "%1"=="" set BUILD=%~1
set QT_BIN=%ROOT%\qt\6.5.3\msvc2019_64\bin
if exist "%QT_BIN%" set PATH=%QT_BIN%;%PATH%
if not exist "%BUILD%\test_render.exe" (
  echo Test binary not found at %BUILD%\test_render.exe
  exit /b 1
)
"%BUILD%\test_render.exe" %*
exit /b %ERRORLEVEL%
