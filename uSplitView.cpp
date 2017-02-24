#include <vcl.h>
#include "uSplitView.h"
#include "uCmdForm.h"
#include "uQuickMenu.h"
#include "UpdateModule.h"
#include <SysUtils.hpp>

#pragma hdrstop
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
# include <locale>
# include <Wininet.h>
# include <wstring.h>
#include  <comdef.h>
#include  <wbemidl.h>
#include <stdlib.h>
#include "Registry.hpp"
#include <winsock2.h>
#include <Shellapi.hpp>
#include <memory>  //dos
#include <string>
#include <iostream>
#include <istream>
#include <fstream>
#include <IOUtils.hpp>
#include <StrUtils.hpp> //dos



//---------------------------------------------------------------------------
#pragma package(smart_init)

#define SystemProcessorTimes 8
#define MAX_PROCESSORS 32
#define _WIN32_DCOM


#pragma link "Vcl.WinXCtrls"
#pragma link "perfgrap"
# pragma link "CGAUGES"

#pragma link "cgauges"
#pragma resource "*.dfm"



String gCurrentVersion = "2";
TInnerWorld *InnerWorld;

TListItem * Item;
bool Shutdown=0;

typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Unknown,
    MaximumIncrement,
    PhysicalPageSize,
    NumberOfPhysicalPages,
    LowestPhysicalPage,
    HighestPhysicalPage,
    AllocationGranularity,
    LowestUserAddress,
    HighestUserAddress,
    ActiveProcessors;
    char NumberProcessors;
}SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_TIMES
{
    __int64 IdleTime,
    KernelTime,
    UserTime,
    DpcTime,
    InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSORS_TIMES[MAX_PROCESSORS], SYSTEM_PROCESSOR_TIMES, *PSYSTEM_PROCESSOR_TIMES;

typedef UINT __stdcall (*ZwQuerySystemInformation)(DWORD,void*,DWORD,DWORD*);

/*function*/

SYSTEM_INFO SYSINFO;



int GetProcessorsCount(void)
{
  SYSTEM_INFO SYSINFO;
  GetSystemInfo(&SYSINFO);
  return SYSINFO.dwNumberOfProcessors;
}

int GetProcessorType(SYSTEM_INFO& SYSINFO)
{
GetSystemInfo(&SYSINFO);
return SYSINFO.dwProcessorType;
}

int GetProcessorOemId(SYSTEM_INFO& SYSINFO)
{
GetSystemInfo(&SYSINFO);
return SYSINFO.dwOemId;
}





int * cpu_usage;
__int64 temp = 0;
unsigned int cycle_count = 0,
			 count_mas = 0,
			 interval = 1000; //������ ������ (��)

HMODULE lib = NULL;
DWORD nowtime, oldtime, pertime, rez, curtime = 0;
SYSTEM_PROCESSORS_TIMES CurrentSysProcTimes, PreviousSysProcTimes;

ZwQuerySystemInformation func;

String msg = "";
char * windowname = new char[20];

bool init = false;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
 __fastcall TInnerWorld::TInnerWorld(TComponent* Owner)
	: TForm(Owner)
{
   InnerWorld->Hide();
}
//---------------------------------------------------------------------------
void __fastcall TInnerWorld::FormCreate(TObject *Sender)
{

	for (int i = 0; i < TStyleManager::StyleNames.Length; i++)
	cbxVclStyles->Items->Add(TStyleManager::StyleNames[i]);
	cbxVclStyles->ItemIndex = cbxVclStyles->Items->IndexOf(TStyleManager::ActiveStyle->Name);

	BOOL bEnablePrivilege=TRUE;
	ChekPrivilege(bEnablePrivilege);
    InfoPC();
	InfoOfDisk();
	ProcessList();
	GetAutoRun();

}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::cbxVclStylesChange(TObject *Sender)
{
	TStyleManager::SetStyle(cbxVclStyles->Text);
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::grpDisplayModeClick(TObject *Sender)
{
  SV->DisplayMode = (TSplitViewDisplayMode)grpDisplayMode->ItemIndex;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::grpCloseStyleClick(TObject *Sender)
{
  SV->CloseStyle = (TSplitViewCloseStyle)grpCloseStyle->ItemIndex;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::grpPlacementClick(TObject *Sender)
{
  SV->Placement = (TSplitViewPlacement)grpPlacement->ItemIndex;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::SVClosed(TObject *Sender)
{
  // When TSplitView is closed, adjust ButtonOptions and Width
  catMenuItems->ButtonOptions = catMenuItems->ButtonOptions >> boShowCaptions;
  if (SV->CloseStyle == svcCompact)
    catMenuItems->Width = SV->CompactWidth;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::SVOpened(TObject *Sender)
{
  // When not animating, change size of catMenuItems when TSplitView is opened
  catMenuItems->ButtonOptions = catMenuItems->ButtonOptions << boShowCaptions;
  catMenuItems->Width = SV->OpenedWidth;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::SVOpening(TObject *Sender)
{
  // When animating, change size of catMenuItems at the beginning of open
  catMenuItems->ButtonOptions = catMenuItems->ButtonOptions << boShowCaptions;
  catMenuItems->Width = SV->OpenedWidth;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::chkUseAnimationClick(TObject *Sender)
{
  SV->UseAnimation = chkUseAnimation->Checked;
  lblAnimationDelay->Enabled = SV->UseAnimation;
  trkAnimationDelay->Enabled = SV->UseAnimation;
  lblAnimationStep->Enabled = SV->UseAnimation;
  trkAnimationStep->Enabled = SV->UseAnimation;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::trkAnimationDelayChange(TObject *Sender)
{
  SV->AnimationDelay = trkAnimationDelay->Position * 5;
  lblAnimationDelay->Caption = "Animation Delay (" + IntToStr(SV->AnimationDelay) + ")";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::trkAnimationStepChange(TObject *Sender)
{
  SV->AnimationStep = trkAnimationStep->Position * 5;
  lblAnimationStep->Caption = "Animation Step (" + IntToStr(SV->AnimationStep) + ")";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::actHomeExecute(TObject *Sender)
{
  Log(actHome->Caption + " Clicked");
  if (SV->Opened && chkCloseOnMenuClick->Checked)
	SV->Close();
  PageControl1->ActivePageIndex=0;
  InfoPC();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::actProcessExecute(TObject *Sender)
{
  Log(actProcess->Caption + " Clicked");
  if (SV->Opened && chkCloseOnMenuClick->Checked)
	SV->Close();
	PageControl1->ActivePageIndex=1;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::actAutoExecute(TObject *Sender)
{
  Log(actAuto->Caption + " Clicked");
  if (SV->Opened && chkCloseOnMenuClick->Checked)
	SV->Close();
  PageControl1->ActivePageIndex=2;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::actRegExecute(TObject *Sender)
{
	Log(actAuto->Caption + " Clicked");
  if (SV->Opened && chkCloseOnMenuClick->Checked)
	SV->Close();
  PageControl1->ActivePageIndex=3;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::actSettingsExecute(TObject *Sender)
{
  Log(actAuto->Caption + " Clicked");
  if (SV->Opened && chkCloseOnMenuClick->Checked)
	SV->Close();
  PageControl1->ActivePageIndex=3;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::catMenuItemsCategoryCollapase(TObject *Sender, TButtonCategory * const Category)
{
  // Prevent the catMenuItems Category group from being collapsed
  catMenuItems->Categories->Items[0]->Collapsed = false;
}
//---------------------------------------------------------------------------

void TInnerWorld::Log(UnicodeString Msg)
{
  //int Idx = lstLog->Items->Add(Msg);
  //lstLog->TopIndex = Idx;
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::ProcessList()
{
   	ListView1->Clear();
    HANDLE InfoProcess=0;
    HANDLE hToken=0;
    DWORD PID;
    bool EnumProcess;
    bool EnumThread;
	bool EnumModule;
    wchar_t pathP[500];
    TOKEN_USER* pUserInfo;
    char name[256];
    char domain[256];
    char *result;
    DWORD dwSize = 256;
    int iUse;
    int ColP=0;

//������ ���������
    HANDLE ProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//������ �������
	HANDLE ModuleSnap  = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,  0);
//������ �������
//��������� ��� ������
    PROCESSENTRY32 Process;
    MODULEENTRY32  Module;
//��������� �� ������, ��� ����������� ��������
    Process.dwSize = sizeof(PROCESSENTRY32);
    Module.dwSize  = sizeof(MODULEENTRY32);
//����������� ������� ��������
    EnumProcess = Process32First (ProcessSnap, &Process);
    EnumModule  = Module32First  (ModuleSnap,  &Module);

    while (EnumProcess)
    {

		PID=Process.th32ProcessID;
		AnsiString Fontdr=Process.szExeFile;

		if((PID==4)||(PID==0)||(Fontdr=="fontdrvhost.exe"))
        {
         EnumProcess = Process32Next(ProcessSnap, &Process);
        }
        else
        {
        ListView1->Items->Add();
		Item = ListView1->Items->Item[ListView1->Items->Count-1];

//-------------------------------���� ���������� � ���������---------------------------------------

        InfoProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
        Process.th32ProcessID);
    if (!(OpenProcessToken(InfoProcess, TOKEN_QUERY, &hToken)))
    {
		ListBox3->Items->Add("OpenToken " + SysErrorMessage(GetLastError()));
    }

    pUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
    if (!(GetTokenInformation(hToken, TokenUser, pUserInfo, dwSize, &dwSize)))
	{
		ListBox3->Items->Add("GetTokenInfo " + AnsiString(GetLastError()));
	}

    // ������ ��������� ����� ������������
    if (!(LookupAccountSidA(0, pUserInfo->User.Sid, name, &dwSize, domain,
        &dwSize, (PSID_NAME_USE) & iUse)))
	{
		ListBox3->Items->Add("Lookup " + SysErrorMessage(GetLastError()));
    }


		Item->Caption=PID;// �� ��������

		Item->ImageIndex=0;//��������
		Item->SubItems->Add(Process.szExeFile); // ��� ��������

        switch (GetPriorityClass(InfoProcess))
            {
              case 256: Item->SubItems->Add("��������"); break;
              case 128:  Item->SubItems->Add("�������"); break;
			  case  32:  Item->SubItems->Add("����������"); break;
			  case  64:  Item->SubItems->Add("������");  break;
			  case  32768:  Item->SubItems->Add("���� ��������"); break;
			  case  16384:  Item->SubItems->Add("���� ��������");  break;
            }

        Item->SubItems->Add(name);//��������� ������ ��������
        Item->SubItems->Add(Module.szModule); //������ ��������
        Item->SubItems->Add(Process.cntThreads); //���-�� �������

        if(GetModuleFileNameEx(InfoProcess, NULL, pathP, MAX_PATH))  //���� �������� ������������ � pBuf
             Item->SubItems->Add(pathP);
        else
            Item->SubItems->Add("����������");

        EnumProcess = Process32Next(ProcessSnap, &Process);
		EnumModule  = Module32Next (ModuleSnap,  &Module);

		Label2->Caption=IntToStr(ColP++);

      }
    }
    CloseHandle(ProcessSnap);
	CloseHandle(ModuleSnap);

}
//---------------------------------------------------------------------------


void  __fastcall TInnerWorld::SetPriority(DWORD PRIORITY_CLASS)
{
  bool rezult;
	int NumP;
	HANDLE PriorityProcess;
	DWORD ExitCode;
	AnsiString NameProc;


	if (ListView1->Selected)
	{
	   NumP=StrToInt(ListView1->ItemFocused->Caption);

	   NameProc= ListView1->Items->Item[ListView1->ItemIndex]->SubItems->Strings[0];
	   StatusBar1->Panels->Items[0]->Text =NameProc;
	}


	PriorityProcess = OpenProcess(THREAD_ALL_ACCESS, true, NumP);
		if (PriorityProcess)
				{
			SetPriorityClass(PriorityProcess,PRIORITY_CLASS);

				}

//CloseHandle(ProcessSnap);
CloseHandle(PriorityProcess);
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::ListView1SelectItem(TObject *Sender, TListItem *Item,
		  bool Selected)
{
	StatusBar1->Panels->Items[1]->Text =" ";

	if (ListView1->Selected)
		{
	StatusBar1->Panels->Items[0]->Text =ListView1->Items->Item[ListView1->ItemIndex]->SubItems->Strings[0];
	StatusBar1->Panels->Items[2]->Text =ListView1->Items->Item[ListView1->ItemIndex]->SubItems->Strings[5];
		}
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::CloseProcessByPID()
{
  	bool rez;
	int NumProc;
	HANDLE closeProcess;
	DWORD ExitCode;
	AnsiString NameProc;

	 if (ListView1->Selected)
	  {
	NumProc=StrToInt(ListView1->ItemFocused->Caption); //���������� ������� � ������
	NameProc= ListView1->Items->Item[ListView1->ItemIndex]->SubItems->Strings[0];
	  }

	closeProcess = OpenProcess(PROCESS_ALL_ACCESS, true, NumProc); //�������� ������� ������� � ��������
		if (closeProcess)
				{
				GetExitCodeProcess(closeProcess, &ExitCode);
				rez=TerminateProcess(closeProcess, ExitCode);//���������� �������� � ��� ��������
				}
		if(rez==true)
		{

			StatusBar1->Panels->Items[1]->Text ="��������";
		  /*
			TNotification *MyNotification;  //����� �����������, ������ Windows 10
			MyNotification = new TNotification;
				try
				{
					MyNotification->Name = "Windows10Notification";
					MyNotification->Title = NameProc;
					MyNotification->AlertBody ="������� �������� ";

					NotificationCenter1->PresentNotification(MyNotification);
				}
				__finally
				{
					delete MyNotification;
				} */
		}
		else
		{
			StatusBar1->Panels->Items[1]->Text ="������ �������� ��������";
		}
CloseHandle(closeProcess);
  ProcessList();
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::N1Click(TObject *Sender)
{
  CloseProcessByPID();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N2Click(TObject *Sender)
{
  ProcessList();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N4Click(TObject *Sender)
{
   	SetPriority(REALTIME_PRIORITY_CLASS);
	StatusBar1->Panels->Items[1]->Text ="�������� ���������";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N5Click(TObject *Sender)
{
	SetPriority(HIGH_PRIORITY_CLASS);
	StatusBar1->Panels->Items[1]->Text ="������� ���������";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N6Click(TObject *Sender)
{
	SetPriority(ABOVE_NORMAL_PRIORITY_CLASS);
	StatusBar1->Panels->Items[1]->Text ="���� ��������";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N7Click(TObject *Sender)
{
	 SetPriority(NORMAL_PRIORITY_CLASS);
	StatusBar1->Panels->Items[1]->Text ="������� ���������";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N8Click(TObject *Sender)
{
	  SetPriority(BELOW_NORMAL_PRIORITY_CLASS);
	StatusBar1->Panels->Items[1]->Text ="���� ��������";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N9Click(TObject *Sender)
{
     SetPriority(IDLE_PRIORITY_CLASS);
	StatusBar1->Panels->Items[1]->Text ="������ ���������";
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N10Click(TObject *Sender)
{
	if (ListView1->Selected)
		{
ShellExecute( Handle, L"explore",
	(ExtractFileDir(ListView1->Items->Item[ListView1->ItemIndex]->SubItems->Strings[5]) ).w_str(),
		NULL, NULL, SW_RESTORE );
		}
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::N11Click(TObject *Sender)
{
   UnicodeString asd=ListView1->Items->Item[ListView1->ItemIndex]->SubItems->Strings[0];
   UnicodeString google="https://www.google.by/#q=";
   UnicodeString c =google+asd;
   ShellExecute(Handle, L"open",c.w_str(), 0, 0, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------









void __fastcall TInnerWorld::Button2Click(TObject *Sender)
{
if (Timer1->Enabled)
{
Timer1->Enabled = false;

//cycle_count = 0;
temp = 0;
curtime = 0;
oldtime = GetTickCount();

Button2->Enabled = false;
Button3->Enabled = true;
}
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Timer1Timer(TObject *Sender)
{
        nowtime = GetTickCount();
        pertime = nowtime - oldtime;
        curtime += pertime;
        oldtime = nowtime;

        /*read the usage*/
        func(SystemProcessorTimes,
              &CurrentSysProcTimes[0],
              sizeof(CurrentSysProcTimes),
              &rez);

        temp = 0;
        for(int j=0;j<MAX_PROCESSORS;j++)
        {
            temp += CurrentSysProcTimes[j].IdleTime - PreviousSysProcTimes[j].IdleTime;
		}
        temp /= 10000;  //��������� �� ���������� � ������������
        temp /= (int) GetProcessorsCount(); //����� �� ���������� ����

        temp = (int) pertime - temp;


		if (cycle_count >= 1)
		{
		msg = "";
		msg += "Current time: " + String(curtime) + " milisecond(s). Processor usage on " + String(temp) + " milisecond.";
  //		Memo1->Lines->Add(msg);


		cpu_usage[count_mas] = temp / (float) pertime*100;
//		Edit4->Text = IntToStr(count_mas + 1);

		msg = "";
		msg += "�������� ���������� " + IntToStr(cpu_usage[count_mas]) + "%";
//		Memo1->Lines->Add(msg);
//		Memo1->Lines->Add("");
		Label3->Caption = msg; //����� ��������
		PerformanceGraph1->Scale += 1;
//      PerformanceGraph1->StepSize = 1;

		count_mas++;
		}

		cycle_count++;
//        Edit2->Text = IntToStr(cycle_count);

		memcpy(&PreviousSysProcTimes[0],
				&CurrentSysProcTimes[0],
				sizeof(PreviousSysProcTimes));
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Button3Click(TObject *Sender)
{
if (init)
{
    Button3->Enabled = false;

    /*init prev processors data*/

    //�������� ������� �� ������� � ���� ��� �� ������� �������.
    oldtime =  GetTickCount();
    if (func(SystemProcessorTimes,
			  &PreviousSysProcTimes[0],
			  sizeof(PreviousSysProcTimes),0) != 0)
	{
		msg = "Error #3: ������ �������!";
		Application->MessageBox(msg.c_str(), L"Warning", MB_OK | MB_ICONERROR);
		FreeLibrary(lib);
		return;
	}

Sleep(100);
cycle_count = 0;

Button2->Enabled = true;

Timer1->Interval = interval;
Timer1->Enabled = true;
}
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::PerformanceGraph1ScaleChange(TObject *Sender)
{
  PerformanceGraph1->DataPoint(clLime, cpu_usage[count_mas]+1);
  PerformanceGraph1->Update();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::FormClose(TObject *Sender, TCloseAction &Action)
{
if (lib) FreeLibrary(lib);
delete[] cpu_usage;
}
//---------------------------------------------------------------------------



void __fastcall TInnerWorld::imgMenuClick(TObject *Sender)
{
  if (SV->Opened)
    SV->Close();
  else
	SV->Open();
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::GetAutoRun()
{
  const String autoRunRoot = "Software\\Microsoft\\Windows\\CurrentVersion\\";
  TRegistry *reg = new TRegistry();
  reg->RootKey = HKEY_LOCAL_MACHINE;
  //�����������, ��� ����������� ��������� (�� ������� �������� �
  //��������������� ������)
  if ( reg->OpenKey(autoRunRoot + "Run", false) )
  {
    if ( reg->ValueExists(Application->Title) )
    {
      //��������� ���� � ������ Run - ����������� ��� ������ ��������
	  //Windows
	  ToggleSwitch1->State=true;
	  //optRunAuto->Checked = true;
	  reg->CloseKey();
      delete reg;
      return;
	}
	else
       ToggleSwitch1->State=false;
    reg->CloseKey();
  }
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::ToggleSwitch1Click(TObject *Sender)
{
	  const String autoRunRoot = "Software\\Microsoft\\Windows\\CurrentVersion\\";
  TRegistry *reg = new TRegistry();
  reg->RootKey = HKEY_LOCAL_MACHINE;
  //������ �������� ������
  //..�������� ��������� �� ������ Run
  if ( !ToggleSwitch1->State )
  {
    if ( reg->OpenKey(autoRunRoot + "Run", false) )
    {
      reg->DeleteValue(Application->Title);
      reg->CloseKey();
    }
  }

  //��������� ������ ������ (�������� ��������� � �������������� ������)
  if ( ToggleSwitch1->State )
  {
    //..���������� ��������� � ������ Run
    if ( reg->OpenKey(autoRunRoot + "Run",  true) )
    {
      reg->WriteString(Application->Title, Application->ExeName);
      reg->CloseKey();
    }
  }

  delete reg;
  //��� ��������, ������� ��������� �� ����� �� ������ �� �������
  GetAutoRun();
}
//---------------------------------------------------------------------------

BOOL TInnerWorld::SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)

{
 TOKEN_PRIVILEGES tp;
	LUID luid;
// lookup privilege on local system. privilege to lookup. receives LUID of privilege
	if ( !LookupPrivilegeValue(NULL, lpszPrivilege, &luid ) )
	{
		AnsiString ErrorLookup=GetLastError();
		ListBox1->Items->Add("LookupPrivilegeValue error: "+ErrorLookup);
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	// Enable the privilege or disable all privileges.

	if ( !AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL) )
	{
		  AnsiString Error=GetLastError();
		  ListBox1->Items->Add("!AdjustTokenPrivileges "+Error);
		  return FALSE;
	}
	else
	{
		 AnsiString Error=GetLastError();
		 ListBox1->Items->Add("AdjustTokenPrivileges OK. code("+Error+")");
    }


	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

	{
		  ListBox1->Items->Add("The token does not have the specified privilege.");
		  return FALSE;
	}

	return TRUE;

	//SetPrivilege(hToken, lpszPrivilege, bEnablePrivilege);
}
//---------------------------------------------------------------------------

BOOL TInnerWorld::ChekPrivilege(BOOL bEnablePrivilege)
{
   HANDLE hToken;
		LPCTSTR lpszPrivilege = SE_DEBUG_NAME;


		if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		{
			AnsiString ErrorOpenProcess=GetLastError();
			ListBox1->Items->Add("OpenProcessToken() error"+GetLastError());
			ListBox1->Items->Add("OpenProcessToken() error"+ErrorOpenProcess);

		}
		else
		{
			AnsiString ErrorOpenProcess=GetLastError();
			ListBox1->Items->Add("OpenProcessToken(OK) "+ErrorOpenProcess);
		}

		if(!SetPrivilege(hToken, lpszPrivilege, bEnablePrivilege))
			ListBox1->Items->Add("Privilege Error");
		else
			ListBox1->Items->Add("Privilege +");
		return  GetLastError();
}
//---------------------------------------------------------------------------


void TInnerWorld::GetWmiInfo(TStrings *L, wchar_t* wsQuery)
{
  HRESULT hres;
	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    IWbemLocator *pWbemLocator = NULL; // ���������
    if (CoCreateInstance(CLSID_WbemAdministrativeLocator, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_IUnknown, (void**)&pWbemLocator) == S_OK)
	{
        IWbemServices *pWbemServices = NULL;
        wchar_t* wsNamespace = (L"ROOT\\CIMV2");
        if (pWbemLocator->ConnectServer(wsNamespace, NULL, NULL, NULL, 0, NULL, NULL, &pWbemServices) == S_OK)
        {
			IEnumWbemClassObject *pEnumClassObject = NULL;
            wchar_t* wsWQL = L"WQL";
            if (pWbemServices->ExecQuery(wsWQL, wsQuery, WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumClassObject) == S_OK)
            {
                IWbemClassObject *pClassObject = NULL;
				ULONG uCount = 1, uReturned = 0;
                if (pEnumClassObject->Reset() == S_OK)
                {
                    int iEnumIdx = 0;
                    hres = pEnumClassObject->Next(WBEM_INFINITE, uCount, &pClassObject, &uReturned);
					while (hres == S_OK)
                    {
						//L->Add("[" + IntToStr(iEnumIdx) + "]");
                        SAFEARRAY *pvNames = NULL;
                        if (pClassObject->GetNames(NULL, WBEM_FLAG_ALWAYS | WBEM_MASK_CONDITION_ORIGIN,
							NULL, &pvNames) == S_OK)
                        {
                            long vbl, vbu;
                            SafeArrayGetLBound(pvNames, 1, &vbl);
                            SafeArrayGetUBound(pvNames, 1, &vbu);
							for (long idx = vbl; idx <= vbu; idx++)
                            {
                                long aidx = idx;
                                wchar_t *wsName = 0;
                                VARIANT vValue;
								VariantInit(&vValue);
                                SafeArrayGetElement(pvNames, &aidx, &wsName);
                                BSTR bs = SysAllocString(wsName);
                                HRESULT hRes = pClassObject->Get(bs, 0, &vValue, NULL, 0);
                                SysFreeString(bs);
								if (hRes == S_OK)
                                {
                                    AnsiString s;
                                    Variant v = *(Variant*)&vValue;
                                    if (v.IsArray())
									{
                                        for (int i = v.ArrayLowBound(); i <= v.ArrayHighBound(); i++)
                                        {
                                            Variant a = v.GetElement(i);
                                            if (!s.IsEmpty()) s += ", ";
											s += VarToStr(a);
                                        }
                                    }
                                    else
									{
										s = VarToStr(v);
									}
									L->Add(s);
									//L->Add(AnsiString(wsName) + " = " + s);
									//L->Text=s;
								}
								VariantClear(&vValue);
                                SysFreeString(wsName);
							}
                        }
						if (pvNames)
							SafeArrayDestroy(pvNames);
						iEnumIdx++;
                        hres = pEnumClassObject->Next(WBEM_INFINITE, uCount,
							&pClassObject, &uReturned);
					} // WHILE
					//if (hres != S_OK)
					//	L->Add((hres & 0xFFFF));
				} // pEnumClassObject->Reset()
				if (pClassObject)
					pClassObject->Release();
			}
			if (pEnumClassObject)
				pEnumClassObject->Release();
		}
		if (pWbemServices)
			pWbemServices->Release();
	}
	if (pWbemLocator)
		pWbemLocator->Release();
}
//---------------------------------------------------------------------------





void __fastcall TInnerWorld::Memo2Change(TObject *Sender)
{
   /*			wchar_t* ss = L"select Name, Caption from WIN32_processor";
		//wchar_t* kk = L"select Caption from WIN32_processor";
	std::auto_ptr<TStringList> lst(new TStringList); // #include <memory>
	GetWmiInfo(lst.get(), ss);
	//GetWmiInfo(lst.get(), kk);
	Label1->Caption = lst->Strings[0];
	Label2->Caption = lst->Strings[1];
   */

}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::Button5Click(TObject *Sender)
{
	Memo2->Clear();
}
//---------------------------------------------------------------------------






void TInnerWorld::InfoPC()
{

   wchar_t*  InfoCompSys = L"select UserName,Workgroup from  Win32_ComputerSystem";
   wchar_t*  InfoProcessor = L"select Name, Caption,NumberOfCores,Manufacturer,L2CacheSize,L3CacheSize,PowerManagementSupported,SecondLevelAddressTranslationExtensions from  Win32_Processor";
   wchar_t*  InfoVideoCard = L"select Description,AdapterRAM From Win32_VideoController";
   wchar_t*  InfoBaseBoard = L"select Manufacturer,Product,SerialNumber,Version From Win32_BaseBoard";
   wchar_t*  InfoBios = L"select Manufacturer,SMBIOSBIOSVersion,Version From Win32_BIOS";
   wchar_t*  InfoMonitor = L"select Description,ScreenWidth,ScreenHeight From Win32_DesktopMonitor";
   wchar_t*  InfoBattery = L"select EstimatedChargeRemaining,BatteryStatus,EstimatedRunTime,DesignVoltage,Name From Win32_Battery ";
   wchar_t*  InfoNetworkAdapter = L"SELECT Description,Index,NetConnectionID,NetConnectionStatus,MACAddress From Win32_NetworkAdapter WHERE NetConnectionStatus=2";
	GetWmiInfo(Memo2->Lines, InfoNetworkAdapter);                                                                                            //ORDER BY NetConnectionStatus     WHERE NetConnectionID IS NOT NULL
		std::auto_ptr<TStringList> lst(new TStringList); // #include <memory>
	GetWmiInfo(lst.get(), InfoCompSys);
	GetWmiInfo(lst.get(), InfoProcessor);
	GetWmiInfo(lst.get(), InfoVideoCard);
	GetWmiInfo(lst.get(), InfoBaseBoard);
	GetWmiInfo(lst.get(), InfoBios);
	GetWmiInfo(lst.get(), InfoMonitor);
	GetWmiInfo(lst.get(), InfoBattery);
	GetWmiInfo(lst.get(), InfoNetworkAdapter);


			 //Description,AdapterRAM,VideoArchitecture
//-----------------------------[���������]----------------------------------------
	LUserName->Caption  ="��� ������������: "+lst->Strings[0]; 	//��� ������������
	LWorkGroup->Caption ="������� ������: "  +lst->Strings[1]; 	//workgroup
	LProcessor->Caption ="���������: "       +lst->Strings[6]; 	//������ ����������
	LCores->Caption     ="���-�� ����: "     +lst->Strings[7]; 	//����
	LManufacturer->Caption ="�������������:" +lst->Strings[5]+"\n" + lst->Strings[2];
	L2Cache->Caption    ="L2 ���: " 		 +lst->Strings[3] + " ��";   //(StrToInt(lst->Strings[3])/1024);
	L3Cache->Caption    ="L3 ���: " 		 +lst->Strings[4] + " ��";    //��� L2,L3;
   //	PowerManager->Caption = lst->Strings[8];      		  //���������� ���������������
	if (StrToBool(lst->Strings[8])==true)
	{
	  PowerManager->Caption = "���������� ���������������: ��";
	}
	else
	  PowerManager->Caption = "���������� ���������������: ���";

   //LHyperV->Caption = lst->Strings[9];
	if (StrToBool(lst->Strings[9])==true)
	{
	  LHyperV->Caption = "��������� Hyper-V: ��";
	}
	else
	  LHyperV->Caption = "��������� Hyper-V: ���";

//-----------------------------[����������]----------------------------------------

   LVideoCard->Caption ="����������: "  +lst->Strings[11];
   LVideoMem->Caption  ="�����������: "+IntToStr(StrToInt(lst->Strings[10])/(1024*1024)*(-1))+" ��";  //(StrToInt(lst->Strings[10])/(1024*1024))*(-1);
   LVideoC2->Caption   ="����������: " 	+lst->Strings[13];

//--------------------------[����������� �����]------------------------------------
   LBaseB->Caption = "����������� �����: "+lst->Strings[14];
   LProd->Caption  = "�������: "		  +lst->Strings[15];
   LSerNum->Caption= "�������� �����: "   +lst->Strings[16];
   LBver->Caption  = "������: "           +lst->Strings[17];
//--------------------------------[����]-------------------------------------------
   LBios->Caption = "���� :"	+lst->Strings[18];
   LBiosver->Caption = "������: "  +lst->Strings[19];
   LBiosdate->Caption= lst->Strings[20];
//------------------------------[�������]------------------------------------------
   LMonitor->Caption = "�������: " +lst->Strings[24] +"\n" +
					"����������: " +lst->Strings[26] + "x" + lst->Strings[25];

//--------------------------------[�������]----------------------------------------
   Label7->Caption="���������� � �������";

  CGaugeBattery->Progress=StrToInt(lst->Strings[29]);
  //LBstatus->Caption = "������: " + lst->Strings[27];
  switch (StrToInt(lst->Strings[27]))
  {
  case 1:  LBstatus->Caption = "������: ������� �����������"; break;
  case 2:  LBstatus->Caption = "������: ������� �����������"; break;
  default: LBstatus->Caption = "0";	break;
  }

  switch (StrToInt(lst->Strings[27]))
  {
  case 1:
	if (StrToInt(lst->Strings[30])==71582788)
	{
	   LBRunTime->Caption ="���������� ����� ������ �� ��������..";
	}
	else
	LBRunTime->Caption ="��������: "+TDateTime( (lst->Strings[30].ToDouble())/SecsPerDay ).FormatString("nn:ss")+" ���"; break;
						//TDateTime( ((double)lst->Strings[30])/SecsPerDay ).FormatString("hh:nn:ss");
  case 2: LBRunTime->Caption = "����������� �������� ���������� ";   break;
  //default: LBRunTime->Caption ="���������� ����� ������ �� ��������..";
 //	;
  }

  LBvolt->Caption =(lst->Strings[28].ToDouble()/1000);

//---------------------------[������� �������]-------------------------------------
   LNetDesc->Caption = lst->Strings[32];


   LNetCap->Caption = "��� :" + lst->Strings[35];
   LNetMAC->Caption = "MAC :" + lst->Strings[34];
   LNetStatus->Caption = "������: ����������";
//��������� �� �������
   AnsiString page,ip;
   try
 {
  page=IdHTTP1->Get("http://yoip.ru/");
 }
	catch (...)
 {
  ip="Error";
 };
//Memo2->Lines->Add(page);
 page.Delete(1,page.Pos("IP-�����:"));//������� ��� � ������ ��� IP
 page.Delete(1,page.Pos("<p")+13);//������� ��� ����� ������ ��� IP + <span>
 ip=page.SubString(1,page.Pos("<")-1);// ������ ��
	LNetIP->Caption="IP �����: "+ip;



	MEMORYSTATUSEX stats;           //������������� ������
stats.dwLength = sizeof(stats);
GlobalMemoryStatusEx(&stats);

__int64 TotalRAM =(stats.ullTotalPhys/(1024 * 1024) ),
		AvailRAM =(stats.ullAvailPhys /(1024 * 1024)),
		TotalPageF = (stats.ullAvailPageFile /(1024 * 1024));


switch (TOSVersion::Architecture) {
case TOSVersion::arIntelX86:
	LArch->Caption="Architecture: IntelX86";
								break;
case TOSVersion::arIntelX64:
  LArch->Caption="Architecture: IntelX64";
								break;
}

	LWin->Caption=(TOSVersion::ToString());
	LRam->Caption="����� ���: "+IntToStr(TotalRAM)+" ��";
	LAvailRam->Caption="������: "+IntToStr(AvailRAM)+" ��";

		CGaugeRAM->Progress=stats.dwMemoryLoad; //�������� ������
		//FQuickMenu->CGauge1->Progress=stats.dwMemoryLoad;
	Label5->Caption="���� ��������: "+IntToStr(TotalPageF)+" ��";





}
//---------------------------------------------------------------------------




void __fastcall TInnerWorld::Button4Click(TObject *Sender)
{
	UpdateModuleForm->CheckUpdate(gCurrentVersion, "http://innerworld.moy.su/Update.inf", true, true, true);
}
//---------------------------------------------------------------------------








void __fastcall TInnerWorld::Image11Click(TObject *Sender)
{
ShellExecute( 0 ,L"open", L"gpedit.msc", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image14Click(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"magnify", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image13Click(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"narrator", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::Image16Click(TObject *Sender)
{
ShellExecute( 0 ,L"open", L"mstsc", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------




void TInnerWorld::InfoOfDisk(void)
{
	TCGauge * Ga[5];
	int k = 0, g=0;
	int sz = GetLogicalDriveStrings(NULL, 0);
	TCHAR* szLogicalDrives = new TCHAR[sz];
	GetLogicalDriveStrings(sz, szLogicalDrives);
	while (*szLogicalDrives)
	{
        TCHAR szDisk[80];
        lstrcpy(szDisk, szLogicalDrives);
        UINT uDriveType = GetDriveType(szDisk);
        if (uDriveType == DRIVE_FIXED) //������� ����
		{

		   //	k = k + 1; //������� ���-�� ������� ������
					Ga[k]= new TCGauge(GmenuDisk);
			//wcout << szDisk << endl; //���������� ��������
			__int64 Free,Total;
			Free = 0;
			Total = 0;
			GetDiskFreeSpaceEx(szDisk, NULL, (PULARGE_INTEGER)&Total, (PULARGE_INTEGER)&Free); //���������� �����
			//cout << "����� ����� : " << Total / (1024 * 1024) << " ��" << endl;
			//cout << "�������� : " << Free / (1024 * 1024) << " ��" << endl;
			//cout << "������ : " << (Total - Free) / (1024 * 1024) << " ��" << endl;
		InfoDiskMemo->Lines->Add(szDisk);
		InfoDiskMemo->Lines->Add("����� �����: " +IntToStr( Total / (1024 * 1024) / 1024 ) );
		InfoDiskMemo->Lines->Add("��������: "    +IntToStr( Free / (1024 * 1024)/ 1024   ) );
		InfoDiskMemo->Lines->Add("������: "      +IntToStr( (Total - Free) / (1024 * 1024)/ 1024) );



	Ga[k]->Parent = GmenuDisk;
	Ga[k]->Height = 12;
	Ga[k]->Width  = 100;
	Ga[k]->Top 	  = 35;
	Ga[k]->Left   = 70+g;
	Ga[k]->Font->Quality= TFontQuality::fqAntialiased;
	Ga[k]->ForeColor=StringToColor("$00E3B333");
	Ga[k]->Visible = true;

		Ga[k]->MaxValue=(Total / (1024 * 1024) / 1024 );
		Ga[k]->Progress=((Total - Free) / (1024 * 1024) / 1024);
		Disk1->Caption=InfoDiskMemo->Lines->Strings[0]+"\n"+"\n"+InfoDiskMemo->Lines->Strings[1]+"\n"+InfoDiskMemo->Lines->Strings[2]+"\n"+InfoDiskMemo->Lines->Strings[3];
		Disk2->Caption=InfoDiskMemo->Lines->Strings[4]+"\n"+"\n"+InfoDiskMemo->Lines->Strings[5]+"\n"+InfoDiskMemo->Lines->Strings[6]+"\n"+InfoDiskMemo->Lines->Strings[7];
		Disk3->Caption=InfoDiskMemo->Lines->Strings[8]+"\n"+"\n"+InfoDiskMemo->Lines->Strings[9]+"\n"+InfoDiskMemo->Lines->Strings[10]+"\n"+InfoDiskMemo->Lines->Strings[11];

		//memory=(Total / (1024 * 1024) / 1024);

		//CGauge3->Progress=((Total - Free) / (1024 * 1024) / 1024);
		k++;
		g+=170;
	   }
	 while (*szLogicalDrives) szLogicalDrives++;
	 szLogicalDrives++;


	}
	Memo2->Lines->Add("���������� ������ "+ IntToStr(k));
}
//---------------------------------------------------------------------------



















void __fastcall TInnerWorld::Image27MouseLeave(TObject *Sender)
{
	Image27->Top+=4;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image27MouseEnter(TObject *Sender)
{
   Image27->Top-=4;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image28MouseEnter(TObject *Sender)
{
	Image28->Top-=4;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image28MouseLeave(TObject *Sender)
{
	Image28->Top+=4;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image28Click(TObject *Sender)
{
		ProcessList();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image27Click(TObject *Sender)
{
	CloseProcessByPID();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image29Click(TObject *Sender)
{
	if (GraphPanel->Visible==false)
	{
		GraphPanel->Visible=true;
		+	strcpy(windowname,"CPU usage");

	ZeroMemory(&CurrentSysProcTimes[0],sizeof(CurrentSysProcTimes));
	ZeroMemory(&PreviousSysProcTimes[0],sizeof(PreviousSysProcTimes));

	lib = LoadLibrary(L"Ntdll.dll");   //������������ �������� ����������

	if (!lib)
	{
		msg = "Error #1: �� ������� ����������� ��������� ����������!";
		Application->MessageBox(msg.c_str(), L"Warning", MB_OK | MB_ICONERROR);
		return;
	}

	func = (ZwQuerySystemInformation)GetProcAddress(lib,"ZwQuerySystemInformation");  //������ ������ �������

	if (!func)
	{
		msg = "Error #2: �� ������� �������� ����� �������!";
		Application->MessageBox(msg.c_str(), L"Warning", MB_OK | MB_ICONERROR);
		FreeLibrary(lib);
		return;
	}
   //	Memo1->Lines->Add("���������� ����: "+IntToStr(GetProcessorsCount()));
	init = true;
	//msg = "������������� �������!";
	//Application->MessageBox(msg.c_str(), (String(windowname)).c_str(), MB_OK | MB_ICONINFORMATION);

	Button3->Enabled = true;

	cpu_usage = new int[100000];
	count_mas = 0;
	}
	else
		GraphPanel->Visible=false;
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::Image29MouseEnter(TObject *Sender)
{
   Image29->Top+=4;
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::Image29MouseLeave(TObject *Sender)
{
	Image29->Top-=4;
}
//---------------------------------------------------------------------------











//---------------------------------------------------------------------------








void __fastcall TInnerWorld::SleepSBClick(TObject *Sender)
{
    if (Shutdown==0)
	{
	  system("shutdown /h /t 10");
	  Shutdown=1;
	}
	else
	  {
		system("shutdown /a");
		Shutdown=0;
	  }

}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::ShutdownSBClick(TObject *Sender)
{

	if (Shutdown==0)
	{
	  system("shutdown /s /t 10");
	  Shutdown=1;
	}
	else
	  {
		system("shutdown /a");
		Shutdown=0;
	  }



}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::RestartSBClick(TObject *Sender)
{
   if (Shutdown==0)
	{
	  system("shutdown /r /t 10");
	  Shutdown=1;
	}
	else
	  {
		system("shutdown /a");
		Shutdown=0;
	  }

}
//---------------------------------------------------------------------------



void __fastcall TInnerWorld::RebootSetClick(TObject *Sender)
{
     if (Shutdown==0)
	{
	  system("shutdown /o /r /t 10");
	  Shutdown=1;
	}
	else
	  {
		system("shutdown /a");
		Shutdown=0;
	  }

}
//---------------------------------------------------------------------------



void __fastcall TInnerWorld::Button6Click(TObject *Sender)
{
	FQuickMenu->Label2->Caption="������: ";
}
//---------------------------------------------------------------------------


void __fastcall TInnerWorld::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	CanClose=false;
	InnerWorld->Hide();
	FQuickMenu->Show();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N12Click(TObject *Sender)
{
   FQuickMenu->Show();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N13Click(TObject *Sender)
{
   InnerWorld->Show();
}
//---------------------------------------------------------------------------

void __fastcall TInnerWorld::N14Click(TObject *Sender)
{
   InnerWorld->Close();
   FQuickMenu->Close();
}
//---------------------------------------------------------------------------


