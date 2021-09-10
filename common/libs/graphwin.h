/******************************************************************************\
 * Copyright (c) 2004-2005
 *
 * Author(s):
 *	Francesco Lanza
 *
 * Description:
 *	
 *
 ******************************************************************************/

/*--------------------------------------------------------------------

        GRAPHWIN.H

  --------------------------------------------------------------------*/

extern BOOL IsRX;
extern int disptype;
extern int newdata;
extern int specarr[530];
extern int specarrim[530];
extern int specarrlen;
extern int DCFreq;
extern int level;
extern char robmode;
extern int specocc;
extern int messtate[20]; //edited DM was 10


/*--------------------------------------------------------------------
        PROGRAM FUNCTIONS
  --------------------------------------------------------------------*/

BOOL RegisterGraphClass ( HINSTANCE hInstance );

LRESULT WINAPI Graph_WndProc (
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

void Graph_Init ( HWND hWnd );

