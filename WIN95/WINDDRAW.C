/******************************************************************************
 WINDDRAW.C - display modes: the GDI window, DirectDraw full screen, F5,
 Alt+Enter, and the window procedure.

 Reconstructed from mk3w.exe (retail).

 DISPLAY_WINDOW (0) and DISPLAY_GDIFULL (2) draw into a DIB section that is
 blitted to the window. The DirectDraw modes (5-9) are full screen at
 320x240 or 640x480 in 16 bits; F5 goes through them in turn. Mode 6
 stretches a 320x240 offscreen surface to the screen; it is kept only if it
 manages ten frames a second, else mode 7 shows the game unstretched in the
 middle of the screen.
******************************************************************************/
/* retail compiled this file as C++ (its uninitialised globals are .bss definitions, not
   common symbols): the C interfaces of DirectX and C names for everything */
#define CINTERFACE
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <string.h>
#include "emulator.h"
#include "win95.h"
#include "winddraw.h"
#include "winddsnd.h"
#include "psxcd.h"

#define F5_TEXT		"Press F5 to toggle full screen modes"
#define F5_COLOR	RGB(128, 0, 0)

LPDIRECTDRAWSURFACE g_lpPrimary;
u_char *g_DIBBits;
HDC g_hDIBDC1;
int g_PixelMode;
static int mode_switching;		/* F5 */
static int g_alt_return;
static RECT g_prev_rect;
static DDBLTFX g_cls_fx1;
int g_bSizing;
int bMoving;
static HBITMAP g_hDib;
LPDIRECTDRAWSURFACE lpBackBuffer1;
LPDIRECTDRAW lpDDraw;
RECT ClientRect1;

/* WINMAIN.C's (retail defines them here: they are in this object's .bss) */
HINSTANCE hInstApp;
int g_JoyCount;
int iWinExit;
RECT WindowRect;
HACCEL g_hAccelerators;
HDC g_hWinDC;
int force_full_screen;
int ScrMode1;
int g_DIBFlags;
int iWinFPS;
HWND g_hWndMain;
int g_DDStatus;
HMODULE hDSound;
FARPROC pDirectDrawCreate;
int xFrame, yFrame;
FARPROC lpDirectSoundCreate;
HMODULE hDDrawInst_;
char g_InstallDir[256];
char g_datadir[256];
static int pad59;			/* not referred to */
static int spare10[2];		/* not referred to */

int g_SavedMode = -1;
int g_LastDDMode = 0;
int iWinActiveState = 1;
static int low_video_memory = 0;	/* not enough for two 640x480 pages */
static int can_colorkey = 0;		/* mode 8 needs a colour keyed blit */
static int can_colorfill = 0;
static int pad6a1110 = 0;
static int no_hardware = 0;
static int dd_init_done = 0;
int iWinDisableESC = 0;			/* MKOPT.C: Esc does not quit (KONTROLS screen) */
int g_bStretchBuffer = 0;
static int sysmem_buffer = 0;		/* the 320x240 surface must be in system memory */
static int closing = 0;

extern short f_music;
extern short cd_vol_tbl[];
extern int cd_playing, g_cd_looping;	/* PSXCDWIN.C */

/* the frame of the GDI modes, 320x240 16-bit (RGB555) */
int CreateFrameDIB(HINSTANCE hInst)
{
	BITMAPINFOHEADER bih;
	int size = sizeof(BITMAPINFOHEADER);

	memset(&bih, 0, size);
	EmuSetPixelFormat(PIXFMT_555);
	g_PixelMode = PIXFMT_555;
	bih.biSize = size;
	bih.biWidth = SCREEN_W;
	bih.biHeight = -SCREEN_H;
	bih.biPlanes = 1;
	bih.biBitCount = 16;
	bih.biCompression = BI_RGB;
	g_hDIBDC1 = CreateCompatibleDC(NULL);
	g_hDib = CreateDIBSection(g_hDIBDC1, (BITMAPINFO *)&bih, DIB_PAL_COLORS, (void **)&g_DIBBits, NULL, 0);
	SelectObject(g_hDIBDC1, g_hDib);
	Emu.pitch = SCREEN_W;
	Emu.height = SCREEN_H;
	Emu.pitch2 = Emu.pitch * 2;
	Emu.pitch4 = Emu.pitch * 4;
	return 1;
}

void DestroyFrameDIB(HINSTANCE hInst)
{
	if (g_hDIBDC1 != NULL)
	{
		DeleteDC(g_hDIBDC1);
		g_hDIBDC1 = NULL;
	}
	if (g_hDib != NULL)
	{
		DeleteObject(g_hDib);
		g_DIBBits = NULL;
		g_hDib = NULL;
	}
	return;
}

/* frames (lock, clear, show) drawn in one second, -1 on error */
static int DDBenchmark(int mode, u_int bytes)
{
	RECT src;
	DDSURFACEDESC ddsd;
	u_long until;
	HRESULT hr;
	RECT rect;
	DDBLTFX fx;
	int count = 0;

	for (until = timeGetTime() + 1000; timeGetTime() < until; count++)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		memset(&fx, 0, sizeof(fx));
		fx.dwSize = sizeof(fx);
		fx.dwFillColor = 0;
		hr = lpBackBuffer1->lpVtbl->Lock(lpBackBuffer1, NULL, &ddsd, DDLOCK_WAIT, NULL);
		if (hr != DD_OK)
			return -1;
		FillWords(ddsd.lpSurface, 0, bytes);
		FillWords(ddsd.lpSurface, 0, bytes);
		lpBackBuffer1->lpVtbl->Unlock(lpBackBuffer1, NULL);
		if (mode == DISPLAY_DD640STRETCH)
		{
			src.left = 0;
			src.top = 0;
			src.right = SCREEN_W;
			src.bottom = SCREEN_H;
			rect.left = 0;
			rect.top = 0;
			rect.right = 640;
			rect.bottom = 480;
			hr = g_lpPrimary->lpVtbl->Blt(g_lpPrimary, &rect, lpBackBuffer1, &src, DDBLT_WAIT, &fx);
		}
		else
			hr = g_lpPrimary->lpVtbl->Flip(g_lpPrimary, NULL, DDFLIP_WAIT);
		if (ScrMode1 == DISPLAY_DD640CENTER)
		{
			rect.left = 160;
			rect.top = 120;
			rect.right = 480;
			rect.bottom = 360;
			hr = lpBackBuffer1->lpVtbl->Blt(lpBackBuffer1, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
		}
		else
			lpBackBuffer1->lpVtbl->Blt(lpBackBuffer1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
	}
	return count;
}

/* the F5 hint at the bottom of the 640x480 screen (DDInit; a block, so every use
   has its own locals and its own copy of the string) */
#define SHOW_F5_TEXT(surf) \
	{ \
		char *text = F5_TEXT; \
		HDC hdc; \
		(surf)->lpVtbl->GetDC((surf), &hdc); \
		SetBkColor(hdc, 0); \
		SetTextColor(hdc, F5_COLOR); \
		SetTextAlign(hdc, TA_CENTER); \
		TextOut(hdc, 320, 450, text, strlen(text)); \
		(surf)->lpVtbl->ReleaseDC((surf), hdc); \
	}

/* fill the back buffer with g_cls_fx1 and wait for the blit */
#define CLEAR_BACK() \
	lpBackBuffer1->lpVtbl->Blt(lpBackBuffer1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &g_cls_fx1); \
	while (lpBackBuffer1->lpVtbl->GetBltStatus(lpBackBuffer1, DDGBS_ISBLTDONE) == DDERR_WASSTILLDRAWING) \
		;

/* go full screen in a DirectDraw mode; returns the mode set
   (which may differ from the one asked for) or -1 */
static int DDInit(HINSTANCE hInst, HWND hWnd, int mode)
{
	DDPIXELFORMAT fmt;
	DDSURFACEDESC ddsd;
	HRESULT ddrval;
	int middle = 0;						/* the 320x240 frame in the middle of 640x480 */
	int half = 0;						/* the frame is 320x240 */
	DDCAPS ddcaps;
	DDCAPS hel;
	DDSCAPS ddscaps;

	low_video_memory = 0;
	no_hardware = 0;
	g_bStretchBuffer = 0;
	can_colorfill = 0;
	can_colorkey = 0;
	if (pDirectDrawCreate == NULL)
		return -1;
	/* retail calls it as a C function and pops the arguments again after the WINAPI call:
	   harmless in this unoptimised code (the frame pointer restores esp), but not in code
	   that keeps values in registers, so other compilers get the right type */
#ifdef _MSC_VER
	ddrval = ((HRESULT (*)(GUID *, LPDIRECTDRAW *, IUnknown *))pDirectDrawCreate)(NULL, &lpDDraw, NULL);
#else
	ddrval = ((HRESULT (WINAPI *)(GUID *, LPDIRECTDRAW *, IUnknown *))pDirectDrawCreate)(NULL, &lpDDraw, NULL);
#endif
	if (ddrval == DD_OK)
	{
		memset(&ddcaps, 0, sizeof(ddcaps));
		ddcaps.dwSize = sizeof(ddcaps);
		memset(&hel, 0, sizeof(hel));
		hel.dwSize = sizeof(hel);
		lpDDraw->lpVtbl->GetCaps(lpDDraw, &ddcaps, &hel);
		ddrval = lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, hWnd,
				DDSCL_EXCLUSIVE | DDSCL_NOWINDOWCHANGES | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN);
		if (ddrval == DD_OK)
		{
			if (mode == DISPLAY_DD320)
			{
				ddrval = lpDDraw->lpVtbl->SetDisplayMode(lpDDraw, SCREEN_W, SCREEN_H, 16);
				if (ddrval != DD_OK)
				{
					ddrval = lpDDraw->lpVtbl->SetDisplayMode(lpDDraw, 640, 480, 16);
					if (ddrval != DD_OK)
					{
						ddrval = lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
						lpDDraw->lpVtbl->Release(lpDDraw);
						lpDDraw = NULL;
						return -1;
					}
					mode = DISPLAY_DD640STRETCH;
				}
				else
					half = 1;
			}
			else
				ddrval = lpDDraw->lpVtbl->SetDisplayMode(lpDDraw, 640, 480, 16);
			if (ddrval == DD_OK)
			{
				if ((mode == DISPLAY_DD320 ? SCREEN_W * SCREEN_H * 2 : 640 * 480 * 2) + ddcaps.dwVidMemTotal < 640 * 480 * 4)
				{
					low_video_memory = 1;
					if (mode > DISPLAY_DD640CENTER)
						mode = DISPLAY_DD320;
				}
				if (ddcaps.dwCaps & DDCAPS_NOHARDWARE)
				{
					no_hardware = 1;
					if (mode > DISPLAY_DD640CENTER)
						mode = DISPLAY_DD320;
				}
				if (ddcaps.dwCaps & DDCAPS_COLORKEY)
					can_colorkey = 1;
				if (ddcaps.dwCaps & DDCAPS_BLTCOLORFILL)
					can_colorfill = 1;
				memset(&ddsd, 0, sizeof(ddsd));
				ddsd.dwSize = sizeof(ddsd);

				if (mode == DISPLAY_DD640STRETCH)
				{
					ddsd.dwFlags = DDSD_CAPS;
					ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
					ddrval = lpDDraw->lpVtbl->CreateSurface(lpDDraw, &ddsd, &g_lpPrimary, NULL);
					if (ddrval == DD_OK)
					{
						fmt.dwSize = sizeof(fmt);
						g_lpPrimary->lpVtbl->GetPixelFormat(g_lpPrimary, &fmt);
						if (fmt.dwRGBBitCount <= 8)
						{
							g_lpPrimary->lpVtbl->Release(g_lpPrimary);
							g_lpPrimary = NULL;
							lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
							lpDDraw->lpVtbl->RestoreDisplayMode(lpDDraw);
							lpDDraw->lpVtbl->Release(lpDDraw);
							lpDDraw = NULL;
							return -1;
						}
						if (fmt.dwGBitMask == 0x3e0)
						{
							EmuSetPixelFormat(PIXFMT_555);
							g_PixelMode = PIXFMT_555;
						}
						else
						{
							EmuSetPixelFormat(PIXFMT_565);
							g_PixelMode = PIXFMT_565;
						}
						ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
						ddsd.dwWidth = SCREEN_W;
						ddsd.dwHeight = SCREEN_H;
						if (!sysmem_buffer
							&& (((ddcaps.dwFXCaps & DDFXCAPS_BLTSTRETCHY) && (ddcaps.dwFXCaps & DDFXCAPS_BLTSTRETCHX))
								|| ((ddcaps.dwFXCaps & DDFXCAPS_BLTSTRETCHXN) && (ddcaps.dwFXCaps & DDFXCAPS_BLTSTRETCHYN))))
							ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
						else
							ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
						half = 1;
						middle = 1;
						g_bStretchBuffer = 1;
						lpBackBuffer1 = NULL;
						ddrval = lpDDraw->lpVtbl->CreateSurface(lpDDraw, &ddsd, &lpBackBuffer1, NULL);
						if (ddrval != DD_OK || (DDBenchmark(DISPLAY_DD640STRETCH, SCREEN_W * SCREEN_H * 2) < 10 && !dd_init_done))
						{
							if (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
							{
								/* try the surface in system memory */
								if (lpBackBuffer1 != NULL)
								{
									lpBackBuffer1->lpVtbl->Release(lpBackBuffer1);
									lpBackBuffer1 = NULL;
								}
								ddsd.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
								ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
								ddrval = lpDDraw->lpVtbl->CreateSurface(lpDDraw, &ddsd, &lpBackBuffer1, NULL);
								if (ddrval != DD_OK)
								{
									g_lpPrimary->lpVtbl->Release(g_lpPrimary);
									g_lpPrimary = NULL;
									lpBackBuffer1 = NULL;
									mode = DISPLAY_DD640CENTER;
									half = 0;
									middle = 0;
								}
								else if (DDBenchmark(DISPLAY_DD640STRETCH, SCREEN_W * SCREEN_H * 2) < 10)
								{
									lpBackBuffer1->lpVtbl->Release(lpBackBuffer1);
									lpBackBuffer1 = NULL;
									g_lpPrimary->lpVtbl->Release(g_lpPrimary);
									g_lpPrimary = NULL;
									mode = DISPLAY_DD640CENTER;
									half = 0;
									middle = 0;
								}
								else
									sysmem_buffer = 1;
							}
							else
							{
								/* too slow: no stretching */
								if (lpBackBuffer1 != NULL)
								{
									lpBackBuffer1->lpVtbl->Release(lpBackBuffer1);
									lpBackBuffer1 = NULL;
								}
								g_lpPrimary->lpVtbl->Release(g_lpPrimary);
								g_lpPrimary = NULL;
								mode = DISPLAY_DD640CENTER;
								half = 0;
								middle = 0;
							}
						}
					}
					else
					{
						lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
						lpDDraw->lpVtbl->RestoreDisplayMode(lpDDraw);
						lpDDraw->lpVtbl->Release(lpDDraw);
						lpDDraw = NULL;
						return -1;
					}
				}
				if (mode == DISPLAY_DD640CENTER && middle)
					SHOW_F5_TEXT(g_lpPrimary)

				if (mode != DISPLAY_DD640STRETCH && !middle)
				{
					/* a primary surface with one back buffer to flip */
					DDCOLORKEY ck;

					if (mode == DISPLAY_DD640DOUBLE && !can_colorkey)
						mode = DISPLAY_DD640LINES;
					ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
					ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
					ddsd.dwBackBufferCount = 1;
					ddrval = lpDDraw->lpVtbl->CreateSurface(lpDDraw, &ddsd, &g_lpPrimary, NULL);
					if (ddrval == DD_OK)
					{
						fmt.dwSize = sizeof(fmt);
						g_lpPrimary->lpVtbl->GetPixelFormat(g_lpPrimary, &fmt);
						if (fmt.dwRGBBitCount <= 8)
						{
							g_lpPrimary->lpVtbl->Release(g_lpPrimary);
							g_lpPrimary = NULL;
							lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
							lpDDraw->lpVtbl->RestoreDisplayMode(lpDDraw);
							lpDDraw->lpVtbl->Release(lpDDraw);
							lpDDraw = NULL;
							return -1;
						}
						if (fmt.dwGBitMask == 0x3e0)
						{
							EmuSetPixelFormat(PIXFMT_555);
							g_PixelMode = PIXFMT_555;
						}
						else
						{
							EmuSetPixelFormat(PIXFMT_565);
							g_PixelMode = PIXFMT_565;
						}
						ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
						ddrval = g_lpPrimary->lpVtbl->GetAttachedSurface(g_lpPrimary, &ddscaps, &lpBackBuffer1);
						ck.dwColorSpaceLowValue = 0;
						ck.dwColorSpaceHighValue = 0;
						lpBackBuffer1->lpVtbl->SetColorKey(lpBackBuffer1, DDCKEY_SRCBLT, &ck);
					}
					else
					{
						lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
						lpDDraw->lpVtbl->RestoreDisplayMode(lpDDraw);
						lpDDraw->lpVtbl->Release(lpDDraw);
						lpDDraw = NULL;
						return -1;
					}
					memset(&g_cls_fx1, 0, sizeof(g_cls_fx1));
					g_cls_fx1.dwSize = sizeof(g_cls_fx1);
					g_cls_fx1.dwFillColor = 0;
					/* clear both pages (and put the F5 text on them in mode 7) */
					CLEAR_BACK()
					g_lpPrimary->lpVtbl->Flip(g_lpPrimary, NULL, DDFLIP_WAIT);
					CLEAR_BACK()
					if (mode == DISPLAY_DD640CENTER)
						SHOW_F5_TEXT(lpBackBuffer1)
					g_lpPrimary->lpVtbl->Flip(g_lpPrimary, NULL, DDFLIP_WAIT);
					CLEAR_BACK()
					if (mode == DISPLAY_DD640CENTER)
						SHOW_F5_TEXT(lpBackBuffer1)
				}
			}
			else
			{
				ddrval = lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
				lpDDraw->lpVtbl->RestoreDisplayMode(lpDDraw);
				lpDDraw->lpVtbl->Release(lpDDraw);
				lpDDraw = NULL;
				return -1;
			}
		}
		else
		{
			ddrval = lpDDraw->lpVtbl->SetCooperativeLevel(lpDDraw, g_hWndMain, DDSCL_NORMAL);
			lpDDraw->lpVtbl->Release(lpDDraw);
			lpDDraw = NULL;
			return -1;
		}
	}
	else
		return -1;
	if (half)
	{
		Emu.pitch = SCREEN_W;
		Emu.height = SCREEN_H;
	}
	else
	{
		Emu.pitch = 640;
		Emu.height = 480;
	}
	Emu.pitch2 = Emu.pitch * 2;
	Emu.pitch4 = Emu.pitch << 2;
	g_LastDDMode = mode;
	dd_init_done = 1;
	return mode;
}

void DDRelease(HINSTANCE hInst)
{
	if (lpDDraw != NULL)
	{
		lpDDraw->lpVtbl->FlipToGDISurface(lpDDraw);
		if (lpBackBuffer1 != NULL)
		{
			lpBackBuffer1->lpVtbl->Release(lpBackBuffer1);
			lpBackBuffer1 = NULL;
		}
		if (g_lpPrimary != NULL)
		{
			g_lpPrimary->lpVtbl->Release(g_lpPrimary);
			g_lpPrimary = NULL;
		}
		lpDDraw->lpVtbl->Release(lpDDraw);
		lpDDraw = NULL;
	}
	return;
}

/* change the display mode; returns the mode set */
int SetDisplayMode(int mode)
{
	LONG style = GetWindowLong(g_hWndMain, GWL_STYLE);
	RECT r;
	int w, h;

	SetCursor(NULL);
	if (mode >= DISPLAY_DD320 && pDirectDrawCreate == NULL)
	{
		force_full_screen = 0;
		if (g_DIBFlags & 4)
			mode = DISPLAY_WINDOW;
		else
			mode = DISPLAY_GDIFULL;
	}
	if (mode <= DISPLAY_GDIFULL)
	{
		CreateFrameDIB(hInstApp);
		if (g_DDStatus > 0)
			DDRelease(hInstApp);
		g_DDStatus = 0;
	}
	if (g_DDStatus > 0)
	{
		if (mode == DISPLAY_DD320 || ScrMode1 == DISPLAY_DD320
			|| mode == DISPLAY_DD640STRETCH || ScrMode1 == DISPLAY_DD640STRETCH
			|| ((mode > DISPLAY_DD640CENTER || ScrMode1 > DISPLAY_DD640CENTER)
				&& (low_video_memory || no_hardware)))
		{
			/* a different surface layout: start DirectDraw again */
			DDRelease(hInstApp);
			if (mode > DISPLAY_DD640CENTER && (low_video_memory || no_hardware))
				mode = DISPLAY_DD320;
			mode = DDInit(hInstApp, g_hWndMain, mode);
			if (mode == -1)
			{
				force_full_screen = 0;
				g_DDStatus = 0;
				return SetDisplayMode(DISPLAY_WINDOW);
			}
			ScrMode1 = mode;
		}
		else
		{
			/* 7, 8 and 9 share the 640x480 flipping surfaces */
			if (mode == DISPLAY_DD640DOUBLE && !can_colorkey)
				mode = DISPLAY_DD640LINES;
			ScrMode1 = mode;
		}
		return ScrMode1;
	}
	if (mode >= DISPLAY_DD320 && g_DDStatus == 0)
	{
		r.right = GetSystemMetrics(SM_CXSCREEN);
		w = r.right;
		r.bottom = GetSystemMetrics(SM_CYSCREEN);
		h = r.bottom;
		style &= ~WS_OVERLAPPEDWINDOW;
		style |= WS_VISIBLE;
		SetWindowLong(g_hWndMain, GWL_STYLE, style);
		SetMenu(g_hWndMain, NULL);
		SetWindowPos(g_hWndMain, HWND_TOPMOST, 0, 0, w, h, SWP_FRAMECHANGED | SWP_NOREDRAW);
		mode = DDInit(hInstApp, g_hWndMain, mode);
		if (mode == -1)
		{
			force_full_screen = 0;
			g_DDStatus = 0;
			return SetDisplayMode(DISPLAY_WINDOW);
		}
		ScrMode1 = mode;
		g_DDStatus = 1;
		DestroyFrameDIB(hInstApp);
		return ScrMode1;
	}
	if (force_full_screen)
	{
		/* windowed cannot work on this desktop: minimise */
		ScrMode1 = mode;
		style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		style |= WS_VISIBLE | WS_SYSMENU;
		SetWindowLong(g_hWndMain, GWL_STYLE, style);
		if (!IsIconic(g_hWndMain))
			PostMessage(g_hWndMain, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		return ScrMode1;
	}
	if (mode == DISPLAY_GDIFULL)
	{
		ScrMode1 = mode;
		GetWindowRect(GetDesktopWindow(), &r);
		w = r.right;
		h = r.bottom;
		style &= ~(WS_VISIBLE | WS_OVERLAPPEDWINDOW);
		SetWindowLong(g_hWndMain, GWL_STYLE, style);
		SetMenu(g_hWndMain, NULL);
		SetWindowPos(g_hWndMain, HWND_TOPMOST, 0, 0, w, h, SWP_FRAMECHANGED);
		return ScrMode1;
	}
	ScrMode1 = DISPLAY_WINDOW;
	w = WindowRect.right - WindowRect.left;
	h = WindowRect.bottom - WindowRect.top;
	style &= ~WS_BORDER;
	style |= WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	SetWindowLong(g_hWndMain, GWL_STYLE, style);
	SetWindowPos(g_hWndMain, HWND_NOTOPMOST, WindowRect.left, WindowRect.top, w, h, SWP_FRAMECHANGED);
	return ScrMode1;
}

/* Esc quits (unless the KONTROLS screen is up), F5 tries the
   next full screen mode */
void Win95_FunctionKey(int vk)
{
	int mode;

	if (vk == VK_ESCAPE)
	{
		if (!iWinDisableESC)
			PostMessage(g_hWndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
		return;
	}
	if (!mode_switching)
	{
		if (ScrMode1 < 2)
			return;
		mode_switching = 1;
		mode = ScrMode1;
		mode++;
		if (mode < 2)
			mode = DISPLAY_DD640LINES;
		if (mode > DISPLAY_DD640LINES)
			mode = DISPLAY_DD320;
		if (mode >= 3 && mode <= 4)
			mode = DISPLAY_DD320;
		if (pDirectDrawCreate == NULL && mode >= DISPLAY_DD320)
			mode = DISPLAY_WINDOW;
		SetDisplayMode(mode);
		mode_switching = 0;
	}
	return;
}

/* back to full screen after the window */
/* back to the full screen mode used last (Alt+Enter, maximise) */
#define TO_FULL_SCREEN() \
	if (g_SavedMode >= 2) \
		SetDisplayMode(g_SavedMode); \
	else \
		SetDisplayMode(DISPLAY_DD320);

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT paint;
	BYTE keys[256];
	WINDOWPOS *wp2;
	WINDOWPOS *pos;
	RECT *r;
	MINMAXINFO *mmi;

	switch (msg)
	{
		case WM_CREATE:
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_CLOSE)
			{
				g_prev_rect = WindowRect;
				closing = 1;
			}
			if ((wParam & 0xfff0) == SC_MOVE)
				bMoving = 1;
			if ((wParam & 0xfff0) == SC_SIZE)
				g_bSizing = 1;
			if ((wParam & 0xf030) == SC_MAXIMIZE)
			{
				/* maximise = full screen */
				bMoving = 0;
				g_bSizing = 0;
				if (ScrMode1 < 2)
					TO_FULL_SCREEN()
				return 0;
			}
			break;

		case WM_SYSKEYUP:
			switch (wParam)
			{
				case VK_MENU:
					return 0;

				case VK_RETURN:
					/* Alt+Enter */
					if (g_alt_return)
						return 0;
					g_alt_return = 1;
					if (ScrMode1 < 2)
						TO_FULL_SCREEN()
					else
					{
						g_SavedMode = ScrMode1;
						SetDisplayMode(DISPLAY_WINDOW);
					}
					g_alt_return = 0;
					break;
			}
			break;

		case WM_GETMINMAXINFO:
			mmi = (MINMAXINFO *)lParam;
			mmi->ptMinTrackSize.x = xFrame + SCREEN_W;
			mmi->ptMinTrackSize.y = yFrame + SCREEN_H;
			if (ScrMode1 == DISPLAY_WINDOW)
			{
				mmi->ptMaxTrackSize.x = xFrame + 640;
				mmi->ptMaxTrackSize.y = yFrame + 480;
			}
			return 0;

		case WM_SIZING:
			/* keep the client area 4:3 */
			r = (RECT *)lParam;
			if (wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT)
				r->bottom = r->top + ((r->right - r->left - xFrame) * SCREEN_H) / SCREEN_W + yFrame;
			else if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT)
				r->left = (r->right - xFrame) - ((r->bottom - r->top - yFrame) * SCREEN_W) / SCREEN_H;
			else
				r->right = r->left + ((r->bottom - r->top - yFrame) * SCREEN_W) / SCREEN_H + xFrame;
			return 0;

		case WM_WINDOWPOSCHANGING:
			/* the window snaps to 1x and 2x */
			pos = (WINDOWPOS *)lParam;
			if (ScrMode1 == DISPLAY_WINDOW && (g_bSizing || bMoving) && !(pos->flags & SWP_NOSIZE))
			{
				if (pos->cx <= SCREEN_W + 5 + xFrame || pos->cy <= SCREEN_H + 5 + yFrame)
				{
					pos->cx = xFrame + SCREEN_W;
					pos->cy = yFrame + SCREEN_H;
				}
				if (pos->cx >= xFrame + 635 || pos->cy >= yFrame + 475)
				{
					pos->cx = xFrame + 640;
					pos->cy = yFrame + 480;
				}
			}
			break;

		case WM_WINDOWPOSCHANGED:
			wp2 = (WINDOWPOS *)lParam;
			if (ScrMode1 == DISPLAY_WINDOW && (g_bSizing || bMoving)
				&& (!(wp2->flags & SWP_NOMOVE) || !(wp2->flags & SWP_NOSIZE)))
			{
				WindowRect.left = wp2->x;
				WindowRect.top = wp2->y;
				WindowRect.right = wp2->cx + WindowRect.left;
				WindowRect.bottom = WindowRect.top + wp2->cy;
			}
			GetClientRect(hWnd, &ClientRect1);
			break;

		case WM_EXITSIZEMOVE:
			bMoving = 0;
			g_bSizing = 0;
			break;

		case WM_SETCURSOR:
			if (ScrMode1 != DISPLAY_WINDOW)
			{
				SetCursor(NULL);
				return 1;
			}
			break;

		case WM_ACTIVATEAPP:
			if (closing)
				break;
			iWinActiveState = wParam;
			if (!iWinActiveState)
			{
				/* switching away: back to the window, CD paused */
				if (g_DDStatus > 0 || ScrMode1 == DISPLAY_GDIFULL)
				{
					g_SavedMode = ScrMode1;
					SetDisplayMode(DISPLAY_WINDOW);
				}
				if (cd_playing || g_cd_looping)
					psxcd_pause();
			}
			else
			{
				if (IsIconic(hWnd))
					SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
				/* NumLock on: player 1 plays on the numeric keypad */
				GetKeyboardState(keys);
				keys[VK_NUMLOCK] = (BYTE)iWinActiveState;
				SetKeyboardState(keys);
				if (force_full_screen && g_SavedMode >= 0 && ScrMode1 != g_SavedMode)
				{
					if (g_SavedMode < 5)
						g_SavedMode = 5;
					SetDisplayMode(g_SavedMode);
				}
				psxcd_restart(cd_vol_tbl[f_music]);
			}
			Win95_SoundActivate(iWinActiveState);
			break;

		case WM_ERASEBKGND:
			return 0;

		case WM_PAINT:
			BeginPaint(hWnd, &paint);
			if (ScrMode1 < 3 && ScrMode1 >= 0 && !closing)
			{
				GetClientRect(hWnd, &ClientRect1);
				if (ClientRect1.right == SCREEN_W && ClientRect1.bottom == SCREEN_H)
					BitBlt(paint.hdc, 0, 0, SCREEN_W, SCREEN_H, g_hDIBDC1, 0, 0, SRCCOPY);
				else
					StretchBlt(paint.hdc, 0, 0, ClientRect1.right, ClientRect1.bottom,
							   g_hDIBDC1, 0, 0, SCREEN_W, SCREEN_H, SRCCOPY);
			}
			EndPaint(hWnd, &paint);
			break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifdef __cplusplus
}
#endif
