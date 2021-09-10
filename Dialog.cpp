/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 *	Daz Man 2021 (DM)
 * 
 * Description:
 *	
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <windows.h>
#include <commctrl.h>           // prototypes and defs for common controls 
#include <direct.h>
#include "dialog.h"
#include "main.h"
#include "resource.h"
#include "common/DrmReceiver.h"
#include "common/DrmTransmitter.h"
#include "common/libs/ptt.h"
#include "common/settings.h"
#include "common/list.h"
#include "common/bsr.h"
//#include "common/libs/callsign.h" //removed - updated DM
#include "common/libs/mixer.h"
#include "common/libs/graphwin.h"
#include "getfilenam.h"
#include "common/AudioFir.h"
#include <shellapi.h>
#include "common/callsign2.h"

#include "RS-defs.h" //added DM




#define ZLIB_WINAPI
#define ZLIB_DLL
#define ZLIB_INTERNAL
#include "zlib.h"
#include "common/RS/RS4-coder.h"

//#include "common/datadecoding/DABMOT.cpp"

CDRMReceiver	DRMReceiver;
CDRMTransmitter	DRMTransmitter;

CDataDecoder* DataDecoder;
CParameter*	TransmParam;
CAudioSourceEncoder* AudioSourceEncoder;

// Main dialog handler

#define starttx_time 14 //12 edited DM - This allows the decoder more time to lock
#define starttx_time_long 24 //This allows the decoder even longer to lock
#define stoptx_time 20 //10 edited DM - This stops the end of the transmission being cut short

#define MAX_PATHLEN 255 //in case the routines can't handle a 255 char path like Windows can, we can reduce this (it's limited to 80 chars elsewhere...) DM

//Vars for new combined file header for new RS coding method DM
int LeadIn = 1; //changed to int DM - 1-3 = Instances OLD, 4,5,6,7 = RS1,RS2,RS3,RS4 NEW
string EZHeaderID = "EasyDRFHeader/|"; //header ID string

int EncSegSize = 0; //Encoder current segment size DM
int EncFileSize = 0; //Encoder current file size DM

int RSretryDelay = 0;
int RxRSlevel = 0;	//added by DM to detect RS encoding on incoming file segments, even if file header fails - Used in DABMOT.cpp
int DecFileSize = 0; //Decoder current file size DM
int HdrFileSize = 0; //Decoder current file size DM
int totsize = 0; //total segment count global DM
int SerialFileSize = 0;

int DecCheckReg = 0b00001111111111111111111111111111; //Decoder check register for serial segment total transmission
int RSfilesize = 0; //The size of the RS encoded data (normally this is an even number of 255 byte blocks)

int DecSegSize = 0; //Decoder Segment Size copy
//int DecTotalSegs = 0; //Decoder total segments - keeps getting corrupted somehow?!?! moved down one
int DecTotalSegs = 0; //Decoder total segments - the actual int being used...
int CompTotalSegs = 0; //Computed total segments
int DecVecbSize = 0; //debug the raw data size available

int DecPacketID = 0; //TEST

int RSlastTransportID = 0; //Last RS decoder transport ID
int DecTransportID = 0; //Current decoder transport ID
int BarTransportID = 0;

int RSError = 0; //To display RS decoding errors
bool CRCOK = 0;
int LastGoodCRC = 0;

#define BARL 0 //bargraph left
#define BARY 240 //bargraph Y
#define BART 237 //bargraph top
#define BARB 243 //bargraph bottom
#define BARR 500 //bargraph bottom

int BarLastID = 0; //check if bargraph needs updating
int BarLastSeg = 0; //check if bargraph needs updating
int BarLastTot = 0; //check if bargraph needs resetting
int BarRedraw = 0;
int Bartotsizeold = 0;

//string DMfilename = {}; //added DM 130 is now 260 - (Windows max path length is 255 characters)
char DMfilename[260]; //added DM 130 is now 260 - (Windows max path length is 255 characters)
char DMfilename2[260]; //added DM 130 is now 260 - (Windows max path length is 255 characters)

int lasterror = 0; //save RS error count
int DecPrevSeg = 0; //Decoder previous segment (for detecting the end of the leader and combining the new segment data)
int DecHighSeg = 0; //Decoder highest segment in leader (for combining the new segment data)

char erasures[3][8192 / 8]{}; //array for segment erasure data (saves the first/last good CRC segment numbers)
int  erasuressegsize[3] = {0,0,0}; //save the segment size that was used for each array
int erasureswitch = 0; //which array is being written to currently DM
int erasureindex = 0;
int erasureflags = 0;

int lasterror2 = 0; //a place for functions to return errors to...

//int SetSegSize = 116; // 116; //Set the segment size here - originally 116 - DM - This doesn't do anything!

// Initialize File Path
char rxfilepath[260] = { "Rx Files\\" }; //Added DM
char rxcorruptpath[260] = { "Corrupt\\" }; //Added DM
char bsrpath[260] = { "" }; //Added DM

char GlobalDMRxRSData[1048576]; //1M RS data buffer for receive NEW =====================================================================================
int DMRSindex = 0; //write index for above array
int DMRSpsize = 0; //previous segment size

//moved from further down DM
long file_size[64]; //edited DM was 32
BOOL longleadin = TRUE;
BOOL autoaddfiles = FALSE;

BOOL IsRX = TRUE;
BOOL RX_Running = FALSE;
BOOL TX_Running = FALSE;
BOOL UseTextMessage = FALSE;
BOOL RXTextMessageON = FALSE;
BOOL RXTextMessageWasClosed = FALSE;
BOOL AllowRXTextMessage = FALSE;
BOOL SaveRXFile = TRUE;
BOOL ShowRXFile = FALSE;
BOOL ShowOnlyFirst = TRUE;
BOOL rxaudfilt = FALSE;
BOOL fastreset = FALSE;
BOOL rtsonfac = FALSE;
BOOL dtronfac = FALSE;
BOOL dolog = FALSE;
int sensivity = 60;

//char LeadIn = 1; //moved up the file, and changed to int DM

int TXpicpospt = 0;
FILE * logfile;
string rxcall     = "nocall";
string lastrxcall = "nocall";
string consrxcall = "nocall"; 

int	RSmode = 2; //added DM
//int KKvar = 192; //added DM

int numdevIn; //edited DM
int numdevOut; //edited DM

int disptype = 0; //0=spectr, 1=psd, 2=level
int newdata = 0;
int numbsrsegments = 0;
int acthash = 0;
int bsrposind;
int txbsrposind;
BOOL bCloseMe = FALSE;

HWND bsrhand;

//string namebsrfile;
char namebsrfile[130]; //edit DM
string filetosend;

float specbufarr[300];

char filetitle[32][260]; // = { " " }; //32 was 8, 320 was 80 DM - 320 changed to 260 now (Windows max path length is 255 characters)
char pictfile[32][260]; // = { " " }; //32 was 8, 1200 was 300 DM - 1200 changed to 260 now (Windows max path length is 255 characters)

char lastfilename[260] = { "none" }; //was 130 DM  (Windows max path length is 255 characters)
char lastrxfilename[260] = { 0 }; //was 130 DM (Windows max path length is 255 characters)
int  lastrxfilesize = 0;
ERobMode lastrxrobmode = RM_ROBUSTNESS_MODE_B;
ESpecOcc  lastrxspecocc = SO_1;
CParameter::ECodScheme lastrxcodscheme = CParameter::CS_1_SM;
int lastrxprotlev = 0;

char robmode = ' ';
int specocc = 0;

/* Implementation of global functions *****************************************/
int messtate[20] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 }; //was 10 DM
HWND messhwnd;
HWND RXMessagehwnd;

void ClearBar(HWND hwnd) {
	HDC hdc = GetDC(hwnd);
	HPEN penx;
	LOGBRUSH lbx;
	lbx.lbStyle = BS_SOLID;
	lbx.lbColor = GetSysColor(COLOR_3DFACE);
	lbx.lbHatch = 0;
	penx = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 6, &lbx, 0, NULL);
	SelectObject(hdc, penx); //penx is the window background colour, and 6 pixels square
	MoveToEx(hdc, BARL, BARY, NULL);
	LineTo(hdc, BARR, BARY); //erase the rest of the window
	DeleteObject(penx);
	ReleaseDC(hwnd, hdc);
}

//Stats window display - DM
void sendinfo(HWND hwnd) {
	char tempstr[300];
//	lasterror2 = sprintf_s(tempstr, "RS%d Err:%d Ft:%d St:%d Ss:%d [%s] d%d tid%d-%d", RxRSlevel, lasterror, totsize, DecTotalSegs, DecSegSize, DMfilename, DecFileSize, DecTransportID, erasureswitch);//RSfilesize //added RS level info - in testing - DM //DecFileSize
//	lasterror2 = sprintf_s(tempstr, "RS%d E:%d Ft:%d St:%d Ss:%d [%s] RSf:%d tid:%d-%d hdr:%d", RxRSlevel, lasterror, totsize, DecTotalSegs, DecSegSize, DMfilename, RSfilesize, DecTransportID, erasureswitch, HdrFileSize);//RSfilesize //added RS level info - in testing - DM //DecFileSize
//	wsprintf(tempstr, "RS%d RSerr:%d fS:%d sS:%d File:%s", RxRSlevel, lasterror, totsize, DecTotalSegs, DMfilename); //added RS level info - in testing - DM
//	lasterror2 = sprintf_s(tempstr, "Fsegt:%d Ssegt:%d Ss:%d [%s] RSf:%d tid:%d-%d Hdr:%d", totsize, DecTotalSegs, DecSegSize, DMfilename, RSfilesize, DecTransportID, erasureswitch, HdrFileSize);//RSfilesize //added RS level info - in testing - DM //DecFileSize
	lasterror2 = sprintf_s(tempstr, "[%s] Hdr:%d Bytes:%d tID:%d-%d Segs:%d Err:%d", DMfilename, HdrFileSize, RSfilesize, DecTransportID, erasureswitch, DecTotalSegs , lasterror);//RSfilesize //added RS level info - in testing - DM //DecFileSize
	lasterror2 = SendMessage(GetDlgItem(hwnd, IDC_EDIT6), WM_SETTEXT, 0, (LPARAM)tempstr); //send to stats window DM
}

void PostWinMessage(unsigned int MessID, int iMessageParam)
{
	HWND hdlg;
	if (MessID == MS_RESET_ALL)
	{
		int i;
		for (i=1;i<9;i++) messtate[i] = -1;
		hdlg = GetDlgItem (messhwnd, IDC_LED_IO);
		SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		hdlg = GetDlgItem (messhwnd, IDC_LED_FREQ);
		SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		hdlg = GetDlgItem (messhwnd, IDC_LED_FAC);
		SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		hdlg = GetDlgItem (messhwnd, IDC_LED_MSC);
		SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		hdlg = GetDlgItem (messhwnd, IDC_LED_FRAME);
		SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		hdlg = GetDlgItem (messhwnd, IDC_LED_TIME);
		SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		if (rtsonfac) endrts();
		return;
	}
	if (messtate[MessID] != iMessageParam)
	{
		if (MessID == MS_IOINTERFACE)
			hdlg = GetDlgItem (messhwnd, IDC_LED_IO); //IO LED
		else if (MessID == MS_FREQ_FOUND)
			hdlg = GetDlgItem (messhwnd, IDC_LED_FREQ); //FREQ LED
		else if (MessID == MS_FAC_CRC)
		{
			hdlg = GetDlgItem (messhwnd, IDC_LED_FAC);
			if (rtsonfac) if (iMessageParam == 0) dorts();	else endrts();
			if (dtronfac) if (iMessageParam == 0) dodtr();	else enddtr();
		}
		else if (MessID == MS_MSC_CRC)
			hdlg = GetDlgItem (messhwnd, IDC_LED_MSC);
		else if (MessID == MS_FRAME_SYNC)
			hdlg = GetDlgItem (messhwnd, IDC_LED_FRAME);
		else if (MessID == MS_TIME_SYNC)
			hdlg = GetDlgItem (messhwnd, IDC_LED_TIME);
		else return;
		messtate[MessID] = iMessageParam;
		
		if (iMessageParam == 0)
			SendMessage (hdlg, BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (hdlg, BM_SETCHECK, (WPARAM)0, 0);
		
	}
}

/*--------------------------------------------------------------------
        PAINT WAVE DATA
        Repaint the dialog box's GraphClass display control
  --------------------------------------------------------------------*/

static void PaintWaveData ( HWND hDlg , BOOL erase )
{
    HWND hwndShowWave = GetDlgItem( hDlg, IDS_WAVE_PANE );

    InvalidateRect( hwndShowWave, NULL, erase );
    UpdateWindow( hwndShowWave );
    return;
}

/*--------------------------------------------------------------------
        Threads
  --------------------------------------------------------------------*/

void RxFunction(  void *dummy  )
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	try
	{
		DRMReceiver.Start();	
	}
	catch (CGenErr GenErr)
	{
		RX_Running = FALSE;
		MessageBox( messhwnd,"RX Audio Setup Wrong\nWinDRM needs 2 soundcards or WinXP\nTry -r, -t or -p startup options","RX Exception",0);	
	}
}

void TxFunction(  void *dummy  )
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	try
	{
		DRMTransmitter.Start();
	}
	catch (CGenErr GenErr)
	{
		TX_Running = FALSE;
		MessageBox( messhwnd,"TX Audio Setup Wrong\nWinDRM needs 2 soundcards or WinXP\n Try -r, -t or -p startup options","TX Exception",0);	
	}
}

int Playtype = 0;
BOOL isplaying = FALSE;

void PlaySound(  void *dummy  )
{
	isplaying = TRUE;
	try
	{
		if (Playtype == 1)
		{
			dotx();
			PlaySound("tune.wav",NULL,SND_FILENAME | SND_SYNC  | SND_NOSTOP | SND_NODEFAULT);
			endtx();
		}
		else if (Playtype == 2)
		{
			dotx();
			PlaySound("id.wav",NULL,SND_FILENAME | SND_SYNC | SND_NOSTOP | SND_NODEFAULT);
			endtx();
		}
		else if (Playtype == 3)
		{
			dotx();
			PlaySound("g.wav",NULL,SND_FILENAME | SND_SYNC | SND_NOSTOP | SND_NODEFAULT);
			endtx();
		}
		else if (Playtype == 4)
		{
			dotx();
			PlaySound("b.wav",NULL,SND_FILENAME | SND_SYNC | SND_NOSTOP | SND_NODEFAULT);
			endtx();
		}
	}
	catch (CGenErr GenErr)
	{
		MessageBox( messhwnd,"Works only with WinXP or later","Message",0);	 //edited DM
	}
	isplaying = FALSE;
}

int actdevinr;
int actdevint;
int actdevoutr;
int actdevoutt;

BOOL CALLBACK DialogProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HMENU   hMenu;
	int i;
	int junk = 0; //added DM
    switch (message)
    {
    case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon (TheInstance, MAKEINTRESOURCE (IDI_ICON1));
			SendMessage (hwnd, WM_SETICON, WPARAM (ICON_SMALL), LPARAM (hIcon));
		}
		messhwnd = hwnd;
        SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"TX Voice");
		SendMessage (GetDlgItem (hwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"TX File");

		getvar(); //read settings.txt and set variables from it
		comtx(gettxport());
		checkcomport(hwnd,gettxport());
		//InitBsr(); //DLL version code needs this removed DM =========================================================================================================

		EZHeaderID = "EasyDRFHeader/|000000"; //set basic header ID string DM

//		numdev = DRMReceiver.GetSoundInterface()->GetNumDevIn(); //edited DM - may need rewriting to split it up into In/Out - YEP!
		//Get a list of the input and output sound devices installed on the PC and list them in the menu:
		numdevIn = DRMReceiver.GetSoundInterface()->GetNumDevIn(); //edited DM
		numdevOut = DRMReceiver.GetSoundInterface()->GetNumDevOut(); //edited DM
		SetMixerValues(numdevIn); //??? Find out what this does... DM
		hMenu = GetSubMenu(GetMenu(hwnd), 1);	
		if (numdevIn >= 30) numdevIn = 30; //edited DM - limit to max 30 devices
		if (numdevOut >= 30) numdevOut = 30; //edited DM - limit to max 30 devices
		for (i=0;i<numdevIn;i++)
		{	
			string drivnam = DRMReceiver.GetSoundInterface()->GetDeviceNameIn(i); //get the device name for this i value
			AppendMenu(hMenu, MF_ENABLED, IDM_O_RX_I_DRIVERS0+i, drivnam.c_str()); //add a menu item for this device NO CHECK MARK
		}
		AppendMenu(hMenu, MF_SEPARATOR, -1, NULL);
		AppendMenu(hMenu, MF_ENABLED, -1, "TX Output");
		for (i=0;i<numdevOut;i++)
		{	
			string drivnam = DRMReceiver.GetSoundInterface()->GetDeviceNameOut(i);
			AppendMenu(hMenu, MF_ENABLED, IDM_O_TX_O_DRIVERS0+i, drivnam.c_str());
		}
		if (runmode != 'P')
		{
			AppendMenu(hMenu, MF_SEPARATOR, -1, NULL);
			AppendMenu(hMenu, MF_ENABLED, -1, "Voice Input");
			for (i=0;i<numdevIn;i++)
			{	
				string drivnam = DRMReceiver.GetSoundInterface()->GetDeviceNameIn(i); //edited DM
				AppendMenu(hMenu, MF_ENABLED, IDM_O_VO_I_DRIVERS0+i, drivnam.c_str());
			}
			AppendMenu(hMenu, MF_SEPARATOR, -1, NULL);
			AppendMenu(hMenu, MF_ENABLED, -1, "Voice Output");
			for (i=0;i<numdevOut;i++)
			{	
				string drivnam = DRMReceiver.GetSoundInterface()->GetDeviceNameOut(i); //edited DM
				AppendMenu(hMenu, MF_ENABLED, IDM_O_VO_O_DRIVERS0+i, drivnam.c_str());
			}
		}
		else	// Picture only mode, dont start up voice sound !
		{
			AllowRXTextMessage = FALSE;
			CheckMenuItem(GetMenu(hwnd), ID_CODEC_LPC10, MF_BYCOMMAND | MF_UNCHECKED) ;
			CheckMenuItem(GetMenu(hwnd), ID_CODEC_SPEEX, MF_BYCOMMAND | MF_UNCHECKED) ;
			EnableMenuItem(GetMenu(hwnd), ID_CODEC_SPEEX, MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), ID_CODEC_LPC10, MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_TEXTMESSAGE, MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_TEXTMESSAGE_OPENRXTEXTMESSAGE, MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), ID_SOUNDCARD_OPENMIXER_VOICEINPUT, MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), ID_SOUNDCARD_OPENMIXER_VOICEOUTPUT, MF_GRAYED);
			EnableWindow(GetDlgItem (hwnd, IDB_START),FALSE);
		}
  
		//check the menu items for the selected devices only and clear the rest
		DrawMenuBar(hwnd);
		actdevinr = getsoundin('r');
//		if (actdevinr >= numdevIn) actdevinr = numdevIn - 1; //edited DM for IO - make sure the selected device doesn't overflow the list?
		unchecksoundrx(hwnd,actdevinr + IDM_O_RX_I_DRIVERS0); //edited DM - This now needs a window menu index and not a driver index! So add the menu base offsets. DONE
		actdevoutt = getsoundout('t');
//		if (actdevoutt >= numdevOut) actdevoutt = numdevOut - 1; //edited DM for IO 
		unchecksoundtx(hwnd,actdevoutt + IDM_O_TX_O_DRIVERS0); //edited DM
		actdevint = getsoundin('t');
//		if (actdevint >= numdevIn) actdevint = numdevIn - 1; //edited DM for IO 
		unchecksoundvoI(hwnd,actdevint + IDM_O_VO_I_DRIVERS0); //edited DM
		actdevoutr = getsoundout('r');
//		if (actdevoutr >= numdevOut) actdevoutr = numdevOut - 1; //edited DM for IO 
		unchecksoundvoO(hwnd,actdevoutr + IDM_O_VO_O_DRIVERS0); //edited DM

		if (runmode != 'T')
		{
			try
			{
				DRMReceiver.GetParameters()->bOnlyPicture = (runmode == 'P');
				DRMReceiver.Init();
				DRMReceiver.GetSoundInterface()->SetInDev(actdevinr);
				DRMReceiver.GetSoundInterface()->SetOutDev(actdevoutr);
				DataDecoder = DRMReceiver.GetDataDecoder();
//				DRMReceiver.SetFreqAcqSensivity(2.0 - (_REAL)sensivity/100.0); //edited DM -------------- needs disabling for update -----------------
	
				RX_Running = TRUE;
				_beginthread(RxFunction,0,NULL);
			}
			catch(CGenErr)
			{
				RX_Running = FALSE;
				MessageBox( hwnd,"Receiver will NOT work\nWinDRM needs 2 soundcards or WinXP\nTry -r, -t or -p startup options","RX Init Exception",0);	
			}
		}


		if (runmode == 'R')
		{
			EnableMenuItem(GetMenu(hwnd), ID_CODEC_SPEEX, MF_GRAYED);
			EnableMenuItem(GetMenu(hwnd), ID_CODEC_LPC10, MF_GRAYED);
		}

		if (runmode != 'R')
		{
			try
			{
				DRMTransmitter.GetParameters()->bOnlyPicture = (runmode == 'P');
				DRMTransmitter.Init();
				DRMTransmitter.GetSoundInterface()->SetInDev(actdevint);
				DRMTransmitter.GetSoundInterface()->SetOutDev(actdevoutt);
				TransmParam = DRMTransmitter.GetParameters();
				TransmParam->Service[0].strLabel = getcall();
				TransmParam->Service[0].AudioParam.eAudioCoding = CParameter::AC_LPC;
				AudioSourceEncoder = DRMTransmitter.GetAudSrcEnc();
				AudioSourceEncoder->ClearTextMessage();
				AudioSourceEncoder->ClearPicFileNames();
				DRMTransmitter.SetCarOffset(350.0);
//				DRMTransmitter.GetAudSrcEnc()->SetStartDelay(starttx_time_long); //Edited DM
				DRMTransmitter.GetAudSrcEnc()->SetTheStartDelay(starttx_time_long); //Edited DM

				TX_Running = TRUE;
				_beginthread(TxFunction,0,NULL);
			}
			catch(CGenErr)
			{
				TX_Running = FALSE;
				MessageBox( hwnd,"Transmitter will NOT work\nWinDRM needs 2 soundcards or WinXP\nTry -r, -t or -p startup options","TX Init Exception",0);	
			}
		}

		if (runmode == 'P')
		{
			TransmParam->Service[0].AudioParam.eAudioCoding = CParameter::AC_SSTV;
		}

		UseTextMessage = gettext();
		if ((UseTextMessage) && (TX_Running))
		{
			FILE * txtset = NULL;
			char textbuf[200];
			txtset = fopen("textmessage.txt","rt");
			if (txtset != NULL)
			{
				junk = fscanf(txtset, "%[^\0]", &textbuf); //edited DM
				fclose(txtset);
				DRMTransmitter.GetAudSrcEnc()->ClearTextMessage();
				DRMTransmitter.GetAudSrcEnc()->SetTextMessage(textbuf);
				DRMTransmitter.GetParameters()->Service[0].AudioParam.bTextflag = TRUE;
			}
		}

		uncheckdisp(hwnd);
		if (disptype == 0)
			CheckMenuItem(GetMenu(hwnd), ID_DISPLAY_SPECTRUM, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 1)
			CheckMenuItem(GetMenu(hwnd), ID_DISPLAY_PSD, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 2)
			CheckMenuItem(GetMenu(hwnd), ID_DISPLAY_LEVEL, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 3)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_WATERFALL, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 4)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_TRANSFERFUNCTION, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 5)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_IMPULSERESPONSE, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 6)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_FACPHASE, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 7)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_MSCPHASE, MF_BYCOMMAND | MF_CHECKED) ;
		else if (disptype == 8)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_TEST, MF_BYCOMMAND | MF_CHECKED) ;


		for (i=0;i<250;i++)
			specbufarr[i] = 1.0;

		if (_chdir("Rx Files"))
		{
			if( _mkdir( "Rx Files" ) != 0 )
				MessageBox( hwnd,"Failed to create Rx Files Directory","ERROR",0);	
			else
				MessageBox( hwnd,"Rx Files Directory created","INFO",0);	
		}
		else
			_chdir("..");
//		if (_chdir("Corrupt"))
		if (_chdir(rxcorruptpath))
		
		{
			if( _mkdir(rxcorruptpath) != 0 )
				MessageBox( hwnd,"Failed to create Corrupt Directory","ERROR",0);	
		}
		else
			_chdir("..");

		if (rtsonfac)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_RTSHIGHONFAC, MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_RTSHIGHONFAC, MF_BYCOMMAND | MF_UNCHECKED);
		if (dtronfac)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_DTRHIGHONFAC, MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_DTRHIGHONFAC, MF_BYCOMMAND | MF_UNCHECKED);

		DRMTransmitter.GetParameters()->eSymbolInterlMode = getinterleave();
		DRMTransmitter.GetParameters()->eMSCCodingScheme = getqam();
		DRMTransmitter.GetParameters()->SetSpectrumOccup(getspec());
		DRMTransmitter.GetParameters()->InitCellMapTable(getmode(),getspec());

		SetTimer(hwnd,1,100,TimerProc); //Original is 100mS

		return TRUE;   
		
    case WM_COMMAND:
		OnCommand(hwnd, LOWORD (wParam), HIWORD (wParam));
        return TRUE;

    case WM_CLOSE:
 		comtx('0');
		KillTimer(hwnd,1);
		savevar();
        DestroyWindow (hwnd);
 		DRMReceiver.Stop(); 
		DRMTransmitter.Stop();
		Sleep(1000);
	    PostQuitMessage (0);
        return TRUE;
    case WM_PAINT:
		PaintWaveData(hwnd,TRUE);
        return FALSE;
    }
    return FALSE;
}

int snddev;
char cmdstr[20];

void OnCommand ( HWND hwnd, int ctrlid, int code)
{
    switch (ctrlid)
    {
    case IDB_STARTPIC:
		if (IsRX)
		{
			//if (callisok())  //disabled for SW Broadcasting use - DM
				DialogBox(TheInstance, MAKEINTRESOURCE (DLG_PICTURE_TX), hwnd, TXPictureDlgProc);
			//else
				//DialogBox(TheInstance, MAKEINTRESOURCE (DLG_CALLSIGN), hwnd, CallSignDlgProc); //disabled for SW Broadcasting use - DM
		}
		else
		{
			if (TX_Running) DRMTransmitter.NotSend();
			IsRX = TRUE;
			EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_ENABLED);
			EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_DRMSETTINGS, MF_ENABLED);
			if (RX_Running) DRMReceiver.Rec();
			SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"TX Voice");
			SendMessage (GetDlgItem (hwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"TX Pic");
			SetDlgItemText(hwnd, IDC_DCFREQ, " ");
			SetDlgItemText(hwnd, IDC_EDIT,   " ");
			SetDlgItemText(hwnd, IDC_EDIT2,  " ");
			SetDlgItemText(hwnd, IDC_EDIT4,  " ");
			SetDlgItemText(hwnd, IDC_EDIT3,  " ");
			SetDlgItemText(hwnd, IDC_EDIT6,  " "); //added DM
			if (strlen(rxdevice) >=1) SelectSrc(rxdevice);
			
			//Clear bargraph DM
			ClearBar(hwnd);

		}
        break;
    case IDB_START:
		if (IsRX)
		{
			char dcbuf[20];
			if (runmode != 'P')
			{
				//if (callisok()) 
				//{
					SetTXmode(FALSE);
					DRMReceiver.NotRec();
					IsRX = FALSE;
					EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_GRAYED);
					EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_DRMSETTINGS, MF_GRAYED);
					if (TX_Running) 
					{	
						DRMTransmitter.Init();
						if (DRMTransmitter.GetParameters()->iNumDecodedBitsMSC <= 980)
							MessageBox( hwnd,"Mode does not allow Voice","Wrong Mode",0);	
						DRMTransmitter.Send();
						sprintf(dcbuf,"%d",(int)DRMTransmitter.GetCarOffset());
						SetDlgItemText( hwnd, IDC_DCFREQ, dcbuf);
					}
					SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"RX");
					SendMessage (GetDlgItem (hwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"RX");
					if (strlen(txdevice) >=1) SelectSrc(txdevice);
				//}
				//else
				//{
				//	DialogBox(TheInstance, MAKEINTRESOURCE (DLG_CALLSIGN), hwnd, CallSignDlgProc);
				//}
			}
		}
		else
		{
			if (TX_Running) DRMTransmitter.NotSend();
			IsRX = TRUE;
			EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_ENABLED);
			EnableMenuItem(GetMenu(hwnd), ID_SETTINGS_DRMSETTINGS, MF_ENABLED);
			DRMReceiver.Rec();
			SendMessage (GetDlgItem (hwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"TX Voice");
			SendMessage (GetDlgItem (hwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"TX Pic");
			SetDlgItemText(hwnd, IDC_DCFREQ, " ");
			SetDlgItemText(hwnd, IDC_EDIT,   " ");
			SetDlgItemText(hwnd, IDC_EDIT2,  " ");
			SetDlgItemText(hwnd, IDC_EDIT4,  " ");
			SetDlgItemText(hwnd, IDC_EDIT3,  " ");
			SetDlgItemText(hwnd, IDC_EDIT6,  " "); //added DM
			if (strlen(rxdevice) >=1) SelectSrc(rxdevice);

			//Clear bargraph DM
			ClearBar(hwnd);

		}
        break;
 
	case ID_PTTPORT_NONE:
		settxport('0');	comtx(gettxport());	checkcomport(hwnd,'0');	break;
	case ID_PTTPORT_COM1:
		settxport('1');	comtx(gettxport());	checkcomport(hwnd,'1');	break;
	case ID_PTTPORT_COM2:
		settxport('2');	comtx(gettxport());	checkcomport(hwnd,'2');	break;
	case ID_PTTPORT_COM3:
		settxport('3');	comtx(gettxport());	checkcomport(hwnd,'3');	break;
	case ID_PTTPORT_COM4:
		settxport('4');	comtx(gettxport());	checkcomport(hwnd,'4');	break;
	case ID_PTTPORT_COM5:
		settxport('5');	comtx(gettxport());	checkcomport(hwnd,'5');	break;
	case ID_PTTPORT_COM6:
		settxport('6');	comtx(gettxport());	checkcomport(hwnd,'6');	break;
	case ID_PTTPORT_COM7:
		settxport('7');	comtx(gettxport());	checkcomport(hwnd,'7');	break;
	case ID_PTTPORT_COM8:
		settxport('8');	comtx(gettxport());	checkcomport(hwnd,'8');	break;
	case ID_SETTINGS_PTTPORT_RTSHIGHONFAC:
		if (rtsonfac)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_RTSHIGHONFAC, MF_BYCOMMAND | MF_UNCHECKED);
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_RTSHIGHONFAC, MF_BYCOMMAND | MF_CHECKED);
		rtsonfac = ! rtsonfac;
		break;
	case ID_SETTINGS_PTTPORT_DTRHIGHONFAC:
		if (dtronfac)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_DTRHIGHONFAC, MF_BYCOMMAND | MF_UNCHECKED);
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_PTTPORT_DTRHIGHONFAC, MF_BYCOMMAND | MF_CHECKED);
		dtronfac = ! dtronfac;
		break;

	/* All disabled to simplify selection - Moved to the bottom after the switch
	case IDM_O_RX_I_DRIVERS0:
		setsoundin(0,'r');
		unchecksoundrx(hwnd,0);
		break;
	case IDM_O_RX_I_DRIVERS1:
		setsoundin(1,'r');
		unchecksoundrx(hwnd,1);
		break;
	case IDM_O_RX_I_DRIVERS2:
		setsoundin(2,'r');
		unchecksoundrx(hwnd,2);
		break;
	case IDM_O_TX_O_DRIVERS0:
		setsoundout(0,'t');
		unchecksoundtx(hwnd,0);
		break;
	case IDM_O_TX_O_DRIVERS1:
		setsoundout(1,'t');
		unchecksoundtx(hwnd,1);
		break;
	case IDM_O_TX_O_DRIVERS2:
		setsoundout(2,'t');
		unchecksoundtx(hwnd,2);
		break;
	case IDM_O_VO_I_DRIVERS0:
		setsoundin(0,'t');
		unchecksoundvoI(hwnd,0);
		break;
	case IDM_O_VO_I_DRIVERS1:
		setsoundin(1,'t');
		unchecksoundvoI(hwnd,1);
		break;
	case IDM_O_VO_I_DRIVERS2:
		setsoundin(2,'t');
		unchecksoundvoI(hwnd,2);
		break;
	case IDM_O_VO_O_DRIVERS0:
		setsoundout(0,'r');
		unchecksoundvoO(hwnd,0);
		break;
	case IDM_O_VO_O_DRIVERS1:
		setsoundout(1,'r');
		unchecksoundvoO(hwnd,1);
		break;
	case IDM_O_VO_O_DRIVERS2:
		setsoundout(2,'r');
		unchecksoundvoO(hwnd,2);
		break;
		*/

	case ID_SOUNDCARD_OPENMIXER_RXINPUT:
		snddev = getsoundin('r');
		wsprintf(cmdstr,"-R -D %d",snddev);
		ShellExecute(NULL, "open", "sndvol.exe", cmdstr, NULL, SW_SHOWNORMAL); //updated for Win7 mixer DM
		break;
	case ID_SOUNDCARD_OPENMIXER_TXOUTPUT:
		snddev = getsoundout('t');
		wsprintf(cmdstr,"-D %d",snddev);
		ShellExecute(NULL, "open", "sndvol.exe", cmdstr, NULL, SW_SHOWNORMAL); //updated for Win7 mixer DM
		break;
	case ID_SOUNDCARD_OPENMIXER_VOICEINPUT:
		snddev = getsoundin('t');
		wsprintf(cmdstr,"-R -D %d",snddev);
		ShellExecute(NULL, "open", "sndvol.exe", cmdstr, NULL, SW_SHOWNORMAL); //updated for Win7 mixer DM
		break;
	case ID_SOUNDCARD_OPENMIXER_VOICEOUTPUT:
		snddev = getsoundout('r');
		wsprintf(cmdstr,"-D %d",snddev);
		ShellExecute(NULL, "open", "sndvol.exe", cmdstr, NULL, SW_SHOWNORMAL); //updated for Win7 mixer DM
		break;
	case ID_SOUNDCARD_AUTOMIXERSWITCH:
		DialogBox(TheInstance, MAKEINTRESOURCE (DLG_MIXERSETTING), hwnd, MixerDlgProc);
		break;
	case ID_ABOUT_TEST:
		DialogBox(TheInstance, MAKEINTRESOURCE (DLG_ABOUT), hwnd, AboutDlgProc);
		break;
	case ID_ABOUT_HELP:
		DialogBox(TheInstance, MAKEINTRESOURCE (DLG_HELP), hwnd, AboutDlgProc);
		break;

	case ID_SETTINGS_CALLSIGN:
		DialogBox(TheInstance, MAKEINTRESOURCE (DLG_CALLSIGN), hwnd, CallSignDlgProc);
		break;

	case ID_SETTINGS_DRMSETTINGS:
		DialogBox(TheInstance, MAKEINTRESOURCE (DLG_DRMSETTINGS), hwnd, DRMSettingsDlgProc);
		break;

	case ID_SETTINGS_DRMSETTINGSRX:
		CreateDialog(TheInstance, MAKEINTRESOURCE (DLG_RXSETTINGS), hwnd, DRMRXSettingsDlgProc);
		break;

	case ID_SETTINGS_TEXTMESSAGE:
		CreateDialog(TheInstance, MAKEINTRESOURCE (DLG_TEXTMESSAGE), hwnd, TextMessageDlgProc);
		break;

	case ID_SETTINGS_TEXTMESSAGE_OPENRXTEXTMESSAGE:
		if (AllowRXTextMessage)
		{
			AllowRXTextMessage = FALSE;
			if (RXTextMessageON) SendMessage(RXMessagehwnd,WM_CLOSE,0,0);
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_TEXTMESSAGE_OPENRXTEXTMESSAGE, MF_BYCOMMAND | MF_UNCHECKED);
		}
		else
		{
			AllowRXTextMessage = TRUE;
			RXTextMessageWasClosed = FALSE;
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_TEXTMESSAGE_OPENRXTEXTMESSAGE, MF_BYCOMMAND | MF_CHECKED);
		}
		break;
	case ID_SETTINGS_FILETRANSFER_SAVERECEIVEDFILES:
		if (SaveRXFile)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SAVERECEIVEDFILES, MF_BYCOMMAND | MF_UNCHECKED);	
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SAVERECEIVEDFILES, MF_BYCOMMAND | MF_CHECKED);	
		SaveRXFile = !SaveRXFile;
	case ID_SETTINGS_FILETRANSFER_SHOWRECEIVEDFILES:
		if (ShowRXFile)
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SHOWRECEIVEDFILES, MF_BYCOMMAND | MF_UNCHECKED);	
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SHOWRECEIVEDFILES, MF_BYCOMMAND | MF_CHECKED);	
		ShowRXFile = !ShowRXFile;
		break;
	case ID_SETTINGS_FILETRANSFER_SHOWONLYONE:
		if (ShowOnlyFirst)
		{
			wsprintf(lastfilename,"none");
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SHOWONLYONE, MF_BYCOMMAND | MF_UNCHECKED);
		}
		else
			CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_FILETRANSFER_SHOWONLYONE, MF_BYCOMMAND | MF_CHECKED);	
		ShowOnlyFirst = !ShowOnlyFirst;
		break;
	case ID_SETTINGS_FILETRANSFER_SENDFILE:
		DialogBox(TheInstance, MAKEINTRESOURCE (DLG_PICTURE_TX), hwnd, TXPictureDlgProc);
		break;

	case ID_DISPLAY_SPECTRUM:
		disptype = 0;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_DISPLAY_SPECTRUM, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_DISPLAY_PSD:
		disptype = 1;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_DISPLAY_PSD, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_DISPLAY_LEVEL:
		disptype = 2;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_DISPLAY_LEVEL, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_SETTINGS_DISPLAY_WATERFALL:
		disptype = 3;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_WATERFALL, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_SETTINGS_DISPLAY_TRANSFERFUNCTION:
		disptype = 4;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_TRANSFERFUNCTION, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_SETTINGS_DISPLAY_IMPULSERESPONSE:
		disptype = 5;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_IMPULSERESPONSE, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_SETTINGS_DISPLAY_FACPHASE:
		disptype = 6;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_FACPHASE, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_SETTINGS_DISPLAY_MSCPHASE:
		disptype = 7;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_MSCPHASE, MF_BYCOMMAND | MF_CHECKED) ;
		break;
	case ID_SETTINGS_DISPLAY_TEST:
		disptype = 8;
		uncheckdisp(hwnd);
		CheckMenuItem(GetMenu(hwnd), ID_SETTINGS_DISPLAY_TEST, MF_BYCOMMAND | MF_CHECKED) ;
		break;

	case ID_CODEC_SPEEX:
		TransmParam->Service[0].AudioParam.eAudioCoding = CParameter::AC_SPEEX;
		CheckMenuItem(GetMenu(hwnd), ID_CODEC_SPEEX, MF_BYCOMMAND | MF_CHECKED) ;
		CheckMenuItem(GetMenu(hwnd), ID_CODEC_LPC10, MF_BYCOMMAND | MF_UNCHECKED) ;
		break;
 	case ID_CODEC_LPC10:
		TransmParam->Service[0].AudioParam.eAudioCoding = CParameter::AC_LPC;
		CheckMenuItem(GetMenu(hwnd), ID_CODEC_LPC10, MF_BYCOMMAND | MF_CHECKED) ;
		CheckMenuItem(GetMenu(hwnd), ID_CODEC_SPEEX, MF_BYCOMMAND | MF_UNCHECKED) ;
		break;

 	case IDC_RESETACQ:
		if (IsRX) DRMReceiver.SetInStartMode();
		break;

	case IDC_RXFILES:
		ShellExecute(NULL, "open", rxfilepath, NULL, NULL, SW_SHOWDEFAULT); //just open the folder DM
		break;

	case IDC_GETPICANY:
		if (IsRX && RX_Running)
		{
			CMOTObject NewPic;
			int i;
			if (DRMReceiver.GetDataDecoder()->GetSlideShowPartPicture(NewPic))
			{
				char filename[260]; //was 130 DM
				int picsize;
				int filnamsize;
				FILE * set = NULL;
				picsize = NewPic.vecbRawData.Size();
				if (SaveRXFile)
				{
					wsprintf(filename, "%s%s", rxcorruptpath,NewPic.strName.c_str()); //edited DM ======================= CHECK THIS
//					wsprintf(filename, "Corrupt\\%s",NewPic.strName.c_str());
					set = fopen(filename,"wb"); //saved partial file is opened here for writing DM - *MANUAL ONLY* - make an automatic routine?
					if (set != NULL)
					{
						for (i=0;i<picsize;i++) putc(NewPic.vecbRawData[i],set); //write partial file data DM  *MANUAL ONLY*
						fclose(set); //close file DM
					}
					else
						MessageBox( hwnd,"Could not open File",filename,0);	

					filnamsize = strlen(filename);

					if (filnamsize >= 11)
					{
						if (checkfilenam(filename, filnamsize))
							//ShellExecute(NULL, "open", filename, NULL, NULL, SW_SHOWNORMAL); //Deactivated - Execution is a security risk DM
							ShellExecute(NULL, "open", rxcorruptpath, NULL, NULL, SW_SHOWDEFAULT); //Open the folder instead DM
						else
							MessageBox( hwnd,"Executable File Saved",filename,0);	
					}
					else
						MessageBox( hwnd,"No Filename","Info",0);	
				}
			}
		}
		break;
	case IDC_SENDBSR:
		//BSR button links here DM
		if (IsRX && RX_Running)
		{

		//this appears to be for sending a BSR request DM
		//Before sending a BSR request, the bsr.bin file must be saved... DM
			if (GetBSR(&numbsrsegments, namebsrfile) == TRUE) //this saves the actual bsr.bin file to disk - it also handles hash etc. and returns TRUE if there were segments missing DM
			{
				if (namebsrfile == "bsr.bin") break; //is this needed?
				//now open a dialog window with the info and choices..
//				txbsrposind = numbsrsegments; //try this..
//				PostMessage(txbsrhwnd[txbsrposind], WM_NOTIFY, 0, 0);
				txbsrhwnd[txbsrposind] = CreateDialog(TheInstance, MAKEINTRESOURCE(DLG_SENDBSR), hwnd, SendBSRDlgProc); //This opens the dialog window to send the request DM
			}

			/*
			//OLD BSR CODE
			char myhashval; //edited DM
			//This grabs the BSR filename from the receiver... DM
			if (DRMReceiver.GetDataDecoder()->GetSlideShowBSR(&numbsrsegments,&namebsrfile,&myhashval)) //Equals new GetBSR
			{
				if (namebsrfile == "bsr.bin") break; //if it was a bsr.bin send from the other station, break here DM
				txbsrposind = -1;	// Search for old position
				for (int el=0;el<NO_BSR_WIND;el++)
					if (txbsr_onscreen_arr[el] == TRUE)
					{
						if (myhashval == txhasharr[el])
						{
							txbsrposind = el;
							el = NO_BSR_WIND;
						}
					}
				if (txbsrposind >= 0)	//Old pos Found
					PostMessage(txbsrhwnd[txbsrposind], WM_NOTIFY, 0, 0);
				else
				{
					txbsrposind = 0;	// Find new position
					for (int el=0;el<NO_BSR_WIND;el++)
						if (!txbsr_onscreen_arr[el])
						{
							txbsrposind = el;
							el = NO_BSR_WIND;
						}
					txbsr_onscreen_arr[txbsrposind] = TRUE;
					txhasharr[txbsrposind] = myhashval;
					txbsrhwnd[txbsrposind] = CreateDialog(TheInstance, MAKEINTRESOURCE (DLG_SENDBSR), hwnd, SendBSRDlgProc);
				}
			}
			*/
		}
		break;
	case IDC_TUNINGTONE:
		if ((TX_Running) && (!isplaying) && (messtate[MS_FAC_CRC] != 0))
		{
			Playtype = 1;
			if (IsRX) _beginthread(PlaySound,0,NULL);
		}
		break;
	case IDC_IDTONE:
		if ((TX_Running) && (!isplaying) && (messtate[MS_FAC_CRC] != 0))
		{
			Playtype = 2;
			if (IsRX) _beginthread(PlaySound,0,NULL);
		}
		break;
	case IDC_GTONE:
		if ((TX_Running) && (!isplaying) && (messtate[MS_FAC_CRC] != 0))
		{
			Playtype = 3;
			if (IsRX) _beginthread(PlaySound,0,NULL);
		}
		break;
	case IDC_BTONE:
		if ((TX_Running) && (!isplaying) && (messtate[MS_FAC_CRC] != 0))
		{
			Playtype = 4;
			if (IsRX) _beginthread(PlaySound,0,NULL);
		}
		break;
	case ID_SETTINGS_LOADLASTRXFILE:
		if (strlen(lastrxfilename) >= 4)
		{
			int i;
			int hashval;
			int iFileNameSize;
			unsigned char	xorfnam = 0;
			unsigned char	addfnam = 0;
			TXpicpospt = 1;
			wsprintf(filetitle[0],"%s",lastrxfilename);
			wsprintf(pictfile[0],"Rx Files\\%s",lastrxfilename);
			file_size[0] = lastrxfilesize;
			iFileNameSize = strlen(filetitle[0]);
			if (iFileNameSize > 255)	iFileNameSize = MAX_PATHLEN; //was 80 DM (Windows max path length is 255 characters)
			for (i=0;i<iFileNameSize;i++)
			{
				xorfnam ^= filetitle[0][i];
				addfnam += filetitle[0][i];
				addfnam ^= (unsigned char)i;
			}
			hashval = 256*(int)addfnam + (int)xorfnam;
			if (hashval <= 2) hashval += iFileNameSize;
			storesentfilename(pictfile[0],filetitle[0],hashval);
			DRMTransmitter.GetParameters()->SetMSCProtLev(lastrxprotlev);
			DRMTransmitter.GetParameters()->SetSpectrumOccup(lastrxspecocc);
			DRMTransmitter.GetParameters()->eMSCCodingScheme = lastrxcodscheme;
			DRMTransmitter.GetParameters()->InitCellMapTable(lastrxrobmode,lastrxspecocc);
			DRMTransmitter.Init();
			DRMTransmitter.GetParameters()->Service[0].DataParam.iPacketLen = calcpacklen(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC); 
			if (IsRX)
			{
				DialogBox(TheInstance, MAKEINTRESOURCE (DLG_PICTURE_TX), hwnd, TXPictureDlgProc);
			}
		}
		break;
   }
   //if menu was clicked in the range of any of the sound driver segments, set that device on and the others in that segment off DM
      if (ctrlid >= IDM_O_RX_I_DRIVERS0 && ctrlid < (IDM_O_END_DRIVERS0)) {
	   if (ctrlid >= IDM_O_RX_I_DRIVERS0 && ctrlid < (IDM_O_RX_I_DRIVERS0+ numdevIn)) {
		   int ctrl = ctrlid - IDM_O_RX_I_DRIVERS0;
		   setsoundin(ctrl, 'r'); //compute sound device number from click index
		   unchecksoundrx(hwnd, ctrlid); //clear all other check marks, and add one where clicked
	   }
	   if (ctrlid >= IDM_O_TX_O_DRIVERS0 && ctrlid < (IDM_O_TX_O_DRIVERS0 + numdevOut)) {
		   int ctrl = ctrlid - IDM_O_TX_O_DRIVERS0;
		   setsoundout(ctrl, 't'); //compute sound device number from click index
		   unchecksoundtx(hwnd, ctrlid); //clear all other check marks, and add one where clicked
	   }
	   if (ctrlid >= IDM_O_VO_I_DRIVERS0 && ctrlid < (IDM_O_VO_I_DRIVERS0 + numdevIn)) {
		   int ctrl = ctrlid - IDM_O_VO_I_DRIVERS0;
		   setsoundin(ctrl, 't'); //compute sound device number from click index
		   unchecksoundvoI(hwnd, ctrlid); //clear all other check marks, and add one where clicked
	   }
	   if (ctrlid >= IDM_O_VO_O_DRIVERS0 && ctrlid < (IDM_O_VO_O_DRIVERS0 + numdevOut)) {
		   int ctrl = ctrlid - IDM_O_VO_O_DRIVERS0;
		   setsoundout(ctrl, 'r'); //compute sound device number from click index
		   unchecksoundvoO(hwnd, ctrlid); //clear all other check marks, and add one where clicked
	   }
   }
}

CVector<_REAL>		vecrData;
CVector<_REAL>		vecrScale;
BOOL firstnorx = TRUE;
int isspdisp = 0;
int stoptx = -1;	
float specagc = 1.0;
float levelsmooth = 1.0;

int oldseg = 0;

void CALLBACK TimerProc (HWND hwnd, UINT nMsg, UINT nIDEvent, DWORD dwTime)
{
	int i;
//	unsigned int i; //edited DM
//	int specarrlen; //edited DM
	char tempstr[100]; //was 20 DM
	char interl = ' ';
  int qam = 0;
  int prot = 0;
  float snr;

  try
  {

	isspdisp++;
	if (isspdisp >= 4) isspdisp = 0;

	if (stoptx == 0) OnCommand(messhwnd,IDB_START,0);
	if (stoptx >= 0) stoptx--;

	if (IsRX && RX_Running)
	{
		newdata++;
		level = (int)(170.0 * DRMReceiver.GetReceiver()->GetLevelMeter());
		//NEW spectral AGC routine Daz Man 2021
		//apply a sensible threshold to the level input
		level = max(level, 1.0); //what is the scaling???
		//smooth the level
		//if (level > levelsmooth * 2) (level = levelsmooth * 2); //slew limit attack
		levelsmooth = max(level, levelsmooth * 0.9); //integrator for decay

		//calculate the gain value
		specagc = (float)70.0 / levelsmooth;
		
//original routine
//		if (level * specagc >= 120)
//			specagc -= (float)1; //was 0.1 DM  = attack time
//		else if (level * specagc <= 80)
//			specagc += (float)0.1; //was 0.1 DM = recovery time
//		if (specagc >= 15.0) specagc = 15.0;

		if (disptype == 0)	//spectrum
		{
			DCFreq = (int)DRMReceiver.GetParameters()->GetDCFrequency();
			DRMReceiver.GetReceiver()->GetInputSpec(vecrData);
			if (vecrData.Size() >= 500)
			{
				for (i=0;i<250;i++)
					specbufarr[i] = (3.0 * specbufarr[i] + vecrData[2*i] + vecrData[2*i+1]) * 0.2;
				for (i=0;i<250;i++)
				{
					specarr[i] = (int)(specbufarr[i] * 40.0 * specagc); //
				}
			}
			if (isspdisp != 0) PaintWaveData(hwnd,TRUE);
		}
		if (disptype == 3)	//waterfall
		{
			DRMReceiver.GetReceiver()->GetInputSpec(vecrData);
			if (vecrData.Size() >= 500)
			{
				for (i=0;i<250;i++)
					specarr[i] = 25.0 * vecrData[i] * specagc; //
			}
			if (isspdisp != 0) PaintWaveData(hwnd,FALSE);
		}
		if (disptype == 8)	//Moving Waterfall
		{
			DCFreq = (int)DRMReceiver.GetParameters()->GetDCFrequency();
			DRMReceiver.GetReceiver()->GetInputSpec(vecrData);
			if (vecrData.Size() >= 500)
			{
				for (i=0;i<500;i++)
					specarr[i] = 40.0 * vecrData[i] * specagc; //
			}
			if (isspdisp != 0) PaintWaveData(hwnd,FALSE);
		}
	}
	if (isspdisp != 0) return;

	if (IsRX && RX_Running)
	{
		if (disptype == 1)	//shifted PSD
		{
			int tmp;
			DRMReceiver.GetOFDMDemod()->GetPowDenSpec(vecrData);
			if (vecrData.Size() >= 256)  // size = 512
				for (i = 0; i < 250; i++)
				{
					if (robmode == 'E') tmp = (int)(-3.0 * vecrData[i + 42]) + 75; //was 45 DM 
					else tmp = (int)(-3.0 * vecrData[i + 85]) + 75; //was 45 DM
					specarr[i] = tmp;
				}
		}
		if (disptype == 4)	//Transfer Funct.
		{
			int tmp;
			DRMReceiver.GetChanEst()->GetTransferFunction(vecrData, vecrScale);
			specarrlen = vecrData.Size();
			for (i = 0; i < specarrlen; i++)
			{
				tmp = (int)(-0.5 * vecrData[i]) + 45;
				specarr[i] = tmp;
				tmp = (int)(-0.5 * vecrScale[i]) + 85;
				specarrim[i] = tmp;
			}
		}
		if (disptype == 5)	//Impulse Response
		{
			int tmp;
			_REAL				rLowerBound, rHigherBound;
			_REAL				rStartGuard, rEndGuard;
			_REAL				rPDSBegin, rPDSEnd;
			DRMReceiver.GetChanEst()->
				GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
					rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
			specarrlen = vecrData.Size();
			for (i = 0; i < specarrlen; i++)
			{
				tmp = (int)(-1.5 * vecrData[i]) + 100;
				specarr[i] = tmp;
			}
		}
		if (disptype == 6)	//FAC constellation
		{
			int tmp;
			CVector<_COMPLEX>	veccData;
			DRMReceiver.GetFACMLC()->GetVectorSpace(veccData);
			specarrlen = veccData.Size();
			for (i = 0; i < specarrlen; i++)
			{
				tmp = (int)(50.0 * veccData[i].real());
				specarr[i] = tmp;
				tmp = (int)(50.0 * veccData[i].imag());
				specarrim[i] = tmp;
			}
		}
		if (disptype == 7)	//MSC constellation
		{
			int tmp;
			CVector<_COMPLEX>	veccData;
			DRMReceiver.GetMSCMLC()->GetVectorSpace(veccData);
			specarrlen = veccData.Size();
			if (specarrlen >= 530) specarrlen = 530;
			for (i = 0; i < specarrlen; i++)
			{
				tmp = (int)(50.0 * veccData[i].real());
				specarr[i] = tmp;
				tmp = (int)(50.0 * veccData[i].imag());
				specarrim[i] = tmp;
			}
		}

		//======================================================================================================================================================
		//SEGMENT BARGRAPH by Daz Man 2021
		//======================================================================================================================================================
		RECT rect;
		int width = 218;
		if (GetWindowRect(hwnd, &rect))
		{
			width = (rect.right - rect.left)-1;
			//int height = rect.bottom - rect.top;
		}
		//read erasures buffer and convert it to a line graph
		int x = DRMReceiver.GetDataDecoder()->GetActPos(); //get current segment number
		int y = DRMReceiver.GetDataDecoder()->GetTotSize(); //Total segment count DM
		
		//if the GetTotSize function fails (it does sometimes - reason unknown), we can use our own computation if in RS mode
		if ((DecSegSize > 0) && (HdrFileSize > 0)){
			y = ceil((float)HdrFileSize / DecSegSize); //segment total = filesize/segsize
		}

		y = max(y, x);

		if (y == 0) { y = 1; } //don't divide by zero!

		//int n = width / max(totsize, DecTotalSegs);
//		int n = float(width << 12) / (y << 12);
		int n = (width * 1000) /y;
		//this executes every 100mS, so make sure it doesn't run too often
		if ((BarLastSeg != x) && (CRCOK)) {
			int d = 0;

			if (n > 1000) { n = 1000; } //Don't stretch bargraph

			HDC hdc = GetDC(hwnd);
			MoveToEx(hdc, BARL, BARY, NULL);
			/*
			XFORM xForm;
			xForm.eM11 = (FLOAT)2.0;
			xForm.eM12 = (FLOAT)0.0;
			xForm.eM21 = (FLOAT)0.0;
			xForm.eM22 = (FLOAT)2.0;
			xForm.eDx = 0;
			xForm.eDy = 0;
			SetWorldTransform(hdc, &xForm);
			*/
			//HPEN penz;
			HPEN penx;
			HPEN penb;
			HPEN penr;
			HPEN peng;
			LOGBRUSH lbb;
			lbb.lbStyle = BS_SOLID;
			lbb.lbColor = RGB(0,0,0);
			lbb.lbHatch = 0;
			LOGBRUSH lbr;
			lbr.lbStyle = BS_SOLID;
			lbr.lbColor = RGB(255, 0, 0);
			lbr.lbHatch = 0;
			LOGBRUSH lbg;
			lbg.lbStyle = BS_SOLID;
			lbg.lbColor = RGB(60, 255, 0);
			lbg.lbHatch = 0;
			LOGBRUSH lbx;
			lbx.lbStyle = BS_SOLID;
			lbx.lbColor = GetSysColor(COLOR_3DFACE); //RGB(127, 127, 127);
			lbx.lbHatch = 0;

			const int pwidth = 1;
			penb = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, pwidth, &lbb, 0, NULL); //Black for current segment
			penr = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 1, &lbr, 0, NULL); //Red for no segment received
			peng = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, pwidth, &lbg, 0, NULL); //Green for good segment
			penx = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 6, &lbx, 0, NULL); //Grey background erase
			//penz = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 1, &lbx, 0, NULL); //Grey background erase

			//Erase the last black line - for the 2nd/3rd pass etc.
			if (x < BarLastSeg) {
				SelectObject(hdc, peng); //use green - because if there was a black line drawn, there must have been a good segment
				MoveToEx(hdc, ((BarLastSeg* n) / 1000), BARB, NULL);
				LineTo(hdc, ((BarLastSeg* n) / 1000), BART); //erase
			}

			//data is good and we have a valid new segment - so update the graph
			//copy from buffer and draw to the screen
			//only read as far into the buffer as we need to...
			for (i = 0; i < y; i++) {
				//for each bit read, set the display red on 0 and green on 1
				d = (erasures[erasureswitch][i >> 3] >> (i & 7)) & 1; //get the correct bit
				//if (d == 0) { d = 0xFF0000; } //make 0 = 0xFF0000 = 16711680 red
				//if (d == 1) { d = 0x00FF00; } //make 1 = 0x00FF00 = 65280    green
		
				if (d == 0) {
					SelectObject(hdc, penr); //select red pen
					MoveToEx(hdc, (i * n) / 1000, BARB, NULL); //
					LineTo(hdc, (i * n) / 1000, BART); //draw a red line up
				}
				if (d == 1) {
					SelectObject(hdc, peng);//select green pen
					MoveToEx(hdc, (i * n) / 1000, BARB, NULL); //
					LineTo(hdc, (i * n) / 1000, BART); //draw a green line up
				}
			}

//			MoveToEx(hdc, floor((x+1)*n), BARY, NULL);
//			LineTo(hdc, floor((x + 3) * n), BARY); //draw a black tip on the line

			SelectObject(hdc, penb);
			MoveToEx(hdc, ((x * n) / 1000), BARB, NULL);
			LineTo(hdc, ((x * n) / 1000), BART); //draw a black tip on the line

			//did the transport ID change? (new file) - check if the Total segment count has changed also, and redraw the window background where needed
			if ((BarTransportID != DecTransportID) || (y != BarLastTot) || (x > BarLastTot)) {
				SelectObject(hdc, penx); //penx is the window background colour, and 6 pixels square
				MoveToEx(hdc, ((x * n) / 1000)+5, BARY, NULL);
				LineTo(hdc, BARR, BARY); //erase the rest of the window
				BarTransportID = DecTransportID;
				BarLastTot = y; //update totsegs
			}

			DeleteObject(penr);
			DeleteObject(peng);
			DeleteObject(penb);
			DeleteObject(penx);
			//DeleteObject(penz);

			BarLastSeg = x;

			ReleaseDC(hwnd, hdc);
		}

		//are there any blocks that need cleaning?
		if (erasureflags > 0) {
			//There are currently three blocks total (0,1,2)
			//don't erase the current block
			//don't erase the previous block
			//erase the next block to be used
			//find the next block number
			int a = erasureswitch + 1;
			if (a > 2) { a = 0; }
			int b = erasureflags && 1 << a; //mask it
			if (b > 0) {
			//block is used - erase it
			for (int d = 0; d < 1024; d++) {
			erasures[a][d] = 0; //clear mem before using it again
			}
			erasureflags = erasureflags & ~b; //clear the flag
			}
		}
		//======================================================================================================================================================

		if (DRMReceiver.GetParameters()->Service[0].IsActive())	//flags, etc.
		{
			CParameter::EAudCod ecodec;
			firstnorx = TRUE;
			rxcall = DRMReceiver.GetParameters()->Service[0].strLabel; //Get callsign from the incoming data DM
			if (rxcall == lastrxcall) consrxcall = rxcall;				//?
			else if (rxcall.length() >= 3) lastrxcall = rxcall;			//?
			SendMessage(GetDlgItem(hwnd, IDC_EDIT), WM_SETTEXT, 0, (LPARAM)rxcall.c_str());
			snr = (float)DRMReceiver.GetChanEst()->GetSNREstdB();	//Get SNR DM
			sprintf(tempstr, "%.1f", snr);							//Format for display DM
			SendMessage(GetDlgItem(hwnd, IDC_EDIT4), WM_SETTEXT, 0, (LPARAM)tempstr); //Display SNR DM
			snr = DRMReceiver.GetParameters()->GetDCFrequency();
			sprintf(tempstr, "%d", (int)snr);
			SendMessage(GetDlgItem(hwnd, IDC_DCFREQ), WM_SETTEXT, 0, (LPARAM)tempstr);

			if (DRMReceiver.GetParameters()->Service[0].eAudDataFlag == CParameter::SF_AUDIO)
			{
				sprintf(tempstr, "%d ", DRMReceiver.GetAudSrcDec()->getdecodperc()); //may need updating DM ======================== This is for speech mode DM
				SendMessage(GetDlgItem(hwnd, IDC_EDIT5), WM_SETTEXT, 0, (LPARAM)tempstr);

				wsprintf(tempstr, "MSCbits = %d  AudioBits = %d", DRMReceiver.GetParameters()->iNumDecodedBitsMSC, DRMReceiver.GetParameters()->iNumAudioDecoderBits);
				SendMessage(GetDlgItem(hwnd, IDC_EDIT6), WM_SETTEXT, 0, (LPARAM)tempstr); //Added receive blocksize DM ================

				ecodec = DRMReceiver.GetParameters()->Service[0].AudioParam.eAudioCoding;
				if (ecodec == CParameter::AC_LPC)
					SendMessage(GetDlgItem(hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)"LPC");
				else if (ecodec == CParameter::AC_SPEEX)
					SendMessage(GetDlgItem(hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)"SPEEX");
				else if (ecodec == CParameter::AC_SSTV)
					SendMessage(GetDlgItem(hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)"SSTV");
				else
					SendMessage(GetDlgItem(hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)" ");
			}
			else
			{
				CMOTObject NewPic; //Generate "NewPic" from the cpp class - figure out exactly how this works! DM
				lasterror = 0;

				//This is data mode (file receive) DM
				if (RxRSlevel == 0) {
					sprintf(tempstr, "Data"); //
				}
				else
					sprintf(tempstr, "RS%d Data", RxRSlevel); //
				SendMessage(GetDlgItem(hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)tempstr);

				//totsize = DecTotalSegs; //From serial backup DM
				//totsize = CompTotalSegs; //Computed from header DM
				totsize = DRMReceiver.GetDataDecoder()->GetTotSize(); //Total segment count DM
				totsize = max(totsize, CompTotalSegs);//Computed from header DM
				totsize = max(totsize, DecTotalSegs); //From serial backup DM

				int actsize = DRMReceiver.GetDataDecoder()->GetActSize(); //Current number of good segments DM
				int actpos = DRMReceiver.GetDataDecoder()->GetActPos();   //Current incoming segment DM

				sprintf(tempstr, "%d / %d / %d", totsize, actsize, actpos); //This is where the receiver stats are displayed DM
				SendMessage(GetDlgItem(hwnd, IDC_EDIT5), WM_SETTEXT, 0, (LPARAM)tempstr);

				//NewPic.strName.c_str(); MOTPicture
				//string tempfilename = MOTPicture; // NewPic.strName.c_str();
//				strcpy(DMfilename2, "unknown"); //set filename as unknown as a default
//				strcpy(DMfilename, "unknown"); //set filename as unknown as a default
//				DRMReceiver.GetDataDecoder()->GetSlideShowPartPicture(NewPic);
				//GetName(NewPic);
//					if (NewPic.strName.length() > 0) {
//						strcpy(DMfilename2, NewPic.strName.c_str());
//						wsprintf(tempstr, "MSCbits: %d  DDbits: %d  File: %s", DRMReceiver.GetParameters()->iNumDecodedBitsMSC, DRMReceiver.GetParameters()->iNumDataDecoderBits, DMfilename);
//						SendMessage(GetDlgItem(hwnd, IDC_EDIT6), WM_SETTEXT, 0, (LPARAM)tempstr); //Added receive blocksize DM ================
//					}
				/*
				//do we have filename data?
				if (DRMReceiver.GetDataDecoder()->GetSlideShowPartPicture(NewPic)) {
					if (NewPic.strName.c_str()) {
						strcpy(DMfilename, NewPic.strName.c_str());//If there is a valid filename from the old header, copy it DM
					}
				}
				*/

				RSfilesize = DecFileSize; //This is exactly what was sent, after RS coding into multiples of 255

					//=========================================================================================================================================================
					//RS Decoding - By Daz Man 2021
					//If there have been enough data packets received, decode the file even if we missed the header DM
					//How do we make sure this only runs once per file? Use the transport ID, also check if it worked or made an error
					if (RSretryDelay > 0) { RSretryDelay -= 1; }

					if (((RxRSlevel > 0) && (DecTotalSegs > 0) && (actsize >= (DecTotalSegs * 0.51)))) {

						if ((RSlastTransportID != DecTransportID) && (RSretryDelay == 0)) {
							RSretryDelay = 5; //wait before another attempt
							//int aa = DecTransportID;
							const uLongf BUFSIZE = 524288; //512k should be enough for anything practical - HEAP STORAGE
							_BYTE* buffer1 = new _BYTE[BUFSIZE * 2]; //Now 1M to fit RS encoding
							_BYTE* buffer2 = new _BYTE[BUFSIZE * 2]; //
							_BYTE* buffer3 = new _BYTE[BUFSIZE * 2]; //erasures processing buffer

							//the exact file size is now sent serially on each segment, so use it!


							if (DecFileSize > 0) {
								//Read raw data into buffer2
								i = 0;


								for (i = 0; i < DecFileSize;i++) {
									buffer2[i] = (_BYTE) GlobalDMRxRSData[i];
//									buffer2[i] = NewPic.Body.DMvecBYTEData[i]; //MOTObjectRaw.BodyRx.DMvecBYTEData[i];
//									buffer2[i] = NewPic.vecbRawData[i]; //save data to buffer2
								}
							}
							else {
								lasterror |= 8;
							}

							//Ideally, save the data into the buffer and start the RS decode attempts in a new thread?
							//Use flags to communicate with this thread?
							//Decoding is pretty fast now, so may not be needed.... But it would avoid stalling the UI

							//Data deinterleaver
							bool rev = 1; //Reverse mode to deinterleave
							distribute(buffer2, buffer1, RSfilesize, rev); //output is put into buffer1

							//Erasure processing should be added here...
							//Unpack the packed erasure bit array and deinterleave it so it matches the deinterleaved data locations
							//Probably could write it into buffer3, and deinterleave it into buffer2
							//buffer2 should overwrite at a slower rate than the erasure data is needed...

							//read erasure data and decode
							i = 0;
							int s = 0;
							while (i < RSfilesize) {
								//check the tID matches and change erasureswitch value if needed... TODO
								//erasures are in SEGMENTS
								//buffer is in BYTES
								//there are N BYTES per SEGMENT
								//where N is the segment size
								//therefore:
								//s = i / segment size
								s = i/erasuressegsize[erasureswitch];
								buffer3[i] = (erasures[erasureswitch][s >> 3] >> (s & 7)) & 1; //get the correct packed bit from each byte
								i++;
							}
							//Deinterleave the erasure data
							distribute(buffer3, buffer2, RSfilesize, rev); //erasures output is put into buffer2 - it can be read before it's overwritten by the RS decoder

							//Erasures now need to be changed into list form....
							//Scan the buffer up to the data size, and make a list of all the zero positions
							//Maybe best to do this in the RS4 decoder... DONE

							//RS decode here
							//lasterror = 0;
							lasterror |= rs4decodeE(buffer1, buffer2, buffer2, RSfilesize);
							RSfilesize = ceil(((float)RSfilesize * 128) / 255); //compute new file size for RS4 output (exact)

							//data needs to be in buffer2 
							 
							//copy buffer2 into buffer1 DEBUGGING ONLY ============================================================================
//							for (int i = 0; i < RSfilesize; i++) {
//								buffer2[i] = buffer1[i];
//							}

							int filesizetest = 0;
							int j = 0;
							//Error count - if RS is working correctly, this should be < 1...
							if (lasterror < 1) {
								//if the RS decode worked....
								//we have good data now, so decode the filename, filesize and headersize from the special header
								//We should match the ID string to be sure we have a valid header...
								int c = 0;
								for (j = 0; j < 15; j++) {
									//all 15 header characters must match
									//this is normally buffer2
									c = (buffer2[j] != EZHeaderID[j]) + c; //logically OR all tests
								}
								//if the header ID matched...
								if (c == 0) {

									//Read the header
									//get the total header length
									//normally buffer2
									j = (buffer2[16] | buffer2[17] << 8); //16 = low byte, 17 = high byte Header Size
									filesizetest = (buffer2[18] | buffer2[19] << 8 | buffer2[20] << 16); //This is the size of the file + header, as sent (plain or gzipped)
									//Save the filename
									char filenametest[260]; //temp
									int k = 0;
									int i = 21; //Filename starts at position 21
									while (i < j) {
										filenametest[k++] = buffer2[i++];
									}
									filenametest[k] = 0; //terminate filename string with a 0

									//copy the new header filename to the old one
									if (size(filenametest) > 0) {
										strcpy(DMfilename, filenametest); //copy
									}

									//to save the file we better have a filename....
									if (size(filenametest) > 0) {

										//First we need a filename and a file size!
										char RSfilenameS[260] = "";
										wsprintf(RSfilenameS, "Rx Files\\%s", filenametest);

										//If file is bigger than 512k, bypass decompression and save it directly DM
										//.gz is used for standard mode to be compatible - RS modes were to use .gzz to prevent unzipping .gz files.. NOT IMPLEMENTED YET - DM
										if ((stricmp(&RSfilenameS[strlen(RSfilenameS) - 3], ".gz") == 0) && (filesizetest <= BUFSIZE)) {
											//Data is gzipped, unzip into buffer1 and save
											//data is compressed, so decompress it
											i = 0;

											uLongf filesize = BUFSIZE; //to tell zlib how big the buffer is
											int result; //not used at this time
											result = uncompress(buffer1, &filesize, buffer2+j, filesizetest); //decompress data using zlib
											//cut extension off filename
											char trimmed[260]; //was 130 initially
											strcpy(trimmed, RSfilenameS);
											unsigned int tlen = strlen(trimmed);
											trimmed[tlen - 3] = 0; //terminate the string early to cut off the extra .gz extension DM

											//saved file is opened here for writing DM
											FILE* set = NULL;
											if ((set = fopen(trimmed, "wb")) == NULL) {
												// handle error here DM
												lasterror |= 16;
											}
											else {
												i = 0; //start at zero
												//this is normally buffer1
												while (i < filesize) { //edit DM
													putc(buffer1[i], set);
													i++;
												}
												fclose(set); //file is closed here - but only if it was opened
											}

										}
										else {
											//If data is noncompressed, save buffer2 to disk
											//saved file is opened here for writing DM
											FILE* set = NULL;
											if ((set = fopen(RSfilenameS, "wb")) == NULL) {
												// handle error here DM
												lasterror |= 16;
											}
											else {
												i = j; //start at the end of the header
												//this is normally buffer2
												while (i < filesizetest) { //edit DM
													putc(buffer2[i], set);
													i++;
												}
												fclose(set); //file is closed here - but only if it was opened
											}
										}
									}
									else {
										lasterror |= 32;
									}
								}
								else {
									lasterror |= 64 | c; //new header fail
								}
								RSlastTransportID = DecTransportID; //save last Transport ID so we don't decode it again
							}
							else {
								lasterror |= 128;
							}

							lasterror |= 1024;

							//remove the buffer arrays from the heap DM
							delete[] buffer1;
							delete[] buffer2;
							delete[] buffer3;
						}
					}

				//=========================================================================================================================================================
				//This is for non-RS received files that have ALL segments decoded OK - DM
				if (DRMReceiver.GetDataDecoder()->GetSlideShowPicture(NewPic))
				{
					//do not execute this on RS coded files DM
					if (RxRSlevel == 0) {
						//RxRSlevel is sent using 3 of the previously unused bits in the segment header
						//This code section does not use the extra RS coding, so RxRSlevel must == 0
						// 
						//This executes when a complete file is received DM
						char filename[260]; //was 130 DM - (Windows max path length is 255 characters) (Ham-DRM only uses 80 characters though...)
						int picsize; //edited DM - reverted
						int filnamsize;
						FILE* set = NULL;
						picsize = NewPic.vecbRawData.Size();

						//check if the file is a BSR (request?) file from the other station DM
						//This appears to be for INCOMING BSR files DM
						if (strcmp(NewPic.strName.c_str(), "bsr.bin") == 0)
						{
							set = fopen("bsrreq.bin", "wt"); //save incoming BSR requests under this name DM =====================================
							if (set != NULL)
							{
								for (i = 0; i < picsize; i++) putc(NewPic.vecbRawData[i], set); //write incoming BSR request to the file DM
								fclose(set);

								//Decompress incoming BSR request file here DM
								char filenam[300];
								char filenam2[300];
								wsprintf(filenam, "%s%s", bsrpath, "bsrreq.bin");
								wsprintf(filenam2, "%s%s", bsrpath, "cbsrreq.bin");
								CopyFile(filenam, filenam2, FALSE);
								decompressBSR(filenam2, filenam); //decompress renamed to decompressBSR to prevent conflict with zlib DM
								remove(filenam2); //delete temp file DM

								filetosend = readbsrfile(bsrpath);
							}
							else
								filetosend = 'x';

							if (filetosend.size() >= 3)
							{
								//acthash = gethash(); //removed DM
								//the hash used was the iTID (transmit ID) DM
								acthash = 0; // gethash(); //removed DM

								bsrposind = -1;	// Search for old position
								for (int el = 0; el < NO_BSR_WIND; el++)
								{
									if (bsr_onscreen_arr[el] == TRUE)
									{
										if (acthash == hasharr[el])  //This is now set to zero DM - This should be OK, as it appears to just be a unique number for the BSR window DM
										{
											if (bsrcall[el] == consrxcall)
											{
												bsrposind = el;
												el = NO_BSR_WIND;
											}
										}
									}
								}
								if (bsrposind >= 0)	//Old pos Found
								{
									PostMessage(bsrhwnd[bsrposind], WM_NOTIFY, 0, 0);
								}
								else				//Find new pos
								{
									bsrposind = 0;	// Find new position
									for (int el = 0; el < NO_BSR_WIND; el++)
									{
										if (bsr_onscreen_arr[el] == FALSE)
										{
											bsrposind = el;
											el = NO_BSR_WIND;
										}
									}
									bsr_onscreen_arr[bsrposind] = TRUE;
									hasharr[bsrposind] = acthash;
									bsrcall[bsrposind] = consrxcall;
									bsrhwnd[bsrposind] =
										CreateDialog(TheInstance, MAKEINTRESOURCE(DLG_ANSWERBSR), hwnd, AnswerBSRDlgProc);
								}
							}
						}
						else if (SaveRXFile)
						{
							int iFileNameSize; //edited DM - reverted
							int tmphashval;
							unsigned char	xorfnam = 0;
							unsigned char	addfnam = 0;
							wsprintf(filename, "%s", NewPic.strName.c_str());
							iFileNameSize = strlen(filename);
							if (iFileNameSize > 255)	iFileNameSize = MAX_PATHLEN;  //was 80 now 260 DM - (Windows max path length is 255 characters)
							for (i = 0; i < iFileNameSize; i++)
							{
								xorfnam ^= filename[i];
								addfnam += filename[i];
								addfnam ^= (unsigned char)i;
							}
							tmphashval = 256 * (int)addfnam + (int)xorfnam;
							if (tmphashval <= 2) tmphashval += iFileNameSize;

							for (int el = 0; el < NO_BSR_WIND; el++)
							{
								if (tmphashval == txhasharr[el])
								{
									if (txbsr_onscreen_arr[el] == TRUE)
									{
										bCloseMe = TRUE;
										SendMessage(txbsrhwnd[el], WM_NOTIFY, 0, 0);
									}
								}
							}

							wsprintf(lastrxfilename, "%s", NewPic.strName.c_str());
							lastrxfilesize = picsize;
							lastrxrobmode = DRMReceiver.GetParameters()->GetWaveMode();
							lastrxspecocc = DRMReceiver.GetParameters()->GetSpectrumOccup();
							lastrxcodscheme = DRMReceiver.GetParameters()->eMSCCodingScheme;
							lastrxprotlev = DRMReceiver.GetParameters()->MSCPrLe.iPartB;

							wsprintf(filename, "Rx Files\\%s", NewPic.strName.c_str());
							//===========================================================================================
							//Daz Man
							//zlib decompression
							//test the file by checking for the extra filename extension to denote it's been compressed
							//if it is present, write into buffer1 and expand the file using zlib and save into buffer2 for writing to file
							//else, write file directly to disk
							//finally close file and delete buffers from heap
							//===========================================================================================
							int j = 0;
							const uLongf BUFSIZE = 524288; //512k should be enough for anything practical - HEAP STORAGE
							//If file is bigger than 512k, bypass decompression and save it directly DM
							//.gz is used for standard mode to be compatible
							if ((stricmp(&filename[strlen(filename) - 3], ".gz") == 0) && (picsize <= BUFSIZE)) {
								//data is compressed, so decompress it
								_BYTE* buffer1 = new _BYTE[BUFSIZE]; //File read buffer
								_BYTE* buffer2 = new _BYTE[BUFSIZE]; //zlib decompression buffer
								i = 0;
								while (i < picsize) {
									buffer1[i++] = NewPic.vecbRawData[j++]; //save data to buffer1 - j should be i..?
								}
								uLongf filesize = BUFSIZE; //to tell zlib how big the buffer is
								int result; //not used at this time
								result = uncompress(buffer2, &filesize, buffer1, picsize); //decompress data using zlib
	//								result = uncompress(buffer2, &filesize, NewPic.vecbRawData, picsize); //see if there is a way to directly read the buffer TODO
									//cut extension off filename
								char trimmed[260]; //was 130 initially
								strcpy(trimmed, filename);
								unsigned int tlen = strlen(trimmed);
								trimmed[tlen - 3] = 0; //terminate the string early to cut off the extra .gz extension DM

								//saved file is opened here for writing DM
								if ((set = fopen(trimmed, "wb")) == NULL) {
									// handle error here DM
								}
								else {
									i = 0;
									while (unsigned(i) < filesize) { //edit DM
										putc(buffer2[i], set);
										i++;
									}
									fclose(set); //file is closed here - but only if it was opened
								}
								delete[] buffer1; //remove the buffer arrays from the heap
								delete[] buffer2;
							}
							else {
								//data is not compressed, save normally - no need for extra buffers
								//Also - if incoming file is bigger than 512k (!) don't decompress it because it will overflow the buffers (can only happen if a *.gz file is sent, so save it normally)
								//saved file is opened here for writing DM
								if ((set = fopen(filename, "wb")) == NULL) {
									//handle error here DM
								}
								else {
									//no error
									//file data is written here DM
									for (i = j; i < picsize; i++) {
										putc(NewPic.vecbRawData[i], set);
									}
									fclose(set); //file is closed here							
								}
							}

							//for (i=0;i<picsize;i++) putc(NewPic.vecbRawData[i],set); //file data was written here DM - DISABLED NOW
	//						}
							//Consider deleting the auto-open function, as it could be a security risk DM - Deactivated
							if (ShowRXFile)
							{
								filnamsize = strlen(filename);
								if (filnamsize >= 11)
								{
									if (checkfilenam(filename, filnamsize))
									{
										//if (stricmp(lastfilename, filename) != 0)
											//ShellExecute(NULL, "open", filename, NULL, NULL, SW_SHOWNORMAL); //Deactivated - Auto-opening files is a security risk DM
										ShellExecute(NULL, "open", rxfilepath, NULL, NULL, SW_SHOWDEFAULT); //Open the folder instead DM
									}
									else
									{
										if (stricmp(lastfilename, filename) != 0)
											MessageBox(hwnd, filename, "Executable File Saved", 0);
									}
								}
								if (ShowOnlyFirst) wsprintf(lastfilename, "%s", filename);
							}
						}
					}
				}
				sendinfo(hwnd); //send stats to window
			}

			if (DRMReceiver.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_A)
				robmode = 'A';
			if (DRMReceiver.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_B)
				robmode = 'B';
			if (DRMReceiver.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_E)
				robmode = 'E';
			if ((DRMReceiver.GetParameters()->GetSpectrumOccup()) == SO_0)
				specocc = 3;
			else if ((DRMReceiver.GetParameters()->GetSpectrumOccup()) == SO_1)
				specocc = 5;
			if (DRMReceiver.GetParameters()->eSymbolInterlMode == CParameter::SI_LONG)
				interl = 'L';
			else if (DRMReceiver.GetParameters()->eSymbolInterlMode == CParameter::SI_SHORT)
				interl = 'S';
			if (DRMReceiver.GetParameters()->eMSCCodingScheme == CParameter::CS_2_SM)
				qam = 16;
			else if (DRMReceiver.GetParameters()->eMSCCodingScheme == CParameter::CS_3_SM)
				qam = 64;
			else 
				qam = 4;
			prot = DRMReceiver.GetParameters()->MSCPrLe.iPartB;
			wsprintf(tempstr,"%c/%c/%d/%d/2.%d",robmode,interl,qam,prot,specocc);	
			SendMessage (GetDlgItem (hwnd, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)tempstr);	
			if ((DRMReceiver.GetParameters()->Service[0].eAudDataFlag == CParameter::SF_AUDIO) && (DRMReceiver.GetParameters()->Service[0].AudioParam.bTextflag == TRUE))
			{
				if ((AllowRXTextMessage) && (runmode != 'P'))
				{
					if ((!RXTextMessageON) && (!RXTextMessageWasClosed))
						RXMessagehwnd = CreateDialog (TheInstance, 
							MAKEINTRESOURCE (DLG_RXTEXTMESSAGE), hwnd, RXTextMessageDlgProc);
					else if (RXTextMessageON)
						SendMessage(RXMessagehwnd,WM_NOTIFY,0,0);
				}
			}
		}
		else if (firstnorx)
		{
			//if (RXTextMessageON) SendMessage(RXMessagehwnd,WM_CLOSE,0,0);
			wsprintf(lastfilename,"%s","none");
			SendMessage (GetDlgItem (hwnd, IDC_EDIT), WM_SETTEXT, 0, (LPARAM)"No Service");
			SendMessage (GetDlgItem (hwnd, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)" ");
			SendMessage (GetDlgItem (hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)" ");
			SendMessage (GetDlgItem (hwnd, IDC_EDIT4), WM_SETTEXT, 0, (LPARAM)"0");
			SendMessage (GetDlgItem (hwnd, IDC_EDIT5), WM_SETTEXT, 0, (LPARAM)" ");
			SendMessage (GetDlgItem (hwnd, IDC_DCFREQ), WM_SETTEXT, 0, (LPARAM)" ");
			SendMessage(GetDlgItem(hwnd, IDC_EDIT6), WM_SETTEXT, 0, (LPARAM)" "); //added DM
			//DMpercent = 0; //added DM
			// 
			//Clear bargraph DM
			ClearBar(hwnd);

			firstnorx = FALSE;
		}
		else
		{
			RXTextMessageWasClosed = FALSE;
		}
	}
	else if (!IsRX && TX_Running)
	{
		int numbits,percent;
		firstnorx = TRUE;
		level = (int)(170.0 * DRMTransmitter.GetReadData()->GetLevelMeter()); //this is for reading audio input level to the speech codecs? DM
		//level = 1; // (int)(170.0 * DRMReceiver.GetReceiver()->GetLevelMeter()); //try this DM - probably wrong - it's for transmit level
		if (TransmParam->Service[0].eAudDataFlag == CParameter::SF_AUDIO)
		{
			if (TransmParam->Service[0].AudioParam.eAudioCoding == CParameter::AC_LPC)
				SendMessage (GetDlgItem (hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)"LPC");
			if (TransmParam->Service[0].AudioParam.eAudioCoding == CParameter::AC_SPEEX)
				SendMessage (GetDlgItem (hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)"SPEEX");
			if (TransmParam->Service[0].AudioParam.eAudioCoding == CParameter::AC_SSTV)
				SendMessage (GetDlgItem (hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)"SSTV");
		}
		else
		{
			if (RxRSlevel == 0) {
				sprintf(tempstr, "Data"); //
			}
			else
				sprintf(tempstr, "Data:RS%d", RxRSlevel); //
			SendMessage(GetDlgItem(hwnd, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)tempstr);

			numbits = DRMTransmitter.GetAudSrcEnc()->GetPicCnt();
			percent = DRMTransmitter.GetAudSrcEnc()->GetPicPerc();
			wsprintf(tempstr,"%d / %d%%",numbits,percent);
			SendMessage (GetDlgItem (hwnd, IDC_EDIT5), WM_SETTEXT, 0, (LPARAM)tempstr);

			wsprintf(tempstr, "Sending data of %d bytes", EncFileSize); //added info - in testing - DM
			SendMessage(GetDlgItem(hwnd, IDC_EDIT6), WM_SETTEXT, 0, (LPARAM)tempstr); //send to stats window DM

			percent = DRMTransmitter.GetAudSrcEnc()->GetNoOfPic();
			if ((numbits+1 > percent) && (stoptx == -1)) stoptx = stoptx_time;
		}
		numbits = DRMTransmitter.GetParameters()->iNumDecodedBitsMSC;
		wsprintf(tempstr,"%d",numbits);
		SendMessage (GetDlgItem (hwnd, IDC_EDIT4), WM_SETTEXT, 0, (LPARAM)tempstr);
		SendMessage (GetDlgItem (hwnd, IDC_EDIT), WM_SETTEXT, 0, (LPARAM)getcall());
		if (DRMTransmitter.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_A)
			robmode = 'A';
		if (DRMTransmitter.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_B)
			robmode = 'B';
		if (DRMTransmitter.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_E)
			robmode = 'E';
		if ((DRMTransmitter.GetParameters()->GetSpectrumOccup()) == SO_0)
			specocc = 3;
		else if ((DRMTransmitter.GetParameters()->GetSpectrumOccup()) == SO_1)
			specocc = 5;
		if (DRMTransmitter.GetParameters()->eSymbolInterlMode == CParameter::SI_LONG)
			interl = 'L';
		else if (DRMTransmitter.GetParameters()->eSymbolInterlMode == CParameter::SI_SHORT)
			interl = 'S';
		if (DRMTransmitter.GetParameters()->eMSCCodingScheme == CParameter::CS_2_SM)
			qam = 16;
		else if (DRMTransmitter.GetParameters()->eMSCCodingScheme == CParameter::CS_3_SM)
			qam = 64;
		else
			qam = 4;
		prot = DRMTransmitter.GetParameters()->MSCPrLe.iPartB;
		wsprintf(tempstr,"%c/%c/%d/%d/2.%d",robmode,interl,qam,prot,specocc);	
		SendMessage (GetDlgItem (hwnd, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)tempstr);	
	}


	if (IsRX)
	{
		if ((disptype == 3) || (disptype == 8))
			PaintWaveData(hwnd,FALSE);
		else
			PaintWaveData(hwnd,TRUE);
	}
	else
		PaintWaveData(hwnd,TRUE);
	
  }
  catch (CGenErr GenErr) //Add some descriptive messages here for the exception generated, and also delete heap buffers if they still exist DM
  {
  	return;
  }

}


BOOL CALLBACK AboutDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog (hwnd, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}


BOOL CALLBACK SendBSRDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	

	char tmpbsrname[20];
	int tmpno;
	BOOL send3 = FALSE;
    switch (message)
    {
    case WM_INITDIALOG:
    case WM_NOTIFY:
		if (bCloseMe)
		{
			bCloseMe = FALSE;
			tmpno = GetDlgItemInt( hwnd, IDC_BSRINST, NULL, FALSE );
			txbsr_onscreen_arr[tmpno] = FALSE;
			EndDialog (hwnd, 0);
		}
		else
		{
//			wsprintf(tmpbsrname,"bsr%d.bin",txbsrposind);  //removed DM == figure out what this was doing - probably multi-window BSR
			wsprintf(tmpbsrname, "%s%s", bsrpath, "bsr.bin"); //added DM
			SetDlgItemInt( hwnd, IDC_SENDBSR_NOSEG, numbsrsegments, FALSE );
//			SetDlgItemText( hwnd, IDC_SENDBSR_FNAME, namebsrfile.c_str() ); //edited DM
			SetDlgItemText(hwnd, IDC_SENDBSR_FNAME, namebsrfile); //edited DM
			SetDlgItemInt( hwnd, IDC_BSRINST, txbsrposind, FALSE );
			SetDlgItemText( hwnd, IDC_SENDBSR_FROMCALL, consrxcall.c_str());
//			CopyFile("bsr.bin",tmpbsrname,FALSE); removed DM
		}
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK3:
			send3 = TRUE;
        case IDOK:
			if (!IsRX) return TRUE; //Return if we are transmitting.. DM
			//This appears to be for sending a bsr.bin REQUEST file to another station DM

			/*
			//New code: (not working) DM
			TXpicpospt = 0;
			SendBSR(1, 1); //send the BSR request - added DM - doesn't work with old queueing system
			SetTXmode(TRUE);

			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_GRAYED);
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_DRMSETTINGS, MF_GRAYED);
			SendMessage(GetDlgItem(messhwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"RX");
			SendMessage(GetDlgItem(messhwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"RX");
			*/

			char filenam[300];
			char filenam2[300];

			DRMTransmitter.GetAudSrcEnc()->ClearPicFileNames();
//			dummy.Init(0);
			wsprintf(filenam, "%s%s", bsrpath, "bsr.bin");
			wsprintf(filenam2, "%s%s", bsrpath, "cbsr.bin");
//			if (fast != 0) //Make this a dialog checkbox option DM
//			{
				// compress the bsr.bin file
				CopyFile(filenam, filenam2, FALSE);
				compressBSR(filenam2, filenam); //renamed to prevent conflict with zlib DM
				remove(filenam2); //delete temp file DM

//			}
//			tmpno = GetDlgItemInt( hwnd, IDC_BSRINST, NULL, FALSE );
//			wsprintf(tmpbsrname,"bsr%d.bin",tmpno);
//			CopyFile(tmpbsrname,"bsr.bin",FALSE);
 
			strcpy(pictfile[0],"bsr.bin");	strcpy(filetitle[0],"bsr.bin");
			strcpy(pictfile[1],"bsr.bin");	strcpy(filetitle[1],"bsr.bin");
			TXpicpospt = 2;
			if (send3)
			{
				strcpy(pictfile[2],"bsr.bin");	strcpy(filetitle[2],"bsr.bin");
				strcpy(pictfile[3],"bsr.bin");	strcpy(filetitle[3],"bsr.bin");
				TXpicpospt = 4;
			}
			SetTXmode(TRUE);
			if (RX_Running) DRMReceiver.NotRec();
			IsRX = FALSE;
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_GRAYED);
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_DRMSETTINGS, MF_GRAYED);
			if (TX_Running)
			{
				char dcbuf[20];
				DRMTransmitter.Init();
				DRMTransmitter.Send();
				sprintf(dcbuf,"%d",(int)DRMTransmitter.GetCarOffset());
				SetDlgItemText( messhwnd, IDC_DCFREQ, dcbuf);
			}
			SendMessage (GetDlgItem (messhwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"RX");
			SendMessage (GetDlgItem (messhwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"RX");
			TXpicpospt = 0;
			
			return TRUE;

		case IDCANCEL:
			tmpno = GetDlgItemInt( hwnd, IDC_BSRINST, NULL, FALSE );
//			wsprintf(tmpbsrname,"bsr%d.bin",tmpno); //removed DM
			wsprintf(tmpbsrname, "%s%s", bsrpath, "bsr.bin"); //added DM
			remove(tmpbsrname); //delete bsr.bin file when cancelled DM
			txbsr_onscreen_arr[tmpno] = FALSE;
            EndDialog (hwnd, 0);
            return TRUE;
        }
        break;
    case WM_CLOSE:
		tmpno = GetDlgItemInt( hwnd, IDC_BSRINST, NULL, FALSE );
//		wsprintf(tmpbsrname,"bsr%d.bin",tmpno); //removed DM
		wsprintf(tmpbsrname, "%s%s", bsrpath, "bsr.bin"); //added DM
		remove(tmpbsrname);
		txbsr_onscreen_arr[tmpno] = FALSE;
 		EndDialog (hwnd, 0);
		return TRUE;
     }
    return FALSE;
}


BOOL CALLBACK AnswerBSRDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	char tmpbsrname[20];
	int tmpno;
	BOOL send3 = FALSE;
    switch (message)
    {
    case WM_NOTIFY:
    case WM_INITDIALOG:
		SetDlgItemInt( hwnd, ID_ABSRINST, bsrposind, FALSE );
		SetDlgItemInt( hwnd, IDC_SENDNOBSR, segnobsrfile(), FALSE );
		SetDlgItemText( hwnd, IDC_SENDNAMEBSR, filetosend.c_str());
		SetDlgItemText( hwnd, IDC_SENDCALLBSR, consrxcall.c_str());
//		wsprintf(tmpbsrname,"bsrreq%d.bin",bsrposind);
//		CopyFile("bsrreq.bin",tmpbsrname,FALSE); //is this necessary? DM
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK3:
			send3 = TRUE;
        case IDOK:
			if (!IsRX) return TRUE;
			tmpno = GetDlgItemInt( hwnd, ID_ABSRINST, NULL, FALSE );
//			wsprintf(tmpbsrname,"bsrreq%d.bin",tmpno); //removed DM
//			CopyFile(tmpbsrname,"bsrreq.bin",FALSE); //removed DM
			readbsrfile(bsrpath); //edited DM - readbsrfile needs a path now (but not a filename)
			if (send3)
				writeselsegments(3);
			else
				writeselsegments(1);
			if (RX_Running) DRMReceiver.NotRec();
			IsRX = FALSE;
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_GRAYED);
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_DRMSETTINGS, MF_GRAYED);
			if (TX_Running)
			{
				char dcbuf[20];
				DRMTransmitter.Init();
				DRMTransmitter.Send();
				sprintf(dcbuf,"%d",(int)DRMTransmitter.GetCarOffset());
				SetDlgItemText( messhwnd, IDC_DCFREQ, dcbuf);
			}
			SendMessage (GetDlgItem (messhwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"RX");
			SendMessage (GetDlgItem (messhwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"RX");  
            return TRUE;
		case IDCANCEL:
			tmpno = GetDlgItemInt( hwnd, ID_ABSRINST, NULL, FALSE );
//			wsprintf(tmpbsrname,"bsrreq%d.bin",tmpno); //removed DM = This is to handle multiple BSR windows and the DLL version doesn't seem to support it DM
			wsprintf(tmpbsrname, "%s%s", bsrpath, "bsr.bin"); //added DM
			remove(tmpbsrname);
			bsr_onscreen_arr[tmpno] = FALSE;
            EndDialog (hwnd, 0);
            return TRUE;
        }
        break;
    case WM_CLOSE:
		tmpno = GetDlgItemInt( hwnd, ID_ABSRINST, NULL, FALSE );
//		wsprintf(tmpbsrname,"bsrreq%d.bin",tmpno); //removed DM = This is to handle multiple BSR windows and the DLL version doesn't seem to support it DM
		wsprintf(tmpbsrname, "%s%s", bsrpath, "bsr.bin"); //added DM
		remove(tmpbsrname);
		bsr_onscreen_arr[tmpno] = FALSE;
 		EndDialog (hwnd, 0);
		return TRUE;
     }
    return FALSE;
}

BOOL CALLBACK CallSignDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	char callbuf[20];
    switch (message)
    {
    case WM_INITDIALOG:
		SetDlgItemText( hwnd, IDC_EDIT1, getcall() );
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
			GetDlgItemText( hwnd, IDC_EDIT1, callbuf, 20);
			setcall(callbuf);
			savevar();
			if (runmode != 'R') TransmParam->Service[0].strLabel = getcall();
            EndDialog (hwnd, 0);
            return TRUE;
        case IDCANCEL:
            EndDialog (hwnd, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}


BOOL CALLBACK DRMSettingsDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	char dcbuf[40]; //was 20 DM
	float TxDCFreq = 0.0;
    switch (message)
    {
    case WM_INITDIALOG:
		if (DRMTransmitter.GetParameters()->GetSpectrumOccup() == SO_1)
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_BW_25), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_BW_23), BM_SETCHECK, (WPARAM)1, 0);
		if (DRMTransmitter.GetParameters()->eSymbolInterlMode == CParameter::SI_SHORT)
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_SHORT400MS), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_LONG2S), BM_SETCHECK, (WPARAM)1, 0);
		
		if (DRMTransmitter.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_A) 
			SendMessage(GetDlgItem(hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)1, 0);
		else if (DRMTransmitter.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_B)
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)1, 0);
		else //if (DRMTransmitter.GetParameters()->GetWaveMode() == RM_ROBUSTNESS_MODE_E) //else must be mode E! DM
			SendMessage(GetDlgItem(hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)1, 0);
		
		if (DRMTransmitter.GetParameters()->eMSCCodingScheme == CParameter::CS_1_SM)
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)1, 0);
		else if (DRMTransmitter.GetParameters()->eMSCCodingScheme == CParameter::CS_2_SM)
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)1, 0);
		if (DRMTransmitter.GetParameters()->MSCPrLe.iPartB == 0)
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_NORMAL), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_LOW), BM_SETCHECK, (WPARAM)1, 0);
		TxDCFreq = DRMTransmitter.GetCarOffset();
		sprintf(dcbuf,"%.1f",TxDCFreq);
		SetDlgItemText( hwnd, IDC_DCOFFS, dcbuf);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDCANCEL:
            EndDialog (hwnd, 0);
            return TRUE;

        case IDOK:
 			GetDlgItemText( hwnd, IDC_DCOFFS, dcbuf, 40); //was 20 DM == Changed for better alignment DM
			if (sscanf(dcbuf,"%f",&TxDCFreq) == 1)
				DRMTransmitter.SetCarOffset(TxDCFreq);
			DRMTransmitter.Init();
			DRMTransmitter.GetParameters()->Service[0].DataParam.iPacketLen = 
				calcpacklen(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC); 
			setmodeqam(DRMTransmitter.GetParameters()->GetWaveMode(),
				       DRMTransmitter.GetParameters()->eMSCCodingScheme,
					   DRMTransmitter.GetParameters()->eSymbolInterlMode,
					   DRMTransmitter.GetParameters()->GetSpectrumOccup());
            EndDialog (hwnd, 0);
            return TRUE;

		case ID_INTERLEAVE_SHORT400MS:
			DRMTransmitter.GetParameters()->eSymbolInterlMode = CParameter::SI_SHORT;
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_SHORT400MS), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_LONG2S), BM_SETCHECK, (WPARAM)0, 0);
			break;
		case ID_INTERLEAVE_LONG2S:
			DRMTransmitter.GetParameters()->eSymbolInterlMode = CParameter::SI_LONG;
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_SHORT400MS), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_LONG2S), BM_SETCHECK, (WPARAM)1, 0);
			break;
		case ID_SETTINGS_MSCPROTECTION_NORMAL:
			DRMTransmitter.GetParameters()->SetMSCProtLev(0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_NORMAL), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_LOW), BM_SETCHECK, (WPARAM)0, 0);
			break;
		case ID_SETTINGS_MSCPROTECTION_LOW:
			DRMTransmitter.GetParameters()->SetMSCProtLev(1);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_NORMAL), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_LOW), BM_SETCHECK, (WPARAM)1, 0);
			break;
		case ID_MSCCODING_4QAM:
			DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_1_SM;
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)0, 0);
			break;
		case ID_MSCCODING_16QAM:
			DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_2_SM;
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)0, 0);
			break;
		case ID_MSCCODING_64QAM:
			DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_3_SM;
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)1, 0);
			break;
		case ID_MODE_ROBMODEA:
			DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_A,
				DRMTransmitter.GetParameters()->GetSpectrumOccup());
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)0, 0);
			break;
		case ID_MODE_ROBMODEB:
			DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_B,
				DRMTransmitter.GetParameters()->GetSpectrumOccup());
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)0, 0);
			break;
		case ID_MODE_ROBMODEE:
			DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_E,
				DRMTransmitter.GetParameters()->GetSpectrumOccup());
			SendMessage(GetDlgItem(hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)1, 0);
			break;
		case ID_SETTINGS_BW_25:
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_BW_25), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_BW_23), BM_SETCHECK, (WPARAM)0, 0);
			DRMTransmitter.GetParameters()->SetSpectrumOccup(SO_1);
			break;
		case ID_SETTINGS_BW_23:
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_BW_23), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_BW_25), BM_SETCHECK, (WPARAM)0, 0);
			DRMTransmitter.GetParameters()->SetSpectrumOccup(SO_0);
			break;
		case ID_SET_A_MAXSPEED:
			DRMTransmitter.GetParameters()->eSymbolInterlMode = CParameter::SI_LONG;
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_SHORT400MS), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_LONG2S), BM_SETCHECK, (WPARAM)1, 0);
			DRMTransmitter.GetParameters()->SetMSCProtLev(1);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_NORMAL), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_LOW), BM_SETCHECK, (WPARAM)1, 0);
			DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_2_SM;
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)0, 0);
			DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_A, DRMTransmitter.GetParameters()->GetSpectrumOccup());
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)0, 0); //added DM
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)1, 0);
			break;
		case ID_SET_B_DEFAULT:
			DRMTransmitter.GetParameters()->eSymbolInterlMode = CParameter::SI_SHORT;
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_SHORT400MS), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_INTERLEAVE_LONG2S), BM_SETCHECK, (WPARAM)0, 0);
			DRMTransmitter.GetParameters()->SetMSCProtLev(0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_NORMAL), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_MSCPROTECTION_LOW), BM_SETCHECK, (WPARAM)0, 0);
			DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_2_SM;
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)0, 0);
			DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_B,
				DRMTransmitter.GetParameters()->GetSpectrumOccup());
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem (hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem (hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)0, 0); //added DM
			break;
		case ID_SET_B_ROBUST:
			DRMTransmitter.GetParameters()->eMSCCodingScheme = CParameter::CS_1_SM;
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_64QAM), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_16QAM), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MSCCODING_4QAM ), BM_SETCHECK, (WPARAM)1, 0);
			DRMTransmitter.GetParameters()->InitCellMapTable(RM_ROBUSTNESS_MODE_B,
				DRMTransmitter.GetParameters()->GetSpectrumOccup());
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEB), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEA), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, ID_MODE_ROBMODEE), BM_SETCHECK, (WPARAM)0, 0); //added DM
			break;
        }
        break;
    }
    return FALSE;
}

int centerfreq = 350;
int windowsize = 200;

BOOL CALLBACK DRMRXSettingsDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	BOOL sucess;
    switch (message)
    {
    case WM_INITDIALOG:
		SetDlgItemInt(hwnd, IDC_FREQACC, sensivity, FALSE); //this should be removed DM
		SetDlgItemInt(hwnd, IDC_SEARCHWINLOWER, centerfreq, FALSE);
		SetDlgItemInt(hwnd, IDC_SEARCHWINUPPER, windowsize, FALSE);
		if (rxaudfilt)
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_AUDIOFILTER_RXFILTER), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, ID_SETTINGS_AUDIOFILTER_RXFILTER), BM_SETCHECK, (WPARAM)0, 0);
		setRXfilt((rxaudfilt==TRUE));
		if (fastreset)
			SendMessage (GetDlgItem (hwnd, IDC_SETTINGS_FASTRESET), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, IDC_SETTINGS_FASTRESET), BM_SETCHECK, (WPARAM)0, 0);
		DRMReceiver.SetFastReset(fastreset==TRUE);
		break;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
//			sensivity = GetDlgItemInt(hwnd, IDC_FREQACC, &sucess, FALSE); // -------------- needs disabling for DLL files update -----------------
//			if (sucess) DRMReceiver.SetFreqAcqSensivity(2.0 - (_REAL)sensivity/100.0); //edited DM  -------------- needs disabling for DLL files update -----------------
			centerfreq = GetDlgItemInt(hwnd, IDC_SEARCHWINLOWER, &sucess, FALSE);
			windowsize = GetDlgItemInt(hwnd, IDC_SEARCHWINUPPER, &sucess, FALSE);
			if (sucess)
			{
				windowsize = GetDlgItemInt(hwnd, IDC_SEARCHWINUPPER, &sucess, FALSE);
				if (sucess) DRMReceiver.SetFreqAcqWinSize((_REAL)centerfreq,(_REAL)windowsize);
			}
            EndDialog (hwnd, 0);
            return TRUE;
        case IDAPP:
//			sensivity = GetDlgItemInt(hwnd, IDC_FREQACC, &sucess, FALSE); // -------------- needs disabling for DLL files update -----------------
//			if (sucess) DRMReceiver.SetFreqAcqSensivity(2.0 - (_REAL)sensivity/100.0); //edited DM  -------------- needs disabling for DLL files update -----------------
			centerfreq = GetDlgItemInt(hwnd, IDC_SEARCHWINLOWER, &sucess, FALSE);
			if (sucess)
			{
				windowsize = GetDlgItemInt(hwnd, IDC_SEARCHWINUPPER, &sucess, FALSE);
				if (sucess) DRMReceiver.SetFreqAcqWinSize((_REAL)centerfreq,(_REAL)windowsize);
			}
            return TRUE;
		case ID_SETTINGS_AUDIOFILTER_RXFILTER:
			if (rxaudfilt)
			{
				rxaudfilt = FALSE;
				SendMessage (GetDlgItem (hwnd, ID_SETTINGS_AUDIOFILTER_RXFILTER), BM_SETCHECK, (WPARAM)0, 0);
				setRXfilt(false);
			}
			else
			{
				rxaudfilt = TRUE;
				SendMessage (GetDlgItem (hwnd, ID_SETTINGS_AUDIOFILTER_RXFILTER), BM_SETCHECK, (WPARAM)1, 0);
				setRXfilt(true);
			}
			break;
		case IDC_SETTINGS_FASTRESET:
			if (fastreset)
				SendMessage (GetDlgItem (hwnd, IDC_SETTINGS_FASTRESET), BM_SETCHECK, (WPARAM)0, 0);
			else
				SendMessage (GetDlgItem (hwnd, IDC_SETTINGS_FASTRESET), BM_SETCHECK, (WPARAM)1, 0);
			fastreset = !fastreset;
			DRMReceiver.SetFastReset(fastreset==TRUE);
			break;
		}
	break;
	}
    return FALSE;
}

BOOL CALLBACK TextMessageDlgProc (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	FILE * txtset = NULL;
	char textbuf[200];
	int junk = 0; //added DM
    switch (message)
    {
    case WM_INITDIALOG:
		txtset = fopen("textmessage.txt","rt");
		if (txtset != NULL)
		{
			junk = fscanf(txtset,"%[^\0]",&textbuf); //edited DM
			fclose(txtset);
		}
		else 
			strcpy(textbuf," \x0d\x0a This is a Test ");
		SetDlgItemText( hwnd, IDC_TEXTMESSAGE, textbuf);		
		if (UseTextMessage)
			SendMessage (GetDlgItem (hwnd, IDC_USETEXTMESSAGE), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, IDC_USETEXTMESSAGE), BM_SETCHECK, (WPARAM)0, 0);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
			GetDlgItemText( hwnd, IDC_TEXTMESSAGE, textbuf, 200);
			txtset = fopen("textmessage.txt","wt");
			if (txtset != NULL)
			{
				fprintf(txtset,"%s",&textbuf);
				fclose(txtset);
			}
			DRMTransmitter.GetAudSrcEnc()->ClearTextMessage();
			if (UseTextMessage)
			{
				DRMTransmitter.GetAudSrcEnc()->SetTextMessage(textbuf);
				DRMTransmitter.GetParameters()->Service[0].AudioParam.bTextflag = TRUE;
			}
			else
				DRMTransmitter.GetParameters()->Service[0].AudioParam.bTextflag = FALSE;
            EndDialog (hwnd, 0);
            return TRUE;
        case IDCANCEL:
            EndDialog (hwnd, 0);
            return TRUE;
		case IDC_USETEXTMESSAGE:
			if (UseTextMessage)
			{
				UseTextMessage = FALSE;
				settext(FALSE);
				SendMessage (GetDlgItem (hwnd, IDC_USETEXTMESSAGE), BM_SETCHECK, (WPARAM)0, 0);
				DRMTransmitter.GetAudSrcEnc()->ClearTextMessage();
			}
			else
			{
				UseTextMessage = TRUE;
				settext(TRUE);
				SendMessage (GetDlgItem (hwnd, IDC_USETEXTMESSAGE), BM_SETCHECK, (WPARAM)1, 0);
			}
			return TRUE;
        }
        break;
    case WM_CLOSE:
 		EndDialog (hwnd, 0);
		return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK RXTextMessageDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	char textbuf[200];
    switch (message)
    {
    case WM_INITDIALOG:
		RXTextMessageON = TRUE;
		strcpy(textbuf,"Waiting");
		SetDlgItemText( hwnd, IDC_RXTEXTMESSAGE, textbuf);
		return TRUE;
    case WM_NOTIFY:
		strcpy(textbuf,DRMReceiver.GetParameters()->Service[0].AudioParam.strTextMessage.c_str());
		if (strlen(textbuf) >= 1)
			SetDlgItemText( hwnd, IDC_RXTEXTMESSAGE, textbuf);
		return TRUE;
    case WM_COMMAND:
		RXTextMessageWasClosed = TRUE;
		RXTextMessageON = FALSE;
		EndDialog (hwnd, 0);
		return TRUE;
    case WM_CLOSE:
		RXTextMessageON = FALSE;
 		EndDialog (hwnd, 0);
		return TRUE;
    }
    return FALSE;
}

BOOL CALLBACK TXPictureDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	BOOL res;
    switch (message)
    {
    case WM_INITDIALOG:
		putfiles(hwnd);
		if (LeadIn == 2) {
			//2 instances
			SendMessage(GetDlgItem (hwnd, IDC_SENDONCE ), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem (hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem (hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
		}
		else if (LeadIn == 3) {
			//3 instances
			SendMessage (GetDlgItem (hwnd, IDC_SENDONCE ), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
		}
		else if (LeadIn == 4) {
			//For RS1
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
		}
		else if (LeadIn == 5) {
			//For RS2
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
		}
		else if (LeadIn == 6) {
			//For RS3
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
		}
		else if (LeadIn == 7) {
			//For RS4
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)1, 0);
		}
		else {
			//1 instance
			SendMessage(GetDlgItem (hwnd, IDC_SENDONCE ), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem (hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem (hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
		}
		if (longleadin)
			SendMessage (GetDlgItem (hwnd, IDC_LEADINLONG ), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage (GetDlgItem (hwnd, IDC_LEADINLONG ), BM_SETCHECK, (WPARAM)0, 0);
		if (autoaddfiles == TRUE) //added DM -----------------------------------------
			SendMessage(GetDlgItem(hwnd, IDC_ADDALLFILES), BM_SETCHECK, (WPARAM)1, 0);
		else
			SendMessage(GetDlgItem(hwnd, IDC_ADDALLFILES), BM_SETCHECK, (WPARAM)0, 0);
		return TRUE;
		break;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
			if (TXpicpospt == 0)
			{
				EndDialog (hwnd, 0);
				return TRUE;
			}
			SetTXmode(TRUE);
			if (RX_Running) DRMReceiver.NotRec();
			IsRX = FALSE;
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_FILETRANSFER_SENDFILE, MF_GRAYED);
			EnableMenuItem(GetMenu(messhwnd), ID_SETTINGS_DRMSETTINGS, MF_GRAYED);
			if (TX_Running)
			{
				char dcbuf[20];
				DRMTransmitter.Init();
				//if (callisok()) DRMTransmitter.Send(); //edited DM
				DRMTransmitter.Send();
				sprintf(dcbuf,"%d",(int)DRMTransmitter.GetCarOffset());
				SetDlgItemText( messhwnd, IDC_DCFREQ, dcbuf);
			}
			SendMessage (GetDlgItem (messhwnd, IDB_START), WM_SETTEXT, 0, (LPARAM)"RX");
			SendMessage (GetDlgItem (messhwnd, IDB_STARTPIC), WM_SETTEXT, 0, (LPARAM)"RX");
			EndDialog (hwnd, 0);
			return TRUE;
		case ID_CHOOSEFILE:
			if (TXpicpospt > 31) //edited DM
			{
				MessageBox(hwnd,"Too many Files in Buffer","Buffer Full",MB_OK);
				return TRUE; 
			}
			res = GetFileName(hwnd,pictfile[TXpicpospt],filetitle[TXpicpospt], sizeof(pictfile[TXpicpospt]),&file_size[TXpicpospt]);
			if (res == 1) TXpicpospt++; //increment file counter/pointer
			putfiles(hwnd); //add filename to window

			//if autoaddfiles = TRUE then scan for more matching filenames for SWRG - DM 
			if (autoaddfiles == 1) {
				BOOL hResult = FALSE;
				HANDLE hFind = 0;
				WIN32_FIND_DATA   FindFileData;

				int maxmatch = 20; //match 20 files maximum
				//make a mask from first pictfile[TXpicpospt]
				char temp[260]; //make an array to hold the filename path string
				char temp2[260]; //make an array to copy it to
				strcpy(temp, pictfile[TXpicpospt - 1]); //copy the filename string into temp
				int templen = 0; //an int to hold the length
				int templen2 = 0; //an int to hold the length 
				templen = strlen(temp); //find the length of the string
				templen2 = templen; //copy for testing
				//Match a maximum of 20 files - if any fails, stop
				while (maxmatch > 0) {
					//only check back 8 places or so (this could be extended to allow for descriptive names to be added)
					while ((templen > (templen - (min(templen, 8)))) && (maxmatch > 0)) {
						//add code here to replace extension with a * so it matches any file type DM - Working 13/03/2021 1:16PM
						//check for "."
						if (temp[templen] == 46) {
						//as soon as a dot is found, replace ext with *0
							templen++; //go to next char
							//make sure we have room (must be at least 1 char after .)
							if ((templen2-templen) >= 1) {
								temp[templen] = 42; //add a *
								templen++; //go to next char
								if (templen <= templen2) {
									temp[templen] = 0; //add a 0 to terminate string if needed
								}
								templen--;
							}
							templen--;
						}

						//quit as soon as a hyphen is found
						if (temp[templen] == 45) {
							maxmatch = 0;
							break;
						}
						//Arithmetic is done in ASCII values
							//look for numbers 0-9
						if ((temp[templen] >= 48) && (temp[templen] <= 57)) {
							//found a number! increment it if it's 0-9
							if ((temp[templen] >= 48) && (temp[templen] <= 57)) {
								temp[templen] = temp[templen] + 1;
								//if it's become a colon it overflowed - set it to zero and inc the next digit instead
								if (temp[templen] == 58) {
									temp[templen] = 48; //set it to zero and carry it next
									templen--; //next digit left
									temp[templen] = temp[templen] + 1; //carry to next digit
								}
								//a number was found - add the file to the list
								//next filename to find is in temp
								hFind = FindFirstFileA(temp, &FindFileData); //get the file handle
								if (hFind != INVALID_HANDLE_VALUE) {
									//a file was found
									file_size[TXpicpospt] = FindFileData.nFileSizeLow; //add file size info
 //									strcpy(pictfile[TXpicpospt],temp); //pictfile holds the full path and filename in a 2-dimansional array

									//take the short name and use it to overwrite the name on the full path
									strcpy(temp2, temp); //copy the full path to a new array
									fixname(temp2, FindFileData.cFileName); //overwrite the wildcard filename with the name that was actually found
									strcpy(filetitle[TXpicpospt], FindFileData.cFileName); //filetitle holds the short filename in a 2-dimansional array
									strcpy(pictfile[TXpicpospt], temp2); //pictfile holds the full path and filename in a 2-dimansional array
									hResult = FindClose(hFind); //close file handle
									TXpicpospt++; //increment array pointer
								}
								//if file not found, quit
							}
						}
						//no number found - try the next character
						else templen--;
					}

					maxmatch--;
					//Then load the next matching filename, and continue until a hyphen is found
				}
				putfiles(hwnd); //add filenames to window
			}  //end Autoadd files

				return TRUE;
		case ID_DELALLFILES:
			TXpicpospt = 0;
			putfiles(hwnd);
			return TRUE;
		case ID_CANCELPICTX:
			EndDialog (hwnd, 0);
			return TRUE;
		case IDC_ADDALLFILES:	//added DM
			if (autoaddfiles == TRUE)
			{
				SendMessage(GetDlgItem(hwnd, IDC_ADDALLFILES), BM_SETCHECK, (WPARAM)0, 0);
				autoaddfiles = FALSE;
			}
			else
			{
				SendMessage(GetDlgItem(hwnd, IDC_ADDALLFILES), BM_SETCHECK, (WPARAM)1, 0);
				autoaddfiles = TRUE;
			}
			return TRUE;
		case IDC_SENDONCE:
			LeadIn = 1;
			SendMessage (GetDlgItem (hwnd, IDC_SENDONCE ), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
			return TRUE;
		case IDC_SENDTWICE:
			LeadIn = 2;
			SendMessage (GetDlgItem (hwnd, IDC_SENDONCE ), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
			return TRUE;
		case IDC_SENDTHREE:
			LeadIn = 3;
			SendMessage (GetDlgItem (hwnd, IDC_SENDONCE ), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage (GetDlgItem (hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
			return TRUE;
			/*
		case IDC_RS1:
			LeadIn = 4;
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
			return TRUE;
		case IDC_RS2:
			LeadIn = 5;
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
			return TRUE;
		case IDC_RS3:
			LeadIn = 6;
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)1, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)0, 0);
			return TRUE;
			*/
		case IDC_RS4:
			LeadIn = 7;
			SendMessage(GetDlgItem(hwnd, IDC_SENDONCE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTWICE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_SENDTHREE), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS1), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS2), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS3), BM_SETCHECK, (WPARAM)0, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RS4), BM_SETCHECK, (WPARAM)1, 0);
			return TRUE;
		case IDC_LEADINLONG:
			if (longleadin)
			{
				longleadin = FALSE;
				SendMessage (GetDlgItem (hwnd, IDC_LEADINLONG ), BM_SETCHECK, (WPARAM)0, 0);
//				DRMTransmitter.GetAudSrcEnc()->SetStartDelay(starttx_time); //edited DM
				DRMTransmitter.GetAudSrcEnc()->SetTheStartDelay(starttx_time); //added DM
			}
			else
			{
				longleadin = TRUE;
				SendMessage (GetDlgItem (hwnd, IDC_LEADINLONG ), BM_SETCHECK, (WPARAM)1, 0);
//				DRMTransmitter.GetAudSrcEnc()->SetStartDelay(starttx_time_long); //edited DM
				DRMTransmitter.GetAudSrcEnc()->SetTheStartDelay(starttx_time_long); //added DM
			}
			return TRUE;
        }
        break;
    case WM_CLOSE:
 		EndDialog (hwnd, 0);
		return TRUE;
   }
    return FALSE;
}

//Tick or untick the appropriate sound device in the menu
//This now needs a window menu INDEX number and not a driver index number! So add the menu base offsets. DONE
//edited DM - num = menu index (NOT driver index)
void unchecksoundrx(HWND hDlg, int num) 
{
	for (int i = 0; i <= numdevIn; i++) {
		int num2 = IDM_O_RX_I_DRIVERS0 + i;
		if (num2 == num) CheckMenuItem(GetMenu(hDlg), num, MF_BYCOMMAND | MF_CHECKED); 
		else CheckMenuItem(GetMenu(hDlg), num2, MF_BYCOMMAND | MF_UNCHECKED);
	}
	DRMReceiver.GetSoundInterface()->SetInDev(getsoundin('r')); //set data receiver sound in device DM
}
void unchecksoundtx(HWND hDlg, int num)
{
	for (int i = 0; i <= numdevOut; i++) {
		int num2 = IDM_O_TX_O_DRIVERS0 + i;
		if (num2 == num) CheckMenuItem(GetMenu(hDlg), num, MF_BYCOMMAND | MF_CHECKED);
		else CheckMenuItem(GetMenu(hDlg), num2, MF_BYCOMMAND | MF_UNCHECKED);
	}
	DRMTransmitter.GetSoundInterface()->SetOutDev(getsoundout('t')); //set data transmitter sound out device DM
}
void unchecksoundvoI(HWND hDlg, int num)
{
	for (int i = 0; i <= numdevIn; i++) {
		int num2 = IDM_O_VO_I_DRIVERS0 + i;
		if (num2 == num) CheckMenuItem(GetMenu(hDlg), num, MF_BYCOMMAND | MF_CHECKED);
		else CheckMenuItem(GetMenu(hDlg), num2, MF_BYCOMMAND | MF_UNCHECKED);
	}
	DRMTransmitter.GetSoundInterface()->SetInDev(getsoundin('t')); //set voice transmitter sound in device
}
void unchecksoundvoO(HWND hDlg, int num)
{
	for (int i = 0; i <= numdevOut; i++) {
		int num2 = IDM_O_VO_O_DRIVERS0 + i;
		if (num2 == num) CheckMenuItem(GetMenu(hDlg), num, MF_BYCOMMAND | MF_CHECKED);
		else CheckMenuItem(GetMenu(hDlg), IDM_O_VO_O_DRIVERS0 + i, MF_BYCOMMAND | MF_UNCHECKED);
	}
	DRMReceiver.GetSoundInterface()->SetOutDev(getsoundout('r')); //set voice receiver sound out device
}

//overwrite the wildcard filename on the full path with the name that was found DM 
void fixname(char* wildpath, char* filename) {
	int pos = strlen(wildpath); //get the length of the path (with wildcard)
	pos--; //go back 1 char to point to * (because strlen has it pointing at the null terminator)
	int pos2 = 0; //start at 0 for filename and go forward
	char test = 0;
	//find the last backslash in the path
	while ((test != 92) && (pos > 0)) {
		pos--;
		test = wildpath[pos]; //try combining this into the conditional (careful! C++ only evaluates the left hand side of && if it's false...)
	}
	if (test == 92) {
		//found it, pos = a pointer to the \...
		pos++; //next char is the start of the filename
		//now overwrite the wildcard name with the filename that was found
		while (test != 0) {
			wildpath[pos] = filename[pos2]; //copy first...
			test = wildpath[pos]; //then test for a zero - that way we will always copy the null termination
			pos++;
			pos2++;
			//make sure this writes the zero!
		}
	}
	else {
		//we failed to find a slash!
	}
}


//DM - The routines below are from the DLL version hamdrm.cpp file:   ======================================================================================================================

// Send BSR
BOOL __cdecl SendBSR(int inst, int fast)
{
	CVector<short> dummy;
	char filenam[300];
	char filenam2[300];

	DRMTransmitter.GetAudSrcEnc()->ClearPicFileNames();
	dummy.Init(0);
	wsprintf(filenam, "%s%s", bsrpath, "bsr.bin");
	wsprintf(filenam2, "%s%s", bsrpath, "cbsr.bin");
	if (fast != 0)
	{
		// compress the bsr.bin file
		CopyFile(filenam, filenam2, FALSE);
		compressBSR(filenam2, filenam); //renamed to prevent conflict with zlib DM
	}
	DRMTransmitter.GetAudSrcEnc()->SetPicFileName(filenam, "bsr.bin", dummy);
	if (inst >= 2)
		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(filenam, "bsr.bin", dummy);
	if (inst >= 3)
		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(filenam, "bsr.bin", dummy);
	if (inst >= 4)
		DRMTransmitter.GetAudSrcEnc()->SetPicFileName(filenam, "bsr.bin", dummy);
	return TRUE;
}

int iTID = 0;

BOOL __cdecl GetFileRX(char* FileName)
{
	CMOTObject NewPic;
	if (DRMReceiver.GetDataDecoder()->GetSlideShowPicture(NewPic))
	{
		char filenam[300];
		int picsize, i;
		FILE* set = NULL;

		picsize = NewPic.vecbRawData.Size();
		iTID = NewPic.iTransportID;
		if (strcmp(NewPic.strName.c_str(), "bsr.bin") == 0)
		{
			char filenam2[300];
			wsprintf(filenam, "%s%s", bsrpath, "comp.bin");
			set = fopen(filenam, "wb");
			if (set != NULL)
			{
				for (i = 0; i < picsize; i++) putc(NewPic.vecbRawData[i], set);
				fclose(set);
			}

			wsprintf(filenam2, "%s%s", bsrpath, "bsrreq.bin");
			decompressBSR(filenam, filenam2); //renamed to prevent conflict with zlib DM

			strcpy(FileName, NewPic.strName.c_str());
		}
		else
		{
			wsprintf(filenam, "%s%s", rxfilepath, NewPic.strName.c_str());
			set = fopen(filenam, "wb");
			if (set != NULL)
			{
				for (i = 0; i < picsize; i++) putc(NewPic.vecbRawData[i], set);
				fclose(set);
			}
			strcpy(FileName, NewPic.strName.c_str());
		}
		return TRUE;
	}
	return FALSE;
}

int __cdecl GetLastTID()
{
	return iTID;
}

int locpiccnt, locpicnum;
BOOL stopsend = FALSE;
int stopct = 0;
int stopval = 10;

BOOL __cdecl GetPercentTX(int* piccnt, int* percent)
{
	if (TX_Running)
	{
		locpiccnt = DRMTransmitter.GetAudSrcEnc()->GetPicCnt();
		locpicnum = DRMTransmitter.GetAudSrcEnc()->GetNoOfPic();
		*piccnt = locpiccnt;
		*percent = DRMTransmitter.GetAudSrcEnc()->GetPicPerc();

		if (stopct == 0)
		{
			if (locpiccnt + 1 > locpicnum)
			{
				stopct = 1;
				if (DRMTransmitter.GetParameters()->GetInterleaverDepth() == CParameter::SI_LONG)
					stopval = 12;
				else
					stopval = 4;
			}
		}
		else
		{
			stopct++;
			if (stopct >= stopval)
			{
				stopct = 0;
				return TRUE;
			}
		}
		return FALSE;
	}
	else
	{
		*piccnt = 0;
		*percent = 0;
		stopct = 0;
		return FALSE;
	}
}

// BSR functions - Only GetBSR is used for now.. DM
int numseg;
string bsrname;

BOOL __cdecl GetBSR(int* numbsrsegments, char* namebsrfile)
{
	int numseg;
	string bsrname;

	if (DRMReceiver.GetDataDecoder()->GetSlideShowBSR(&numseg, &bsrname, bsrpath))
	{
		*numbsrsegments = numseg;
		strcpy(namebsrfile, bsrname.c_str());
		return TRUE;
	}
	*numbsrsegments = 0;
	strcpy(namebsrfile, "No segments missing");
	return FALSE;
}

/*

BOOL __cdecl readthebsrfile(char* fnam, int* segno)
{
	strcpy(fnam, readbsrfile(bsrpath).c_str());
	*segno = segnobsrfile();
	return (strlen(fnam) >= 2);
}

void   __cdecl writebsrselsegments(int inst)
{
	writeselsegments(inst);
}

void __cdecl GetData(int* totsize, int* actsize, int* actpos)
{
	*totsize = DRMReceiver.GetDataDecoder()->GetTotSize();
	*actsize = DRMReceiver.GetDataDecoder()->GetActSize();
	*actpos = DRMReceiver.GetDataDecoder()->GetActPos();
}

BOOL __cdecl GetCorruptFileRX(char* FileName)
{
	CMOTObject NewPic;
	if (DRMReceiver.GetDataDecoder()->GetSlideShowPartPicture(NewPic))
	{
		char filenam[300];
		int picsize, i;
		FILE* set = NULL;

		picsize = NewPic.vecbRawData.Size();
		iTID = NewPic.iTransportID;
		wsprintf(filenam, "%s%s", rxcorruptpath, NewPic.strName.c_str());
		set = fopen(filenam, "wb");
		if (set != NULL)
		{
			for (i = 0; i < picsize; i++) putc(NewPic.vecbRawData[i], set);
			fclose(set);
		}
		strcpy(FileName, NewPic.strName.c_str());
		return TRUE;
	}
	return FALSE;
}

CVector<_BINARY>	actsegs;

BOOL  __cdecl GetActSegm(char* data)
{
	if (DRMReceiver.GetDataDecoder()->GetSlideShowPartActSegs(actsegs))
	{
		int i;
		int size = actsegs.Size();
		if (size >= 650) size = 650;
		for (i = 0; i < size; i++)
		{
			if (actsegs[i] == TRUE) *(data + i) = 0;
			else *(data + i) = 1;
		}
		return TRUE;
	}
	else
		return FALSE;
}

void __cdecl ResetRX(void)
{
	DRMReceiver.SetInStartMode();
}

*/

//******************************************************************************
