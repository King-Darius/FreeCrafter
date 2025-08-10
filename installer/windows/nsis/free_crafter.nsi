!include "MUI2.nsh"

!ifndef VERSION
  !define VERSION "0.0.0"
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
