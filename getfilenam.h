/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 *  Daz Man 2021
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

#include "common/DrmTransmitter.h"
#include "common/DrmReceiver.h"

/*--------------------------------------------------------------------

        GetFilenam.h

  --------------------------------------------------------------------*/

extern CDRMTransmitter	DRMTransmitter;
extern CDRMReceiver	DRMReceiver;
//extern char ECCmode; //Edited DM
extern int ECCmode;
extern int specocc; //3->2.3  5->2.5
extern int TXpicpospt;
extern char pictfile[32][260]; //edited DM was 8, 300 //1200 is now 260
extern char filetitle[32][260]; //edited by DM was 32 80 //320 is now 260
extern long file_size[64]; //edited DM was 8 was 32


BOOL GetFileName (HWND hDlg, LPSTR lpszFile, LPSTR lpszFileTitle, int iMaxFileNmLen, long * fsize);

BOOL checkfilenam(LPSTR filename, int filnamsize);

BOOL checkext(LPSTR filename);

void SetTXmode(BOOL ispic);

void putfiles(HWND hwnd);

void unchecksoundrx(HWND hDlg, int num);
void unchecksoundtx(HWND hDlg, int num);
void unchecksoundvoI(HWND hDlg, int num);
void unchecksoundvoO(HWND hDlg, int num);
void uncheckdisp(HWND hwnd);
void checkcomport(HWND hDlg, int num);

