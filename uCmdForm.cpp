//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <memory>
#include <string>
#include <iostream>
#include <istream>
#include <fstream>
// vcl
#include <IOUtils.hpp>
#include <StrUtils.hpp>
#include "uCmdForm.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TcmdForm *cmdForm;
//---------------------------------------------------------------------------
__fastcall TcmdForm::TcmdForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void AddOemLine(TStrings *AStrings, ifstream & /* out */ ifile)
{
	std::string sLine;
	sLine="";
	std::getline(ifile, sLine);
	char *chBuff = new char[sLine.size() + 1];
	OemToAnsi(sLine.c_str(), chBuff);
	AStrings->Add(chBuff);
	delete[]chBuff;
}
//---------------------------------------------------------------------------
void CMD2Memo(TStrings *AStrings, const AnsiString &sCommandLine)
{
	// �������� ���������� ���������
	const AnsiString sRootDir = ExtractFilePath(Application->ExeName);
	// ��������� �����, ���� ����� �������������� �������
	const AnsiString sTempFileName = sRootDir + "temp_file_name~.cmd";
	const AnsiString sOutputFile = sRootDir + "stdout~.txt";
	// ��������� ������ ���������� � ������
	TFile::WriteAllText(sTempFileName, sCommandLine);

	// ���������� ������� ����� ����������, ��� ������ ��� �����������
	DWORD dwAppRunning = 0;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// ������ ����, ���� ����� ���������������� ����� �� �������
	HANDLE hFile = CreateFileA(sOutputFile.c_str(), GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, 0, NULL);

	SetFilePointer(hFile, 0, NULL, FILE_END);

	STARTUPINFOA si = {
		sizeof(STARTUPINFOA)
	};

	si.hStdOutput = hFile;
	si.hStdError = hFile;
	si.dwFlags = STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi;

	// ��������� ��������� ���� ��� ������
	std::ifstream ifile(sOutputFile.c_str());
	if (!ifile.is_open())
	{
		return;
	}

	// ��������� ������
	bool bSuccess = CreateProcessA(NULL, sTempFileName.c_str(), NULL, NULL, TRUE, 0, 0, 0, &si,
		&pi);
	if (bSuccess) {
		do {
			// ���������, ����������� �� ��� �������
			dwAppRunning = WaitForSingleObject(pi.hProcess, 100);

			// ������ ������ ���������� �� �����
			if (ifile.good()) {
				AddOemLine(AStrings, ifile);
			}

			// ��� ����������� ���������� ���������� �������
			Sleep(5);
			Application->ProcessMessages();

		}
		while (dwAppRunning == WAIT_TIMEOUT);
	}

	// ����� ����, ��� ������� ��������, ������������ ������� ������� ���������� �� �����
	while (ifile.good()) {
		AddOemLine(AStrings, ifile);
	}
	ifile.close();
	CloseHandle(hFile);

	// ��������� ������� ����������
	AStrings->Add(GetCurrentDir() + ">");
}
//---------------------------------------------------------------------------

void __fastcall TcmdForm::MemoInputKeyPress(TObject *Sender, System::WideChar &Key)
{
		if (Key == '\n') {

		MemoOutput->Lines->Add("===========================================");
		MemoOutput->Lines->Add(MemoInput->Lines->Text);
		MemoOutput->Lines->Add("===========================================");

		/* ��� ��� ������� CD �� ������ �� ������� ���������� ������ ��������,
		�� ���������� �������������� ���������� ����� ���������� �������������� */
		for (int i = 0; i < MemoInput->Lines->Count; i++) {
			std::auto_ptr<TStringList>AStringList(new TStringList());
			AStringList->StrictDelimiter = true;
			AStringList->Delimiter = ' ';
			AStringList->DelimitedText = MemoInput->Lines->Strings[i];

			if (AStringList->Count >= 2) {
				const UnicodeString sCommand = AStringList->Strings[0].UpperCase();
				if (sCommand == "CD" || sCommand == "CHDIR") {
					const UnicodeString sChangeDir = AStringList->Strings[1].UpperCase();
					if (sChangeDir == "/D") {
						AStringList->Delete(1);
					}
					AStringList->Delete(0);

					SetCurrentDir(AStringList->DelimitedText);
				}
			}
		}

		// ��������� ��������� ������
		CMD2Memo(MemoOutput->Lines, MemoInput->Lines->Text);

		// ������ ���� ��� �����
		MemoInput->Lines->Clear();
		Key = NULL;

	}
}
//---------------------------------------------------------------------------




void __fastcall TcmdForm::Button1Click(TObject *Sender)
{
	Memo1->Text=MemoOutput->SelText;
	if(MemoOutput->SelText!="")
	{
		if(SaveDialog1->Execute())
		{
			UnicodeString fname = SaveDialog1->FileName;
			Memo1->Lines->SaveToFile(fname);
		}
	}
	else
	{
		if(SaveDialog1->Execute())
		{
			UnicodeString fname = SaveDialog1->FileName;
			MemoOutput->Lines->SaveToFile(fname);
		}
	}


}
//---------------------------------------------------------------------------


void __fastcall TcmdForm::Button2Click(TObject *Sender)
{
	Memo1->Clear();

}
//---------------------------------------------------------------------------

void __fastcall TcmdForm::Image1Click(TObject *Sender)
{
	MemoOutput->Clear();
}
//---------------------------------------------------------------------------

void __fastcall TcmdForm::ImMenuDownMouseEnter(TObject *Sender)
{
	  ImMenuDown->Top+=5;
}
//---------------------------------------------------------------------------

void __fastcall TcmdForm::ImMenuDownClick(TObject *Sender)
{
	if (cmdForm->Height<520)
	{
		cmdForm->Height=520;
		//ImMenuDown->Picture->LoadFromFile("DragListUP.png");
	}
	else
	{
		cmdForm->Height=437;
		//ImMenuDown->Picture->LoadFromFile("DragListDown.png");
	}


}
//---------------------------------------------------------------------------

void __fastcall TcmdForm::ImMenuDownMouseLeave(TObject *Sender)
{
	ImMenuDown->Top-=5;
}
//---------------------------------------------------------------------------

