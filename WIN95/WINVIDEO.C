/******************************************************************************
 WINVIDEO.C - the frame: locking it for the PSX emulation, clearing it,
 showing it, the message pump and the frame rate statistics.

 Reconstructed from mk3w.exe (retail).

 In the GDI modes (< 3) the frame is a 320x240 DIB section blitted to the
 window. In the DirectDraw modes it is the back surface, flipped or blitted
 to the primary surface.
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
#include "emulator.h"
#include "win95.h"
#include "winddraw.h"

#define MAX_PUMP_DEPTH	10

int f_block_flip;

/* the names of the statics give retail's .bss order (Visual C++'s name hash % 1024) */
static MSG msgs[MAX_PUMP_DEPTH];
static DDBLTFX fillfx;
static DDBLTFX clearfx;
static DDSURFACEDESC surfdesc;
static long tsum[10];				/* per slot, summed by Win95_LogTime */
static long t_count[10];
static DDBLTFX offsetfx;
static DDBLTFX show_fx;
static char fps_text[256];
/* initialised (in retail's .data) */
static long draw_total = 0, draw_count = 1;		/* frames drawn */
static long frame_total = 0, frame_count = 0;	/* all frames */
static long last_fps_time = 0;
static int pump_depth = -1;					/* Win95_Pump's nesting, 0 = outermost */

/* the frame to draw into, NULL if it cannot be locked */
u_char *Win95_LockFrame(void)
{
	HRESULT hr;

	if (ScrMode1 < 3)
		return g_DIBBits;
	else
	{
		surfdesc.dwSize = sizeof(surfdesc);
		hr = lpBackBuffer1->lpVtbl->Lock(lpBackBuffer1, NULL, &surfdesc, DDLOCK_WAIT, NULL);
		if (hr != DD_OK)
			return NULL;
		return (u_char *)surfdesc.lpSurface;
	}
}

void Win95_UnlockFrame(u_char *frame)
{
	if (ScrMode1 >= DISPLAY_DD320)
		lpBackBuffer1->lpVtbl->Unlock(lpBackBuffer1, frame);
	return;
}

/* mode 8 doubles the lines by copying the frame one line down
   onto itself (black, the colour key, is not copied) */
static void Win95_OffsetBlt(void)
{
	long timeStart = timeGetTime();
	RECT dst, src;
	HRESULT ddrval;

	dst.left = 0;
	dst.top = 1;
	dst.right = 640;
	dst.bottom = 480;
	src.left = 0;
	src.top = 0;
	src.right = 640;
	src.bottom = 479;
	offsetfx.dwSize = sizeof(offsetfx);
	ddrval = lpBackBuffer1->lpVtbl->BltFast(lpBackBuffer1, 0, 1, lpBackBuffer1, &src, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT);
	if (iWinFPS)
		Win95_LogTime("Offset Blt time", timeStart, timeGetTime(), 4);
	return;
}

/* the whole frame is filled whatever the rectangle */
/* emulator.h declares int r, g, b (the callers pass ints); retail reads bytes here.
   Visual C++ only warns (C4028) about the mismatch; other compilers refuse it. */
#ifdef _MSC_VER
void Win95_FillRect(u_char r, u_char g, u_char b, int x, int y, int w, int h)
#else
void Win95_FillRect(int r, int g, int b, int x, int y, int w, int h)
#endif
{
	u_short color;
	RECT rect;
	HRESULT ddrval;

	if (g_PixelMode == PIXFMT_555)
		color = ((g & 0xf8) << 2) | ((b & 0xf8) >> 3) | ((r & 0xf8) << 7);
	else
		color = ((b & 0xf8) >> 3) | ((g & 0xfc) << 3) | ((r & 0xf8) << 8);
	if (ScrMode1 < 3)
		FillWords(g_DIBBits, color, Emu.height * Emu.pitch * 2);
	else
	{
		fillfx.dwSize = sizeof(fillfx);
		fillfx.dwFillColor = color;
		if (CENTER_MODE())
		{
			rect.left = 160;
			rect.top = 120;
			rect.right = 480;
			rect.bottom = 360;
			ddrval = lpBackBuffer1->lpVtbl->Blt(lpBackBuffer1, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fillfx);
		}
		else
			ddrval = lpBackBuffer1->lpVtbl->Blt(lpBackBuffer1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fillfx);
	}
	return;
}

/* the statistics line main() shows when iWinFPS is set */
char *Win95_FPS(int skip_cnt, int skipped)
{
	long elapsed, drawTime, frameTime;
	long avg[10];
	int t;

	if (iWinFPS)
	{
		elapsed = timeGetTime() - last_fps_time;
		frame_total += elapsed;
		if (++frame_count > 300)
		{
			frame_count = 1;
			frame_total = elapsed;
		}
		draw_total += elapsed;
		if (!skipped && ++draw_count > 100)
		{
			draw_count = 1;
			draw_total = elapsed;
		}
		if ((drawTime = draw_total / draw_count) == 0)
			drawTime = 1;
		if ((frameTime = frame_total / frame_count) == 0)
			frameTime = 1;
		for (t = 0; t < 6; t++)
		{
			if (t_count[t] == 0)
				t_count[t] = 1;
			avg[t] = tsum[t] / t_count[t];
		}
		wsprintf(fps_text, "M:%ld TICK:%ld FPS:%ld,%ld C:%ld L:%ld FL:%ld FI:%ld B:%ld W:%ld",
				 ScrMode1, 1000 / frameTime, 1000 / drawTime, drawTime,
				 avg[1], avg[0], avg[2], avg[3], avg[4], avg[5]);
		last_fps_time = timeGetTime();
	}
	else
		fps_text[0] = 0;
	return fps_text;
}

/* add a timing to a slot (average of the last 100) */
int Win95_LogTime(char *what, long start, long end, int slot)
{
	long dt = end - start;

	tsum[slot] += dt;
	if (++t_count[slot] > 100)
	{
		t_count[slot] = 1;
		tsum[slot] = dt;
	}
	return 0;
}

/* main() (MKINIT.C) keeps this time in a float to pace the game at
   60 Hz: once Windows has been up for more than about three days (2^28 ms) the float
   cannot hold 16 ms steps and the game runs too fast. MK3_FIX_FLOAT_TIMER (not in
   the retail build) counts from the first call instead. */
long Win95_timeGetTime(void)
{
#ifdef MK3_FIX_FLOAT_TIMER
	static DWORD start;

	if (start == 0)
		start = timeGetTime() - 1;
	return timeGetTime() - start;
#else
	return timeGetTime();
#endif
}

/* handle the waiting messages; returns 1 when the game must
   quit. Keys go straight to the pad emulation, F5 and Esc to
   Win95_FunctionKey. */
int Win95_Pump(void)
{
	pump_depth++;
	if (pump_depth < MAX_PUMP_DEPTH)
	{
		while (PeekMessage(&msgs[pump_depth], NULL, 0, 0, PM_REMOVE))
		{
			if (msgs[pump_depth].message == WM_QUIT)
			{
				iWinExit = 1;
				return 1;
			}
			if (msgs[pump_depth].message == WM_KEYDOWN)
			{
				Win95_KeyEvent(1, msgs[pump_depth].wParam, msgs[pump_depth].lParam);
				continue;
			}
			else if (msgs[pump_depth].message == WM_KEYUP)
			{
				if (msgs[pump_depth].wParam == VK_F5 || msgs[pump_depth].wParam == VK_ESCAPE)
				{
					Win95_FunctionKey(msgs[pump_depth].wParam);
					continue;
				}
				Win95_KeyEvent(0, msgs[pump_depth].wParam, msgs[pump_depth].lParam);
				continue;
			}
			DispatchMessage(&msgs[pump_depth]);
		}
	}
	pump_depth--;
	return 0;
}

/* show the finished frame */
void Win95_Blast_Frame(void)
{
	long begin;
	long time;
	RECT src, dstRect;
	HRESULT hr;

	if (ScrMode1 < 3)
	{
		begin = timeGetTime();
		if (ClientRect1.right == SCREEN_W && ClientRect1.bottom == SCREEN_H)
			BitBlt(g_hWinDC, 0, 0, SCREEN_W, SCREEN_H, g_hDIBDC1, 0, 0, SRCCOPY);
		else
			StretchBlt(g_hWinDC, 0, 0, ClientRect1.right, ClientRect1.bottom,
					   g_hDIBDC1, 0, 0, SCREEN_W, SCREEN_H, SRCCOPY);
		if (iWinFPS)
			Win95_LogTime("Blt time", begin, timeGetTime(), 4);
	}
	else if (ScrMode1 != DISPLAY_DD640STRETCH && !f_block_flip && !g_bStretchBuffer)
	{
		if (ScrMode1 == DISPLAY_DD640DOUBLE)
		{
			Win95_OffsetBlt();
			while (lpBackBuffer1->lpVtbl->GetBltStatus(lpBackBuffer1, DDGBS_ISBLTDONE) == DDERR_WASSTILLDRAWING)
				;
		}
		hr = g_lpPrimary->lpVtbl->Flip(g_lpPrimary, NULL, DDFLIP_WAIT);
	}
	else
	{
		time = timeGetTime();
		if (ScrMode1 >= DISPLAY_DD640DOUBLE)
		{
			src.left = 0;
			src.top = 0;
			src.right = 640;
			src.bottom = 480;
			dstRect.left = 0;
			dstRect.top = 0;
			dstRect.right = 640;
			dstRect.bottom = 480;
		}
		else if (ScrMode1 == DISPLAY_DD640CENTER)
		{
			if (g_bStretchBuffer)
			{
				src.left = 0;
				src.top = 0;
				src.right = SCREEN_W;
				src.bottom = SCREEN_H;
				dstRect.left = 160;
				dstRect.top = 120;
				dstRect.right = 480;
				dstRect.bottom = 360;
			}
			else
			{
				src.left = 0;
				src.top = 0;
				src.right = 640;
				src.bottom = 480;
				dstRect.left = 0;
				dstRect.top = 0;
				dstRect.right = 640;
				dstRect.bottom = 480;
			}
		}
		else if (ScrMode1 == DISPLAY_DD640STRETCH)
		{
			src.left = 0;
			src.top = 0;
			src.right = SCREEN_W;
			src.bottom = SCREEN_H;
			dstRect.left = 0;
			dstRect.top = 0;
			dstRect.right = 640;
			dstRect.bottom = 480;
		}
		else
		{
			src.left = 0;
			src.top = 0;
			src.right = SCREEN_W;
			src.bottom = SCREEN_H;
			dstRect.left = 0;
			dstRect.top = 0;
			dstRect.right = SCREEN_W;
			dstRect.bottom = SCREEN_H;
		}
		show_fx.dwSize = sizeof(show_fx);
		hr = g_lpPrimary->lpVtbl->Blt(g_lpPrimary, &dstRect, lpBackBuffer1, &src, DDBLT_WAIT, &show_fx);
		if (iWinFPS)
			Win95_LogTime("Strecth Blt time", time, timeGetTime(), 4);
	}
	return;
}

/* not called */
void Win95_ClearSurface(u_long color)
{
	long start = timeGetTime();
	HRESULT hr;

	if (ScrMode1 >= DISPLAY_DD320)
	{
		clearfx.dwSize = sizeof(clearfx);
		clearfx.dwFillColor = color;
		hr = lpBackBuffer1->lpVtbl->Blt(lpBackBuffer1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &clearfx);
		if (iWinFPS)
			Win95_LogTime("Clear time", start, timeGetTime(), 3);
	}
	return;
}

/* debug output (process_dispatch) */
void Win95_OUT(char *text)
{
	OutputDebugString(text);
	return;
}

#ifdef __cplusplus
}
#endif
