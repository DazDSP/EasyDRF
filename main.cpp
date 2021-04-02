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

#include "main.h"
#include "dialog.h"
#include "common/libs/graphwin.h"
#include "resource.h"

HINSTANCE TheInstance = 0;

char runmode = 'A';

// The main window is a modeless dialog box

//int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR cmdParam, _In_ int cmdShow)  //edited DM
{
    TheInstance = hInst;

	if (strlen(cmdParam) >= 2)
	{
		if (!strcmp(cmdParam,"-r")) runmode = 'R';
		if (!strcmp(cmdParam,"-t")) runmode = 'T';
		if (!strcmp(cmdParam,"-p")) runmode = 'P';
		if (!strcmp(cmdParam,"-R")) runmode = 'R';
		if (!strcmp(cmdParam,"-T")) runmode = 'T';
		if (!strcmp(cmdParam,"-P")) runmode = 'P';
	}

    if (!RegisterGraphClass( hInst )) return( 0 );

	HWND hDialog;
    hDialog = CreateDialog (hInst, MAKEINTRESOURCE (DLG_MAIN), NULL, DialogProc);

    if (!hDialog)
    {
        char buf [100];
        wsprintf (buf, "Error x%x", GetLastError ());
        MessageBox (0, buf, "CreateDialog", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    MSG  msg;
    while ( GetMessage (&msg, NULL, 0, 0 ) )
    {
        if (!IsDialogMessage (hDialog, &msg))
        {
            TranslateMessage ( &msg );
            DispatchMessage ( &msg );
        }
    }

    return msg.wParam;
}


