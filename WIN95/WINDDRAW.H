/******************************************************************************
 WINDDRAW.H - the window and the display modes (WINDDRAW.C, WINMAIN.C).

 Reconstructed; the names are descriptive.
******************************************************************************/
#ifndef __WINDDRAW_H
#define __WINDDRAW_H

#include <windows.h>
#include <ddraw.h>

/* WINMAIN.C */
extern HINSTANCE hInstApp;
extern HWND g_hWndMain;
extern HDC g_hWinDC;
extern HACCEL g_hAccelerators;
extern RECT WindowRect;			/* the window in DISPLAY_WINDOW, saved in mk3w.cfg */
extern int xFrame, yFrame;		/* window size minus client size */
extern int force_full_screen;		/* the desktop is not 16-bit: windowed modes cannot work */
extern int g_DIBFlags;
extern int g_DDStatus;				/* -1 no DirectDraw, 0 not in use, 1 in use */
extern HMODULE hDDrawInst_;
extern FARPROC pDirectDrawCreate;
extern HMODULE hDSound;
extern FARPROC lpDirectSoundCreate;
extern char g_InstallDir[256];
extern char g_datadir[256];			/* where the game files are (the CD) */
extern int iWinExit;

/* WINDDRAW.C */
extern LPDIRECTDRAW lpDDraw;
extern LPDIRECTDRAWSURFACE g_lpPrimary;
extern LPDIRECTDRAWSURFACE lpBackBuffer1;
extern u_char *g_DIBBits;
extern HDC g_hDIBDC1;
extern int g_PixelMode;			/* PIXFMT_555 / PIXFMT_565 */
extern RECT ClientRect1;
extern int g_SavedMode;				/* mode to return to after window / inactive */
extern int g_LastDDMode;
extern int iWinActiveState;
extern int iWinDisableESC;

int CreateFrameDIB(HINSTANCE hInst);
void DestroyFrameDIB(HINSTANCE hInst);
void DDRelease(HINSTANCE hInst);
int SetDisplayMode(int mode);
void Win95_FunctionKey(int vk);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* MEMFILL.C */
void FillWords(void *dst, u_short value, u_int bytes);
void CopyLongs(void *dst, void *src, int bytes);

#endif /* __WINDDRAW_H */
