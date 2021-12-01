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

/*--------------------------------------------------------------------

        ptt.h - nope, it's settings.h! DM

  --------------------------------------------------------------------*/

#include "Parameter.h"
extern RECT WindowPosition;
extern int	WindowX;
extern int	WindowY;

extern int disptype;
extern BOOL rxaudfilt;
extern BOOL rtsonfac;
extern BOOL dtronfac;
extern BOOL fastreset;
extern int ECCmode;

void comtx(char port);
void dotx(void);
void endtx(void);

void dorts(void);
void endrts(void);
void dodtr(void);
void enddtr(void);

void savevar(void);
void getvar(void);

char gettxport();
void settxport(char port);

ERobMode getmode();
CParameter::ECodScheme getqam();
CParameter::ESymIntMod getinterleave();
ESpecOcc getspec();
void setmodeqam(ERobMode themode, CParameter::ECodScheme theqam, CParameter::ESymIntMod theinterl, ESpecOcc thespecocc);

// char 'r' for rx.
int getsoundin(char rtx);
void setsoundin(int dev, char rtx);

int getsoundout(char rtx);
void setsoundout(int dev, char rtx);

void settext(BOOL ison);
BOOL gettext(void);


