//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "uQuickMenu.h"
#include "uSplitView.h"
#include "uCmdForm.h"
#include "uStartP.h"
#include <math.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "cgauges"
#pragma resource "*.dfm"
#define SystemProcessorTimes 8
#define MAX_PROCESSORS 32

TFQuickMenu *FQuickMenu;
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
bool init = false;



//---------------������ ���������� ��------------
int h = 0,
	sub_h = 0,
	m = 0,
	sub_m = 0,
	s = 0,
	sub_s = 0,
	i = 0;
bool poweroff = false,
	 shutdow = false,
	 reboot = false,
	 force = false,
	 started = false,
	 goon = false,
	 wait = false;
//------------------------------------------------
//---------------------------------------------------------------------------
__fastcall TFQuickMenu::TFQuickMenu(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------




void __fastcall TFQuickMenu::Timer1Timer(TObject *Sender)
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
			Label2->Caption=msg;
		count_mas++;
		}

		cycle_count++;
//        Edit2->Text = IntToStr(cycle_count);

		memcpy(&PreviousSysProcTimes[0],
				&CurrentSysProcTimes[0],
				sizeof(PreviousSysProcTimes));
}
//---------------------------------------------------------------------------




void __fastcall TFQuickMenu::FormClose(TObject *Sender, TCloseAction &Action)
{
	if (Timer1->Enabled)
	{
	Timer1->Enabled = false;

	//cycle_count = 0;
	temp = 0;
	curtime = 0;
	oldtime = GetTickCount();

	//Button2->Enabled = false;
	//Button3->Enabled = true;
	}
if (lib) FreeLibrary(lib);
delete[] cpu_usage;
bool init = false;
}
//---------------------------------------------------------------------------



void TFQuickMenu::SizeRam()
{
   	MEMORYSTATUSEX stats;           //������������� ������
	stats.dwLength = sizeof(stats);
	GlobalMemoryStatusEx(&stats);

	__int64 TotalRAM =(stats.ullTotalPhys/(1024 * 1024) ),
		AvailRAM =(stats.ullAvailPhys /(1024 * 1024)),
		TotalPageF = (stats.ullAvailPageFile /(1024 * 1024));

	//LWin->Caption=(TOSVersion::ToString());
	//LRam->Caption="����� ���: "+IntToStr(TotalRAM)+" ��";

	LRAM->Caption="�������� ���: "+IntToStr(AvailRAM)+" ��";
	CGauge1->Progress=stats.dwMemoryLoad; //�������� ������


		//FQuickMenu->CGauge1->Progress=stats.dwMemoryLoad;
	//Label5->Caption="���� ��������: "+IntToStr(TotalPageF)+" ��";
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::FormCreate(TObject *Sender)
{
FQuickMenu->Left=1460;
FQuickMenu->Top=300;
TimerOFF->Interval = 1000;
Btn_OkClickClick(Sender);

  StartForm = new TStartForm(this);
  StartForm->ShowModal();
  delete StartForm;

	SizeRam();
	if (init)
	{
		InitializationCPU();
	}

		if (init)
	{

	//Button3->Enabled = false;

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
	//Button2->Enabled = true;
	Timer1->Interval = interval;
	Timer1->Enabled = true;
	}
}
//---------------------------------------------------------------------------

void TFQuickMenu::InitializationCPU()
{
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
	//Application->MessageBox(msg.c_str(),0, MB_OK | MB_ICONINFORMATION);

	//Button3->Enabled = true;

	cpu_usage = new int[100000];
	count_mas = 0;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::FormDestroy(TObject *Sender)
{
	if (Timer1->Enabled)
	{
	Timer1->Enabled = false;

	//cycle_count = 0;
	temp = 0;
	curtime = 0;
	oldtime = GetTickCount();

	//Button2->Enabled = false;
	//Button3->Enabled = true;
	}
if (lib) FreeLibrary(lib);
delete[] cpu_usage;
bool init = false;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::FormShow(TObject *Sender)
{
	SizeRam();
	InitializationCPU();

		if (init)
	{
	//Button3->Enabled = false;

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
	//Button2->Enabled = true;
	Timer1->Interval = interval;
	Timer1->Enabled = true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Button4Click(TObject *Sender)
{
	if (Timer1->Enabled)
	{
	Timer1->Enabled = false;

	//cycle_count = 0;
	temp = 0;
	curtime = 0;
	oldtime = GetTickCount();

	//Button2->Enabled = false;
	//Button3->Enabled = true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Image2Click(TObject *Sender)
{
    FQuickMenu->Hide();
	InnerWorld->OnCreate;
	InnerWorld->Show();

}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImPanelMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
ReleaseCapture();
SendMessage(FQuickMenu->Handle,WM_NCLBUTTONDOWN,HTCAPTION,0);
}
//---------------------------------------------------------------------------



void __fastcall TFQuickMenu::Button5Click(TObject *Sender)
{
	if (FQuickMenu->Height<=217)
	{
	  FQuickMenu->Height=333;
	}
	else
	  FQuickMenu->Height=217;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImInternetMouseEnter(TObject *Sender)
{
	ImInternet->Top+=8;

}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImInternetMouseLeave(TObject *Sender)
{
	ImInternet->Top-=8;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImInternetClick(TObject *Sender)
{
	system("control/name Microsoft.NetworkAndSharingCenter");
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImKeyboardMouseEnter(TObject *Sender)
{
	ImKeyboard->Top+=8;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImKeyboardMouseLeave(TObject *Sender)
{
	ImKeyboard->Top-=8;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImNoneMouseEnter(TObject *Sender)
{
	ImNone->Top+=8;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImNoneMouseLeave(TObject *Sender)
{
	ImNone->Top-=8;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImShutdownMouseEnter(TObject *Sender)
{
	ImShutdown->Left-=6;
   ImShutdown->Top+=6;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImShutdownMouseLeave(TObject *Sender)
{
   ImShutdown->Left+=6;
   ImShutdown->Top-=6;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImRebootMouseEnter(TObject *Sender)
{
   ImReboot->Left-=6;
   ImReboot->Top+=6;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImRebootMouseLeave(TObject *Sender)
{
  ImReboot->Left+=6;
  ImReboot->Top-=6;
}
//---------------------------------------------------------------------------


void __fastcall TFQuickMenu::ImCMDMouseEnter(TObject *Sender)
{
  ImCMD->Top-=6;
  ImCMD->Left-=6;

}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImCMDMouseLeave(TObject *Sender)
{
  ImCMD->Top+=6;
  ImCMD->Left+=6;
}
//---------------------------------------------------------------------------


void __fastcall TFQuickMenu::ImControlPanelMouseEnter(TObject *Sender)
{
  ImControlPanel->Top-=6;
  ImControlPanel->Left-=6;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImControlPanelMouseLeave(TObject *Sender)
{
  ImControlPanel->Top+=6;
  ImControlPanel->Left+=6;
}
//---------------------------------------------------------------------------


void __fastcall TFQuickMenu::ImControlPanelClick(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"control", 0, 0, SW_SHOW);
	if (FQuickMenu->Left==540)
	{
	   Timer3->Enabled=true;
	}
	Image5->Visible=false;
	Image4->Visible=true;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImCMDClick(TObject *Sender)
{
	cmdForm->Show();
	if (FQuickMenu->Left==540)
	{
	   Timer3->Enabled=true;
	}
	Image5->Visible=false;
	Image4->Visible=true;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImRebootClick(TObject *Sender)
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

void __fastcall TFQuickMenu::ImShutdownClick(TObject *Sender)
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

void __fastcall TFQuickMenu::ImKeyboardClick(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"osk", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImServiceClick(TObject *Sender)
{
	system("services.msc");
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImRegClick(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"regedit.exe", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::ImDeviceClick(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"devmgmt.msc", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Image1Click(TObject *Sender)
{
	ShellExecute( 0 ,L"open", L"eventvwr.msc", 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Timer2Timer(TObject *Sender)
{
   FQuickMenu->Left=FQuickMenu->Left-40;
   if (FQuickMenu->Left==540)
   {
	 Timer2->Enabled=false;
   }
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Image4Click(TObject *Sender)
{

	if (FQuickMenu->Left==1460)
	{
	   Timer2->Enabled=true;
	}

	if (FQuickMenu->Top!=300)
	{
	   FQuickMenu->Left=1460;
	   FQuickMenu->Top=300;
	}
	Image4->Visible=false;
	Image5->Visible=true;

}
//---------------------------------------------------------------------------
		///	FQuickMenu->Left=1485; //FQuickMenu->Top=300;
void __fastcall TFQuickMenu::Timer3Timer(TObject *Sender)
{
   FQuickMenu->Left=FQuickMenu->Left+20;
   if (FQuickMenu->Left==1460)
   {
	 Timer3->Enabled=false;
   }
}
//---------------------------------------------------------------------------
void __fastcall TFQuickMenu::Image3Click(TObject *Sender)
{
	if (FQuickMenu->Height==230)
	{
	  Timer5->Enabled=true;
	}
	if (FQuickMenu->Height==350)
	{
	  Timer4->Enabled=true;
	}
}
//---------------------------------------------------------------------------
void __fastcall TFQuickMenu::Timer4Timer(TObject *Sender)
{
   FQuickMenu->Height=FQuickMenu->Height-10;
   if (FQuickMenu->Height==230)
   {
	 Timer4->Enabled=false;
   }
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Timer5Timer(TObject *Sender)
{
   FQuickMenu->Height=FQuickMenu->Height+10;
   if (FQuickMenu->Height==350)
   {
	 Timer5->Enabled=false;
   }
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Image5Click(TObject *Sender)
{
	if (FQuickMenu->Left==540)
	{
	   Timer3->Enabled=true;
	}
	Image5->Visible=false;
	Image4->Visible=true;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::TimerOFFTimer(TObject *Sender)
{
// ����� ��� ��� ���������� �������
  // 12:34:56
  // 1 - h
  // 2 - sub_h
  // 3 - m
  // 4 - sub_m
  // 5 - s
  // 6 - sub_s

  sub_s--;
  i++;
  Time_Progress->Position = i;
	 if(sub_s < 0)
	 {
		sub_s = 9;
		s--;
				if(s < 0)
				{
						s = 5;
						sub_m--;
				}
	 }
	 if(sub_m < 0)
	 {
		sub_m=9;
		m--;
				if(m < 0)
				{
						m = 5;
						sub_h--;
				}
	 }
	 if(sub_h < 0)
	 {
		sub_h = 9;
		h--;
				if(h < 0)
				{
						h = 0;  sub_h = 0;
						m = 0;  sub_m = 0;
						s = 0;  sub_s = 0;
						goon = true;    i = 0;
				}
	 }
  // ������� ��� �� �����
	 Label_Time->Caption = String(h) + String(sub_h) + ":" +
						String(m) + String(sub_m) + ":" + String(s) + String(sub_s);
						  // ���� ����, ��
	 if(goon)
	 {
		shoot(poweroff, shutdown, reboot, force);
		   InnerWorld->Close();
		   FQuickMenu->Close();
	 }

}
//---------------------------------------------------------------------------




void __fastcall TFQuickMenu::Btn_OkClickClick(TObject *Sender)
{
// ��������� �������� �����
		//AutoSize = false;
		wait = false;
		i=0;
		Time_Progress->Position = 0;
		started = true;
  // ���������� ������ ������� �� �������, ������������ ��������� ��������� ������� �������.
		/*if(CB_Poweroff->Checked)
		{
										poweroff = true;
										shutdow = false;
										reboot = false;
										force = false;
		}
		  */
  // ����������� ���������� ������� ��
		if(CB_Shutdown->Checked)
		{
										poweroff = false;
										shutdow = false;
										reboot = false;
										force = false;
		}
		/*
  // ����������� ���������� ���������� ��
		if(CB_Reboot->Checked)
		{
										poweroff = false;
										shutdown = false;
										reboot = true;
										force = false;
		}
  // ����������� ���������� ������� ��
		if(CB_Force->Checked)
		{
										poweroff = false;
										shutdown = false;
										reboot = false;
										force = true;
		}

		*/
  // ����������� ���������� �����
		h = MaskEdit1->Text.SubString(1, 1).ToInt();
		sub_h = MaskEdit1->Text.SubString(2, 1).ToInt();
		m = MaskEdit1->Text.SubString(4, 1).ToInt();
		sub_m = MaskEdit1->Text.SubString(5, 1).ToInt();
		s = MaskEdit1->Text.SubString(7, 1).ToInt();
		sub_s = MaskEdit1->Text.SubString(8, 1).ToInt();
   // �������� ������ �����
		//Btn_Start_Stop->Caption = "�����";
   // ������� �������� �����
		Label_Time->Caption = String(h) + String(sub_h) + ":" +
						String(m) + String(sub_m) + ":" + String(s) + String(sub_s);
  // �������� ������ ��������
		//Panel_Setting->Visible = false;
  // ���������� ������ � �������� �������
		//Panel_Time->Visible = true;
  // Start
		started = true;
  // ��������� ���� �������� �����������
		Time_Progress->Max = TimeToInt(Label_Time->Caption);
		Time_Progress->Min = 0;
  // ��������� �������� �����
	   // AutoSize = true;
}
//---------------------------------------------------------------------------

void __fastcall TFQuickMenu::Btn_Start_StopClick(TObject *Sender)
{
  if(started)
		{
  // ���� ������ ��������, �� �������� ������
				if(!TimerOFF->Enabled)
				{
						if(wait)
						{
						  // ����������� ���������� �����
								h = Label_Time->Caption.SubString(1, 1).ToInt();
								sub_h = Label_Time->Caption.SubString(2, 1).ToInt();
								m = Label_Time->Caption.SubString(4, 1).ToInt();
								sub_m = Label_Time->Caption.SubString(5, 1).ToInt();
								s = Label_Time->Caption.SubString(7, 1).ToInt();
								sub_s = Label_Time->Caption.SubString(8, 1).ToInt();
						  // ��������� ������
								TimerOFF->Enabled = true;
								//Btn_Start_Stop->Caption = "����";
						}
						else
						{
								TimerOFF->Enabled = true;
								wait = true;
								//Btn_Start_Stop->Caption = "����";
						}
				}
  // ���� ������ �������, �� ���������
				else
				{
						TimerOFF->Enabled = false;
						if(wait)
						{
							   //	Btn_Start_Stop->Caption = "����������";
						}
						else
						{
								//Btn_Start_Stop->Caption = "�����";
						}
				}
		}

}
//---------------------------------------------------------------------------

