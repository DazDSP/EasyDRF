/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 * Daz Man 2021
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
#include <stdio.h>
#include <math.h>
#include "callsign2.h"

//new callsign handling and packed data routine by DM 2021

#include "GlobalDefinitions.h"

char call[20] = { "\0" };

char * getcall()
{
	return &call[0];
}

//modified from the original code DM
void setcall(const char* thecall)
{
	int i;
	char ch;
	int capt = 0;

	for (i = 0; i < 10; i++)
	{
		ch = thecall[i];
		if ((ch >= 'A') && (ch <= 'Z'))
		{
			call[capt] = ch;
			capt++;
		}
		else if ((ch >= '0') && (ch <= '9'))
		{
			call[capt] = ch;
			capt++;
		}
		else if ((ch >= 'a') && (ch <= 'z'))
		{
			call[capt] = ch; // -('a' - 'A'); //Fold to caps disabled DM
			capt++;
		}
		else if ((ch == '-') || (ch == ' ') || (ch == '/')) //added "-" DM
		{
			call[capt] = ch;
			capt++;
		}

		if (thecall[i] == 0)
		{
			call[capt] = 0;
			return;
		}
	}
	call[capt] = 0;
}

//modified from the original code DM
BOOL callisok(void)
{
	return (TRUE); //accept any callsign DM
}

//The original WinDRM calcpacklen lib routine was affected by an invalid callsign such as are used in broadcasting (why? buffer overflow?) DM
int calcpacklen(int iNoBits) {
	/* Set the total available number of bits, byte aligned */
	return floor(((iNoBits / SIZEOF__BYTE))-3); //(in/8)-3 is the correct formula, because there are 3 bits used for CRC and Header data * 8 (ref: DataDecoder.cpp) DM
	//Floor probably isn't needed due to the use of int DM
};

