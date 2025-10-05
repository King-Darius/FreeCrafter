@echo off
setlocal
set ROOT=%~dp0..
set BUILD=%ROOT%\build
if not "%1"=="" set BUILD=%~1
set QT_BIN=%ROOT%\qt\6.5.3\msvc2019_64\bin
if exist "%QT_BIN%" set PATH=%QT_BIN%;%PATH%
where ctest >nul 2>nul
if errorlevel 1 (
  echo CTest was not found on PATH. Run from the Visual Studio Developer Prompt or add CMake/bin to PATH.
  exit /b 1
)

where test_render.exe >nul 2>nul
if errorlevel 1 (
  if not exist "%BUILD%\test_render.exe" (
    echo Test binary not found at %BUILD%\test_render.exe
    echo Configure the project with: cmake -S %ROOT% -B %BUILD%
    echo Then build or install it with: cmake --build %BUILD% --config Release
    exit /b 1
  )
)

if not exist "%BUILD%\test_render.exe" (
  echo Test binary not found at %BUILD%\test_render.exe
  echo Build the tree first with: cmake --build %BUILD% --config Release
  exit /b 1
)
"%BUILD%\test_render.exe" %*
exit /b %ERRORLEVEL%
