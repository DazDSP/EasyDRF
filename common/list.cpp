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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <conio.h>
#include "../dialog.h"
#include "../resource.h"
#include "../getfilenam.h"
#include "libs/mixer.h"
#include "list.h"

HWND hlbrx;
HWND hlbtx;

BOOL CALLBACK MixerDlgProc (HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	UINT item;
	int i;
    switch (message)
    {
    case WM_INITDIALOG:
		if (DRMReceiver.GetSoundInterface()->GetNumDevIn() >= 2) //edited DM to work with new sound in/out
		{
			SetDlgItemText( hwnd, IDC_SOUNDCARDNAME, "Only with Single Soundcard Systems");
			break;
		}
		else
		{
			string drivnam = DRMReceiver.GetSoundInterface()->GetDeviceNameIn(0); //edited DM
			SetDlgItemText( hwnd, IDC_SOUNDCARDNAME, drivnam.c_str());
			getmix();
		}
		hlbrx = GetDlgItem(hwnd, IDC_LISTRX);
		ListBox_ResetContent(hlbrx);
		hlbtx = GetDlgItem(hwnd, IDC_LISTTX);
		ListBox_ResetContent(hlbtx);

		item = ListBox_AddString(hlbrx,"Default (do not switch)");
		item = ListBox_AddString(hlbtx,"Default (do not switch)");

		SelectSrc("none");

		for (i=0;i<nolistitems;i++)
		{
			ListBox_AddString(hlbrx,getlist(i));
			ListBox_AddString(hlbtx,getlist(i));
		}
		SendMessage(GetDlgItem(hwnd,IDC_LISTRX),LB_SETCURSEL, rxindex, 0); 
		SendMessage(GetDlgItem(hwnd,IDC_LISTTX),LB_SETCURSEL, txindex, 0); 

		break;
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDOK:
			rxindex = (int)SendMessage(GetDlgItem(hwnd,IDC_LISTRX),LB_GETCURSEL, 0, 0);  
			if (rxindex >= 1) strcpy(rxdevice,getlist(rxindex-1));
			else rxdevice[0] = 0;
			txindex = (int)SendMessage(GetDlgItem(hwnd,IDC_LISTTX),LB_GETCURSEL, 0, 0);  
			if (txindex >= 1) strcpy(txdevice,getlist(txindex-1));
			else txdevice[0] = 0;
			savemix();
            EndDialog (hwnd, 0);
            return TRUE;
		case ID_SETDEFMIXER:
			SendMessage(GetDlgItem(hwnd,IDC_LISTRX),LB_SETCURSEL, 0, 0); 
			SendMessage(GetDlgItem(hwnd,IDC_LISTTX),LB_SETCURSEL, 0, 0); 
			rxdevice[0] = 0;
			txdevice[0] = 0;
			savemix();
			break;
		case IDC_LISTRX:
			break;
		case IDC_LISTTX:
			break;
		}
	break;
	}
    return FALSE;
}

