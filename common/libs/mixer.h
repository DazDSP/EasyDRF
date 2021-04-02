/******************************************************************************\
 * Copyright (c) 2004
 *
 * Author(s):
 *	Francesco Lanza
 *
 * Description:
 *	mixer.h
 *
 ******************************************************************************/
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

extern char rxdevice[];
extern char txdevice[];
extern int rxindex;
extern int txindex;
extern int nolistitems;

char * getlist(int devnum);

void savemix(void);
void getmix(void);

void SetMixerValues(int numdev);

int SelectSrc(const char *SrcName);




