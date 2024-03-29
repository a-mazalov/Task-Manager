//---------------------------------------------------------------------------

#pragma hdrstop
#include "UnitProcess.h"
#include "uSplitView.h"
#include <tlhelp32.h>
#include <windows.h>
#include <psapi.h>
#include <stdlib.h>
//---------------------------------------------------------------------------

#pragma package(smart_init)

//---------------------------------------------------------------------------
int PS(int a,int b)
{
	return (a+b);
}

  void GetProcessList()
  {

	  //ListView1->Clear();
	HANDLE InfoProcess=0;
	HANDLE hToken=0;
	DWORD PID;
    bool EnumProcess;
    bool EnumThread;
   //	bool EnumModule;
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
	//EnumModule  = Module32First  (ModuleSnap,  &Module);

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
		//ListView1->Items->Add();
		//Item = ListView1->Items->Item[ListView1->Items->Count-1];
//-------------------------------���� ���������� � ���������---------------------------------------

        InfoProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
		Process.th32ProcessID);
    if (!(OpenProcessToken(InfoProcess, TOKEN_QUERY, &hToken)))
    {
		//ListBox3->Items->Add("OpenToken " + SysErrorMessage(GetLastError()));
	}

	pUserInfo = (PTOKEN_USER) GlobalAlloc(GPTR, dwSize);
    if (!(GetTokenInformation(hToken, TokenUser, pUserInfo, dwSize, &dwSize)))
	{
		//SplitViewForm->ListBox3->Items->Add("GetTokenInfo " + AnsiString(GetLastError()));
	}

	// ������ ��������� ����� ������������
    if (!(LookupAccountSidA(0, pUserInfo->User.Sid, name, &dwSize, domain,
        &dwSize, (PSID_NAME_USE) & iUse)))
	{
	   //	SplitViewForm->ListBox3->Items->Add("Lookup " + SysErrorMessage(GetLastError()));
	}

		//Item->Caption=PID;// �� ��������
		//Item->ImageIndex=0;//��������
		//Item->SubItems->Add(Process.szExeFile); // ��� ��������

	   /*	switch (GetPriorityClass(InfoProcess))
			{
			  case 256: Item->SubItems->Add("��������"); break;
			  case 128:  Item->SubItems->Add("�������"); break;
			  case  32:  Item->SubItems->Add("����������"); break;
			  case  64:  Item->SubItems->Add("������");  break;
			  case  32768:  Item->SubItems->Add("����������+"); break;
			  case  16384:  Item->SubItems->Add("������+");  break;
			}
		 */
		//Item->SubItems->Add(name);//��������� ������ ��������
	  //	Item->SubItems->Add(Module.szModule); //������ ��������
	 //	Item->SubItems->Add(Process.cntThreads); //���-�� �������

		GetModuleFileNameEx(InfoProcess, NULL, pathP, MAX_PATH ); //���� �������� ������������ � pBuf
			 //Item->SubItems->Add(pathP);
			//Item->SubItems->Add("����������");

	   PInf *Inf = new PInf;
	   Inf->PID =Process.th32ProcessID;
	   Inf->Name=Process.szExeFile;
	   Inf->User=name;
	   Inf->Module=Module.szModule;
	   Inf->FullPatch=pathP;
	   Inf->Threads=Process.cntThreads;
	   Inf->ColP=ColP++;
	   Inf->Priority=GetPriorityClass(InfoProcess);



	   //SplitViewForm->Item->Caption=Inf->Name;
		EnumProcess = Process32Next(ProcessSnap, &Process);
       // EnumModule  = Module32Next (ModuleSnap,  &Module);

		//Label1->Caption=IntToStr(ColP++);
	  }
  }
	CloseHandle(ProcessSnap);
	CloseHandle(ModuleSnap);
  }
