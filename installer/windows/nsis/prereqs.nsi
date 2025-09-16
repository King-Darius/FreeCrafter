# Runs ONLY on Windows builds (CPack NSIS).
# 1) Detect VC++ 2015–2022 x64 runtime (common for MSVC builds).
# 2) If missing, run our bundled vcredist silently.

!macro InstallPrereqs
  Var /GLOBAL VCInstalled
  StrCpy $VCInstalled "0"

  # VC++ 2015–2022 x64 runtime presence:
  # HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64  Installed=1
  ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
  StrCmp $0 1 0 +2
    StrCpy $VCInstalled "1"

  ${If} $VCInstalled == "0"
    # Prefer a bundled redistributable if present:
    IfFileExists "$INSTDIR\vcredist_x64.exe" 0 +3
      DetailPrint "Installing Microsoft Visual C++ Redistributable (x64)..."
      ExecWait '"$INSTDIR\vcredist_x64.exe" /quiet /norestart'
      Goto +3

    # (Optional fallback) Download at install time if not bundled (requires NSIS inetc plugin):
    ; inetc::get /popup "" /caption "Downloading VC++ Redistributable" \
    ;   /url "https://aka.ms/vs/17/release/vc_redist.x64.exe" /end \
    ;   /file "$INSTDIR\vcredist_x64.exe"
    ; ExecWait '"$INSTDIR\vcredist_x64.exe" /quiet /norestart'
  ${EndIf}
!macroend
