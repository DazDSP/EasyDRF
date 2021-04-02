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

        ptt.h

  --------------------------------------------------------------------*/


void comtx(char port);
void dotx(void);
void endtx(void);

void dorts(void);
void endrts(void);
void dodtr(void);
void enddtr(void);

char gettxport();
void settxport(char port);



