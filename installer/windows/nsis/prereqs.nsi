# Runs ONLY on Windows builds (CPack NSIS).
# 1) Detect VC++ 2015-2022 x64 runtime.
# 2) If missing, download via PowerShell and install silently.

!macro InstallPrereqs
  Var /GLOBAL VCInstalled
  StrCpy $VCInstalled "0"

  # VC++ 2015–2022 x64 runtime presence:
  # HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64  Installed=1
  ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
  StrCmp $0 1 0 +2
    StrCpy $VCInstalled "1"

  ${If} $VCInstalled == "0"
    DetailPrint "Downloading Microsoft Visual C++ Redistributable (x64)..."
    # Use PowerShell to download to %TEMP%\vcredist_x64.exe
    nsExec::ExecToStack 'powershell.exe -ExecutionPolicy Bypass -NoProfile -Command "[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri \"https://aka.ms/vs/17/release/vc_redist.x64.exe\" -OutFile \"$env:TEMP\\vcredist_x64.exe\""'

    DetailPrint "Installing Microsoft Visual C++ Redistributable (x64)..."
    ExecWait '"$TEMP\\vcredist_x64.exe" /install /passive /norestart'
  ${EndIf}
!macroend
