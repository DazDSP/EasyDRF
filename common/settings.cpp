/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
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

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <conio.h>
#include "../dialog.h"
#include "../resource.h"
//#include "libs/callsign.h"
#include "callsign2.h" //edit by DM
#include "settings.h"

HANDLE hComm = NULL;;


void comtx(char port)
{
	DWORD errcode;
	struct _DCB dcb;

	if (hComm != NULL)
	{
		CloseHandle(hComm);
		hComm = NULL;
		if (port == '0') return;
	}
	if (port == '1')
		hComm = CreateFile( "COM1:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '2')
		hComm = CreateFile( "COM2:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '3')
		hComm = CreateFile( "COM3:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '4')
		hComm = CreateFile( "COM4:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '5')
		hComm = CreateFile( "COM5:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '6')
		hComm = CreateFile( "COM6:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '7')
		hComm = CreateFile( "COM7:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (port == '8')
		hComm = CreateFile( "COM8:",  
		                GENERIC_READ | GENERIC_WRITE, 
			            FILE_SHARE_READ | FILE_SHARE_WRITE, 
				        0, 
					    OPEN_EXISTING,
						0, //FILE_FLAG_OVERLAPPED,
						0);
	if (hComm == INVALID_HANDLE_VALUE)
	{
		ClearCommError(hComm,&errcode,NULL);
		hComm = NULL;
	}
	else
	{
		GetCommState(hComm,&dcb);
		dcb.fDtrControl = 0;
		dcb.fRtsControl = 0;
		dcb.fOutxCtsFlow = 0;
		dcb.fOutxDsrFlow = 0;
		SetCommState(hComm,&dcb);
		ClearCommError(hComm,&errcode,NULL);
	}
}

void dotx(void)
{
	EscapeCommFunction(hComm,SETRTS);
	EscapeCommFunction(hComm,SETDTR);
}

void endtx(void)
{
	EscapeCommFunction(hComm,CLRRTS);
	EscapeCommFunction(hComm,CLRDTR);
}

void dorts(void)
{
	EscapeCommFunction(hComm,SETRTS);
}
void endrts(void)
{
	EscapeCommFunction(hComm,CLRRTS);
}
void dodtr(void)
{
	EscapeCommFunction(hComm,SETDTR);
}
void enddtr(void)
{
	EscapeCommFunction(hComm,CLRDTR);
}


FILE * set = NULL;
char txport = '0';
int soundouttx = 0;
int soundintx = 0;
int soundoutrx = 0;
int soundinrx = 0;
bool istxinsatnce = false;
BOOL usetext = false;
int audfiltrx = 0;
int muteonfac = 0;
int qamtype = 16;
int modetype = 1;
int interleave = 0;
int specoccuppa = 0;
int fastres = 0;
char dataisok = 0;

void savevar(void)
{
	if (rxaudfilt) audfiltrx = 1;
	else audfiltrx = 0;
	if (rtsonfac) muteonfac = 1;
	else muteonfac = 0;
	if (dtronfac) muteonfac += 2;
	set = fopen("settings.txt","wt");
	if (fastreset) fastres = 1;
	else fastres = 0;
	if (set != NULL)
	{
		fprintf(set,"%c TX_Port\n",txport);
		fprintf(set, "%s Call\n", getcall());
		fprintf(set,"%d TX_Sound_OUT_Device\n",soundouttx);
		fprintf(set,"%d TX_Sound_IN_Device\n",soundintx);
		fprintf(set,"%d RX_Sound_OUT_Device\n",soundoutrx);
		fprintf(set,"%d RX_Sound_IN_Device\n",soundinrx);
		fprintf(set,"%d Send_Text_Message\n",usetext);
		fprintf(set,"%d Display_Type\n",disptype);
		fprintf(set,"%d RX_Audfilt\n",audfiltrx);
		fprintf(set,"%d FAC_mute\n",muteonfac);
		fprintf(set,"%d QAM\n",qamtype);
		fprintf(set,"%d MODE\n",modetype);
		fprintf(set,"%d BW\n",specoccuppa);
		fprintf(set,"%d Interleave\n",interleave);
		fprintf(set, "%d FastReset\n", fastres);
		fprintf(set, "%d Instances/RS\n", LeadIn); //added DM
		fclose(set);
	}
}

void getvar(void)
{
	set = fopen("settings.txt","rt");
	if (set != NULL)
	{
		char rubbish[100];
		char thecall[20];
		fscanf(set,"%c %s",&txport,&rubbish);
		fscanf(set,"%s %s",&thecall,&rubbish);
		fscanf(set,"%d %s",&soundouttx,&rubbish);
		fscanf(set,"%d %s",&soundintx,&rubbish);
		fscanf(set,"%d %s",&soundoutrx,&rubbish);
		fscanf(set,"%d %s",&soundinrx,&rubbish);
		fscanf(set,"%d %s",&usetext,&rubbish);
		fscanf(set,"%d %s",&disptype,&rubbish);
		fscanf(set,"%d %s",&audfiltrx,&rubbish);
		fscanf(set,"%d %s",&muteonfac,&rubbish);
		fscanf(set,"%d %s",&qamtype,&rubbish);
		fscanf(set,"%d %s",&modetype,&rubbish);
		fscanf(set,"%d %s",&specoccuppa,&rubbish);
		fscanf(set,"%d %s",&interleave,&rubbish);
		fscanf(set, "%d %s", &fastres, &rubbish);
		fscanf(set, "%d %s", &LeadIn, &rubbish); //added DM
		fclose(set);
		if (audfiltrx == 1) rxaudfilt = TRUE;
		else rxaudfilt = FALSE;
		if (muteonfac == 1) rtsonfac = TRUE;
		else rtsonfac = FALSE;
		if (muteonfac == 2) dtronfac = TRUE;
		else dtronfac = FALSE;
		if (muteonfac == 3) {rtsonfac = TRUE; dtronfac = TRUE; }
		if (qamtype <= 4) qamtype = 4;
		else if (qamtype >= 64) qamtype = 64;
		else qamtype = 16;
		if (modetype <= 0) modetype = 0;
		else if (modetype >= 1) modetype = 1;
		if (interleave <= 0) interleave = 0;
		else if (interleave >= 1) interleave = 1;
		if (specoccuppa <= 0) specoccuppa = 0;
		else if (specoccuppa >= 1) specoccuppa = 1;
		if (fastres == 1) fastreset = TRUE;
		else fastreset = FALSE;
		setcall(thecall);		
		dataisok = 1;
	}
}

void testvar(void)
{
	if (dataisok == 0)
		getvar();
}

ERobMode getmode()
{
	if (modetype == 0) return RM_ROBUSTNESS_MODE_A;
	else if (modetype == 1) return RM_ROBUSTNESS_MODE_B; //edited DM
	else return RM_ROBUSTNESS_MODE_E;
}
CParameter::ECodScheme getqam()
{
	if (qamtype == 64) return CParameter::CS_3_SM;
	else if (qamtype == 16) return CParameter::CS_2_SM;
	else return CParameter::CS_1_SM;
}
CParameter::ESymIntMod getinterleave()
{
	if (interleave == 0) return CParameter::SI_SHORT;
	else return CParameter::SI_LONG;
}
ESpecOcc getspec()
{
	if (specoccuppa == 0) return SO_0;
	else return SO_1;
}

void setmodeqam(ERobMode themode, CParameter::ECodScheme theqam, CParameter::ESymIntMod theinterl, ESpecOcc thespecocc)
{
	if (themode == RM_ROBUSTNESS_MODE_A) modetype = 0;
	else if (themode == RM_ROBUSTNESS_MODE_B) modetype = 1; //edit DM
	else modetype = 2; //mode E - DM
	if (theqam == CParameter::CS_2_SM) qamtype = 16;
	else if (theqam == CParameter::CS_3_SM) qamtype = 64;
	else qamtype = 4;
	if (theinterl == CParameter::SI_SHORT) interleave = 0;
	else interleave = 1;
	if (thespecocc == SO_0) specoccuppa = 0;
	else specoccuppa = 1;
	savevar();
}

char gettxport()
{
	return txport;
}
void settxport(char port)
{
	txport= port;
	savevar();
}
int getsoundin(char rtx)
{
	testvar();
	if (rtx == 'r')
		return soundinrx;
	else
		return soundintx;
}
int getsoundout(char rtx)
{
	testvar();
	if (rtx == 'r')
		return soundoutrx;
	else
		return soundouttx;
}
void setsoundin(int dev, char rtx)
{
	testvar();
	if (rtx == 'r')
		soundinrx = dev;
	else
		soundintx = dev;
	savevar();
}
void setsoundout(int dev, char rtx)
{
	testvar();
	if (rtx == 'r')
		soundoutrx = dev;
	else
		soundouttx = dev;
	savevar();
}

void settext(BOOL ison)
{
	usetext = ison;
	savevar();
}

BOOL gettext(void)
{
	return usetext;
}

