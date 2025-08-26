!include "MUI2.nsh"

!ifndef VERSION
  !define VERSION "0.0.0"
!endif

!ifndef NSISDIR
  !error "NSIS tools not found. Install makensis and ensure it is available in your PATH."
!endif

!ifexist "dist\\FreeCrafter.exe"
!else
  !error "Required binary dist\\FreeCrafter.exe not found. Build the project before packaging."
!endif

Name "FreeCrafter"
OutFile "FreeCrafter-${VERSION}.exe"
InstallDir "$PROGRAMFILES\\FreeCrafter"

Page Directory
Page InstFiles
Page UninstConfirm
Page UninstInstFiles

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "dist\\*"
  CreateShortcut "$SMPROGRAMS\\FreeCrafter.lnk" "$INSTDIR\\FreeCrafter.exe"
  CreateShortcut "$DESKTOP\\FreeCrafter.lnk" "$INSTDIR\\FreeCrafter.exe"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\\FreeCrafter.lnk"
  Delete "$DESKTOP\\FreeCrafter.lnk"
  RMDir /r "$INSTDIR"
SectionEnd
