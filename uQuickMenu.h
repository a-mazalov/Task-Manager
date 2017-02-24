//---------------------------------------------------------------------------

#ifndef uQuickMenuH
#define uQuickMenuH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include <Vcl.Buttons.hpp>
#include <Vcl.Imaging.jpeg.hpp>
#include "cgauges.h"
#include <Vcl.Graphics.hpp>
#include <Vcl.Mask.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFQuickMenu : public TForm
{
__published:	// IDE-managed Components
	TCGauge *CGauge1;
	TLabel *LRAM;
	TLabel *Label2;
	TTimer *Timer1;
	TImage *Image2;
	TImage *ImPanel;
	TImage *ImInternet;
	TImage *ImKeyboard;
	TImage *ImNone;
	TImage *ImControlPanel;
	TImage *ImCMD;
	TImage *ImReboot;
	TImage *ImShutdown;
	TImage *ImReg;
	TImage *ImDevice;
	TImage *ImService;
	TImage *Image1;
	TLabel *Label1;
	TImage *Image3;
	TButton *Button1;
	TTimer *Timer2;
	TImage *Image4;
	TTimer *Timer3;
	TTimer *Timer4;
	TTimer *Timer5;
	TMaskEdit *MaskEdit1;
	TLabel *Label3;
	TImage *Image5;
	TTimer *TimerOFF;
	TLabel *Label_Time;
	TProgressBar *Time_Progress;
	TCheckBox *CB_Shutdown;
	TImage *Btn_OkClick;
	TImage *Btn_Start_Stop;
	void __fastcall Timer1Timer(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Button4Click(TObject *Sender);
	void __fastcall Image2Click(TObject *Sender);
	void __fastcall ImPanelMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y);
	void __fastcall Button5Click(TObject *Sender);
	void __fastcall ImInternetMouseEnter(TObject *Sender);
	void __fastcall ImInternetMouseLeave(TObject *Sender);
	void __fastcall ImInternetClick(TObject *Sender);
	void __fastcall ImKeyboardMouseEnter(TObject *Sender);
	void __fastcall ImKeyboardMouseLeave(TObject *Sender);
	void __fastcall ImNoneMouseEnter(TObject *Sender);
	void __fastcall ImNoneMouseLeave(TObject *Sender);
	void __fastcall ImShutdownMouseEnter(TObject *Sender);
	void __fastcall ImShutdownMouseLeave(TObject *Sender);
	void __fastcall ImRebootMouseEnter(TObject *Sender);
	void __fastcall ImRebootMouseLeave(TObject *Sender);
	void __fastcall ImCMDMouseEnter(TObject *Sender);
	void __fastcall ImControlPanelMouseEnter(TObject *Sender);
	void __fastcall ImControlPanelMouseLeave(TObject *Sender);
	void __fastcall ImCMDMouseLeave(TObject *Sender);
	void __fastcall ImControlPanelClick(TObject *Sender);
	void __fastcall ImCMDClick(TObject *Sender);
	void __fastcall ImRebootClick(TObject *Sender);
	void __fastcall ImShutdownClick(TObject *Sender);
	void __fastcall ImKeyboardClick(TObject *Sender);
	void __fastcall ImServiceClick(TObject *Sender);
	void __fastcall ImRegClick(TObject *Sender);
	void __fastcall ImDeviceClick(TObject *Sender);
	void __fastcall Image1Click(TObject *Sender);
	void __fastcall Image3Click(TObject *Sender);
	void __fastcall Timer2Timer(TObject *Sender);
	void __fastcall Image4Click(TObject *Sender);
	void __fastcall Timer3Timer(TObject *Sender);
	void __fastcall Timer4Timer(TObject *Sender);
	void __fastcall Timer5Timer(TObject *Sender);
	void __fastcall Image5Click(TObject *Sender);
	void __fastcall TimerOFFTimer(TObject *Sender);
	void __fastcall Btn_OkClickClick(TObject *Sender);
	void __fastcall Btn_Start_StopClick(TObject *Sender);

private:	// User declarations
public:		// User declarations
	void InitializationCPU();
	void SizeRam();
	__fastcall TFQuickMenu(TComponent* Owner);
bool shoot(bool poweroff, bool shutdown, bool reboot, bool force)
{
OSVERSIONINFO ver;
ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
GetVersionEx(&ver);
if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) // � ��������� NT ��� ���������� ���������� ����� ���������� SE_SHUTDOWN_NAME
        {
        HANDLE hToken;
        TOKEN_PRIVILEGES* NewState;
        OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken);
        NewState=(TOKEN_PRIVILEGES*)malloc(sizeof(TOKEN_PRIVILEGES) + sizeof (LUID_AND_ATTRIBUTES));
        NewState->PrivilegeCount = 1;
        LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&NewState->Privileges[0].Luid);
        NewState->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, NewState, NULL, NULL,NULL);
        free(NewState);
        CloseHandle(hToken);
        }

UINT mode = 0;
        if (poweroff)
		{
			//system("shutdown /r /t 10");
			mode = EWX_POWEROFF;
        }
        if (shutdown)
        {
				//system("shutdown /r /t 10");
				mode = EWX_SHUTDOWN;
        }
        if (reboot)
        {
				mode = EWX_REBOOT;
        }
        if (force)
        {
				mode = EWX_FORCE;
        }

return ExitWindowsEx(mode ,0);

}
//---------------------------------------------------------------------------

int TimeToInt(AnsiString Time)
{
String h = 0, sub_h = 0, m = 0, sub_m = 0, s = 0, sub_s = 0;
int ret_h, ret_m, ret_s;

        h = Time.SubString(1,1).Trim();
        sub_h = Time.SubString(2,1).Trim();
        m = Time.SubString(4,1).Trim();
        sub_m = Time.SubString(5,1).Trim();
        s = Time.SubString(7,1).Trim();
        sub_s = Time.SubString(8,1).Trim();
                if(h != "0")
                {
                        h += sub_h;
                }
                else
                {
                        h = sub_h;
                }
                if(m != "0")
                {
                        m += sub_m;
                }
                else
                {
                        m = sub_m;
                }
                if(s != "0")
                {
                        s += sub_s;
                }
                else
                {
                        s = sub_s;
                }

        ret_h = StrToInt(h)*3600;
        ret_m = StrToInt(m)*60;
        ret_s = StrToInt(s);

  return ret_h + ret_m + ret_s;
}
};
//---------------------------------------------------------------------------
extern PACKAGE TFQuickMenu *FQuickMenu;
//---------------------------------------------------------------------------
#endif
