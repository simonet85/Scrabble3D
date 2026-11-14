; Inno Setup 6 script, called by package-windows.ps1 with /DAppVersion, /DSourceDir and /DOutputDir

#ifndef AppVersion
  #define AppVersion "dev"
#endif
#ifndef SourceDir
  #define SourceDir "..\dist\Scrabble3D"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

[Setup]
AppId={{6F1C2E4A-3B7D-4C59-9E21-5A8D0B3C7F14}
AppName=Scrabble3D
AppVersion={#AppVersion}
DefaultDirName={autopf}\Scrabble3D
DefaultGroupName=Scrabble3D
UninstallDisplayIcon={app}\Scrabble3D.exe
OutputDir={#OutputDir}
OutputBaseFilename=Scrabble3D-{#AppVersion}-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Scrabble3D"; Filename: "{app}\Scrabble3D.exe"
Name: "{group}\{cm:UninstallProgram,Scrabble3D}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Scrabble3D"; Filename: "{app}\Scrabble3D.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Scrabble3D.exe"; Description: "{cm:LaunchProgram,Scrabble3D}"; Flags: nowait postinstall skipifsilent
