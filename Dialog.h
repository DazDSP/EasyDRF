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

#if !defined DIALOG_H
#define DIALOG_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "LzmaLib.h"

/* Implementation of global functions *****************************************/
void PostWinMessage(unsigned int MessID, int iMessageParam);
void updateLEDs(void);
void DrawBar(HWND hwnd);

// procedures called by Windows
// Main dialog handler

BOOL CALLBACK DialogProc
	(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK AboutDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK CallSignDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK DRMSettingsDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK DRMRXSettingsDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK TextMessageDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK RXTextMessageDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK TXPictureDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK SendBSRDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);
BOOL CALLBACK AnswerBSRDlgProc
   (HWND hwnd, UINT message, UINT wParam, LPARAM lParam);

void CALLBACK TimerProc
   (HWND hwnd, UINT nMsg, UINT nIDEvent, DWORD dwTime);

void OnCommand (HWND hwnd, int id, int code);

void fixname(char* wildpath, char* filename); //Added DM


//Added DM:
// BSR Routines
// Calculate BSR Request
BOOL __cdecl GetBSR(int* numbsrsegments, char* namebsrfile);
// use SendBSR to send request
// numbsrsegments -> reports number of segments missing (this is an output!)
// namebsrfile -> reports name of file (if known, this is an output!) 
// Send BSR (NEW)
BOOL __cdecl SendBSR(int inst = 1, int fast = 0);
// inst -> number of instances, 0 disables multiple inst on few seg
// fast -> fast = 1 is not compatible to older versions
// Answer BSR Request
BOOL __cdecl readthebsrfile(char* fnam, int* segno);
// reads file bsrreq.bin, returns filename. 
// returns number of missing segments in bsr request
// if false then the bsr is for another station
void   __cdecl writebsrselsegments(int inst = 1);
// writes selected segments to tx buffer, start tx after this.
// 1 to 4 allowed for instance parameter
// auto sends 2 instance if (noseg < 50), 3 if (noseg < 10), 4 if (noseg < 3)

#endif
