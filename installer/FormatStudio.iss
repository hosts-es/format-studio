; Format Studio 安装包（Inno Setup 7）
;
; 生成步骤：
;   1. 生成 dist\FormatStudio-<版本>-win64 目录（Release exe + Qt 运行库 + ffmpeg）
;   2. 编译："D:\Inno Setup 7\ISCC.exe" installer\FormatStudio.iss
;   3. 产物：dist\FormatStudio-Setup-<版本>.exe
;
; 版本号可在命令行覆盖：ISCC /DMyAppVersion=0.2.2 installer\FormatStudio.iss

#ifndef MyAppVersion
  #define MyAppVersion "0.2.1"
#endif

#define MyAppName "Format Studio"
#define MyAppExeName "formatstudio.exe"
#define MyAppPublisher "hosts-es"
#define MyAppURL "https://github.com/hosts-es/format-studio"

[Setup]
AppId={{8F3C2B41-6D2E-4A7B-9C55-1E0D7A4B9F21}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
; 安装到当前用户目录，安装 / 自动更新都不需要管理员权限
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=..\dist
OutputBaseFilename=FormatStudio-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\icons\app.ico
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; 自动更新时旧版本可能还在运行，让安装器负责关闭它
CloseApplications=yes
RestartApplications=no
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Setup
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
MinVersion=10.0

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\FormatStudio-{#MyAppVersion}-win64\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; 卸载时清理应用设置（QSettings 存在这里）
Root: HKCU; Subkey: "Software\FormatStudio"; Flags: uninsdeletekey

[UninstallDelete]
; 缩略图缓存
Type: filesandordirs; Name: "{localappdata}\FormatStudio"
; 开始菜单分组（避免卸载后残留空目录）
Type: filesandordirs; Name: "{group}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
