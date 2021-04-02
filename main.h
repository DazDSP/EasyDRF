
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

#if !defined MAIN_H
#define MAIN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern HINSTANCE TheInstance;
extern char runmode;

// procedures called by Windows

//int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow);

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInst, _In_ LPSTR cmdParam, _In_ int cmdShow); //edited DM

#endif
