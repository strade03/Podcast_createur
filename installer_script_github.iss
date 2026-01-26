; Script adapté pour GitHub Actions
#define MyAppName "Podcast Createur"
#define MyAppVersion "1.0"
#define MyAppPublisher "Stéphane Verdier"
#define MySite "https://estrade03.forge.apps.education.fr/"
#define MyAppExeName "PodcastCreateur.exe"
; Le dossier généré par le script YAML
#define SourceFolder "deploy" 

[Setup]
; ID Unique (Gardé le tien pour la compatibilité des mises à jour)
AppId={{780FF633-FFD6-4F3C-A28C-889CB9F836E2}
       
; Informations de base
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MySite}
AppSupportURL={#MySite}
AppUpdatesURL={#MySite}

; Installation
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
; Pas besoin de droits admin pour installer (installé dans AppData utilisateur)
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; Sortie (Le fichier .exe sera créé à la racine du runner)
OutputDir=.
OutputBaseFilename=PodcastCreateur_Setup_Windows

; Icône (Doit être présente dans ton dépôt git sous icones/icon.ico)
SetupIconFile=icones/icon.ico

; Compression (LZMA2 est bien meilleur que ZIP pour un installeur)
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

; Métadonnées du fichier exe
VersionInfoVersion={#MyAppVersion}
VersionInfoDescription=Application de création de podcast
VersionInfoCopyright=© 2026 {#MyAppPublisher}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoOriginalFileName={#MyAppExeName}

; Pages de l'assistant
DisableWelcomePage=no
DisableDirPage=no
DisableProgramGroupPage=yes
AllowNoIcons=yes

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; C'est ici que la magie opère : on prend tout ce que le YAML a mis dans 'deploy'
Source: "{#SourceFolder}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Comment: "Podcast Créateur"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; Comment: "Podcast Créateur"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\*.*"
Type: dirifempty; Name: "{app}"