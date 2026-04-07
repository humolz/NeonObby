; ----------------------------------------------------------------------------
;  NeonObby — Inno Setup installer script
;
;  Build the installer:
;     1. cmake -B build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
;     2. cmake --build build
;     3. cmake --install build --prefix install/
;     4. "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\NeonObby.iss
;
;  Output:  dist\NeonObbySetup-1.0.4.exe
; ----------------------------------------------------------------------------

#define MyAppName        "NeonObby"
#define MyAppVersion     "1.0.4"
#define MyAppPublisher   "Vance Andrei Maglinte"
#define MyAppExeName     "NeonObby.exe"

[Setup]
; AppId uniquely identifies the app to Windows. Keep this GUID the same
; across versions so 1.1 upgrades 1.0 instead of installing side-by-side.
AppId={{8B7E2F1A-3C4D-4E5F-9A0B-1C2D3E4F5A6B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/
AppSupportURL=https://github.com/
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=NeonObbySetup-{#MyAppVersion}
SetupIconFile=..\resources\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Auto-updater self-replacement: when the in-game updater launches this
; installer, the game .exe is still in use. CloseApplications=force tells
; Inno to detect that, gracefully ask the running game to close, then resume
; the file copy. RestartApplications=yes is left off so the silent update
; doesn't relaunch — the user can reopen the new version on their own.
CloseApplications=force
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
Source: "..\install\NeonObby.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\install\assets\*";     DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}";           Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}";     Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
