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

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "ptt.h"

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

DWORD ModemStat;

void dotx(void)
{
	if (hComm != NULL)
	{
		EscapeCommFunction(hComm,SETRTS);
		EscapeCommFunction(hComm,SETDTR);
	}
}

void endtx(void)
{
	if (hComm != NULL)
	{
		EscapeCommFunction(hComm,CLRRTS);
		EscapeCommFunction(hComm,CLRDTR);
	}
}

