//---------------------------------------------------------------------------
// ----- ���� � ��� ����� Indy10 � ������, ------
// ------- �� ��������������� ���� define -------
#define INDY10
// ----------------------------------------------

#define min(a, b)  (((a) < (b)) ? (a) : (b))

#include <vcl.h>
#include <process.h>
#include <Objbase.h>
#include <stdio.h>

#pragma hdrstop

#include "UpdateModule.h"
#include <IniFiles.hpp>
#include <HTTPApp.hpp>

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma warn -8057

TUpdateModuleForm *UpdateModuleForm;

// �����
TThread *hUpdateThread;
bool g_bThreadIsRunning = false;

// ����������, ������������ ��� ��������� ��� ������ ������� �������
String g_sCurrentVersion = "";
String g_sURLinf = "";
bool g_bShowError;
bool g_bAutoRenameToExe;
bool g_bBlockForms;

int VersionCompare(String sVersion1, String sVersion2);
int ProcessHTTPError(int ErrorCode);
int ProcessHTTPError2(int ErrorCode);

//---------------------------------------------------------------------------
__fastcall TUpdateModuleForm::TUpdateModuleForm(TComponent* Owner)
		: TForm(Owner)
{
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
__fastcall TUpdateThread::TUpdateThread(bool suspended)
	: TThread(suspended)
{
}
//---------------------------------------------------------------------------


void TUpdateModuleForm::CheckUpdate(String sCurrentVersion, String sURLinf, bool bShowError, bool bAutoRenameToExe, bool bBlockForms)
{
	g_sCurrentVersion = sCurrentVersion;
	g_sURLinf = sURLinf;
	g_bShowError = bShowError;
	g_bAutoRenameToExe = bAutoRenameToExe;
	g_bBlockForms = bBlockForms;

	if ( g_bThreadIsRunning == false)
	{
		g_bThreadIsRunning = true;
		hUpdateThread = new TUpdateThread(true);
		hUpdateThread->OnTerminate = FuncOnThreadTerminate;
		hUpdateThread->Resume();
	}
}

void __fastcall TUpdateThread::Execute()
{
	int iMsgRet;
	TCHAR cTempDir[MAX_PATH];
	GetTempPath(MAX_PATH, cTempDir);

	// --------------------------------------- ���������� update.inf � ����� ------------------------------------
	TCHAR cTempInfFile[MAX_PATH];
	GetTempFileName(cTempDir, L"Update", NULL, cTempInfFile);

	TIdHTTP *IdHTTPUpdate = new TIdHTTP(UpdateModuleForm);
	TFileStream *fs;

	try { fs = new TFileStream(cTempInfFile, fmCreate|fmOpenWrite); } catch (...)
	{
		if (g_bShowError) MessageBox(Application->Handle, (L"��������� ������ �� ����� ������� � �����:\r\n" + (AnsiString)cTempInfFile).c_str(), L"�������� ����������", MB_OK|MB_ICONWARNING);
		return;
	}
	try { IdHTTPUpdate->Get(g_sURLinf, fs); } catch (...) {}
	delete fs;
	if (g_bShowError) if ( ProcessHTTPError(IdHTTPUpdate->ResponseCode) ) { delete IdHTTPUpdate; return; }
	delete IdHTTPUpdate;
	// -----------------------------------------------------------------------------------------------------------

	// ---------------------------------- �������� update.inf � ������� TMemIniFile -------------------------
	TMemIniFile *mif = new TMemIniFile(cTempInfFile);
	String sLastVersion = mif->ReadString("Info", "Update Version", ""), sMsgText;
	String sURLexe = mif->ReadString("Info", "Update File", "");
	String sChanges = mif->ReadString("Info", "Update Changes", "");
	DeleteFile(cTempInfFile);
	if (sLastVersion == "" || sURLexe == "")
	{
		if (g_bShowError) MessageBox(Application->Handle, L"��������� ������ �� ����� �������� ����������.\r\n\r\n���������� �� ���������� �� ������� - �����������.", L"�������� ����������", MB_OK|MB_ICONWARNING);
		return;
	}
	// -------------------------------------------------------------------------------------------------------

	// ��������� ������� � ��������� ������
	if ( VersionCompare(sLastVersion, g_sCurrentVersion) <= 0 )
	{
		if (g_bShowError) MessageBox(Application->Handle, L"� ��� �� ������ ������ ����� ��������� ������!", L"�������� ����������", MB_OK|MB_ICONINFORMATION);
		return;
	}

	// ------------------------- ����� ������� "�������� ���������?" ----------------------------
	sMsgText = L"���������� ����� ������ ���������: " + sLastVersion;

	if (sChanges != "") sMsgText += L"\r\n��������� � ������: " + sChanges;

	sMsgText += L"\r\n������ �������� ���������?";

	iMsgRet = MessageBox(Application->Handle, sMsgText.c_str(),L"����������", MB_YESNO | MB_ICONQUESTION);
	if (iMsgRet != IDYES) return;
	// -----------------------------------------------------------------------------------------------------

	// ----------------------------- ������������ ��������� ���� �� ����� ���������� ---------------------------
	Synchronize(ShowUpdateFormAndBlockOther);
	// -------------------------------------------------------------------------------------------------------

	// --------------------------------------- ���������� ��������� ������ ---------------------------
	TCHAR cTempExeFile[MAX_PATH];
	GetTempFileName(cTempDir, L"Update", NULL, cTempExeFile);

	IdHTTPUpdate = new TIdHTTP(UpdateModuleForm);
	IdHTTPUpdate->OnWorkBegin = UpdateModuleForm->IdHTTPUpdateWorkBegin;
	IdHTTPUpdate->OnWork = UpdateModuleForm->IdHTTPUpdateWork;
	try { fs = new TFileStream(cTempExeFile, fmCreate|fmOpenWrite); } catch (...)
	{
		if (g_bShowError) MessageBox(Application->Handle, (L"��������� ������ �� ����� ������� � �����:\r\n" + (AnsiString)cTempInfFile).c_str(), L"�������� ����������", MB_OK|MB_ICONWARNING);
		return;
	}
	try { IdHTTPUpdate->Get(sURLexe, fs); } catch (...) {}
	delete fs;
	if (g_bShowError) if ( ProcessHTTPError2(IdHTTPUpdate->ResponseCode) ) { delete IdHTTPUpdate; return; }
	delete IdHTTPUpdate;

	String sDestFilePath = ExtractFileDir(Application->ExeName) + "\\" + ExtractFileName(StringReplace(sURLexe, "/", "\\", TReplaceFlags() << rfReplaceAll));
	if (g_bAutoRenameToExe) sDestFilePath = ChangeFileExt(sDestFilePath, ".exe");
	// ----------------------------------------------------------------------------------------------------

	WinExec( ((AnsiString)getenv("COMSPEC") + " /c ping -n 2 localhost > nul & Move /y \"" + cTempExeFile + "\" \"" + sDestFilePath + "\" & Start \"\" \"" + sDestFilePath + "\"").c_str() , SW_HIDE);

	// ����������� � ���, ��� ����� ��������� ���������
	SendMessage(Application->Handle, WM_CLOSE, 0, 0);

	return;
}

void __fastcall TUpdateModuleForm::FuncOnThreadTerminate(TObject *ASender)
{
	g_bThreadIsRunning = false;
}

void __fastcall TUpdateThread::ShowUpdateFormAndBlockOther()
{
	int FormCount = Screen->FormCount;
	if (g_bBlockForms)
	{
		for (int i=0; i<FormCount; i++)
		{
			Screen->Forms[i]->Enabled = false;
		}
	}
	UpdateModuleForm->Visible = true;
	UpdateModuleForm->Enabled = true;
}

int ProcessHTTPError(int ErrorCode)
{
	if (ErrorCode == 200) return 0;
	else if (ErrorCode == -1)
	{
		MessageBox(Application->Handle, L"��������� ������ �� ����� �������� ����������.\r\n\r\n��������� ��������-����������.", L"�������� ����������", MB_OK|MB_ICONWARNING);
	}
	else if (ErrorCode == 404)
	{
		MessageBox(Application->Handle, L"��������� ������ �� ����� �������� ����������.\r\n\r\n���������� �� ���������� ����������� �� �������.", L"�������� ����������", MB_OK|MB_ICONWARNING);
	}


	return 1;
}

int ProcessHTTPError2(int ErrorCode)
{
	if (ErrorCode == 200) return 0;
	else if (ErrorCode == -1)
	{
		MessageBox(Application->Handle, L"��������� ������ �� ����� �������� ����������.\r\n\r\n��������� ��������-����������.", L"������", MB_OK|MB_ICONWARNING);
	}
	else if (ErrorCode == 404)
	{
		MessageBox(Application->Handle, L"��������� ������ �� ����� �������� ����������.\r\n\r\���������� ����������� �� �������.", L"������", MB_OK|MB_ICONWARNING);
	}

	return 1;
}

int VersionCompare(String sVersion1, String sVersion2)
{
	if (sVersion1 == sVersion2) return 0;

	String sep = '.';
	if (sVersion1.Pos(sep) == 0) sep = ",";

	TSysCharSet Separators, WhiteSpace;
	Separators << '.';

	TStrings *aVersion1 = new TStringList;
	TStrings *aVersion2 = new TStringList;
	ExtractStrings( Separators, WhiteSpace, sVersion1.c_str(), aVersion1);
	ExtractStrings( Separators, WhiteSpace, sVersion2.c_str(), aVersion2);

	int count1 = aVersion1->Count, num1;
	int count2 = aVersion2->Count, num2;
	int cmp = 0, retValue = 0, min_count = min(count1, count2);
	char str1[5], str2[5];

	for (int i=0; i<min_count; i++)
	{
		num1 = -1; num2 = -1; str1[0] = 0; str2[0] = 0;

		sscanf( AnsiString(aVersion1->Strings[i]).c_str(), "%i%s", &num1, &str1);
		if (num1 == -1) sscanf( AnsiString(aVersion1->Strings[i]).c_str(), "%s", &str1);
		sscanf( AnsiString(aVersion2->Strings[i]).c_str(), "%i%s", &num2, &str2);
		if (num2 == -1) sscanf( AnsiString(aVersion2->Strings[i]).c_str(), "%s", &str2);

		if (num1 != -1 && num2 == -1) retValue = 1;
		else if (num1 == -1 && num2 != -1) retValue = -1;
		else if (num1 != -1 && num2 != -1 && num1 != num2)
		{
			if (num1 > num2) retValue = 1;
			else if (num1 < num2) retValue = -1;
		}
		else if ( (num1 == -1 && num2 == -1) || num1 == num2 )
		{
			if (str1[0] == 0 && str2[0] != 0) retValue = 1;
			else if (str1[0] != 0 && str2[0] == 0) retValue = -1;
			else
			{
				cmp = strcmp(str1, str2);
				if (cmp > 0) retValue = 1;
				else if (cmp < 0) retValue = -1;
			}
		}

		if (retValue != 0) break;
	}

	if ( retValue == 0 )
	{
		if (count1 > count2) retValue = 1;
		else retValue = -1;
	}

	delete aVersion1; delete aVersion2;

	return retValue;
}

void __fastcall TUpdateModuleForm::IdHTTPUpdateWorkBegin(TObject *ASender, TWorkMode AWorkMode, IDHTTPINT AWorkCountMax)
{
	ProgressBarUpdate->Position = 0;
	ProgressBarUpdate->Max = AWorkCountMax;
}

void __fastcall TUpdateModuleForm::IdHTTPUpdateWork(TObject *ASender, TWorkMode AWorkMode, IDHTTPINT AWorkCount)
{
	ProgressBarUpdate->Position = AWorkCount;
}


