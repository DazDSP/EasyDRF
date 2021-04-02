/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 *
 * Description:
 *	
 *
 ******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "callsign.h"

char call[20] = "NOCALL";

char * getcall()
{
	return &call[0];
}

void setcall(const char * thecall)
{
	int i;
	char ch;
	int capt = 0;

	for (i=0;i<10;i++)
	{
		ch = thecall[i];
		if ((ch >= 'A') && (ch <= 'Z'))
		{
			call[capt] = ch;
			capt++;
		}
		else if ((ch >= '0') && (ch <= '9'))
		{
			call[capt] = ch ;
			capt++;
		}
		else if ((ch >= 'a') && (ch <= 'z'))
		{
			call[capt] = ch - ('a' - 'A');
			capt++;
		}
		else if ((ch == ' ') || (ch == '/'))
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


BOOL callisok(void)
{
	int k;
	BOOL hasnum = FALSE;
	BOOL hasalph = FALSE;
	char ch;
	for (k=0;k<(int)strlen(call);k++)
	{
		ch = call[k];
		if ((ch >= 'A') && (ch <= 'Z')) hasalph = TRUE;
		if ((ch >= '0') && (ch <= '9')) hasnum = TRUE;
	}
	return (hasalph && hasnum);
}


int calcpacklen(int iNoBits)
{
	double fmsc;
	// Set Stream Length
	fmsc = floor(iNoBits / 8.0);
	return (int)fmsc - 3;
}


