/******************************************************************************
 EMULATOR.H - state of the PlayStation emulation in the Windows 95 version.

 Reconstructed from mk3w.exe (retail). The game includes "emulator.h" and uses
 Emu.kludgeflags and Emu.ConvertPalettePSX; the other members are the
 emulation's own and are named here after what they hold.
******************************************************************************/
#ifndef __EMULATOR_H
#define __EMULATOR_H

#include "psxlib.h"

#define SCREEN_W		320
#define SCREEN_H		240

/* VRAM: the 1024x512 16-bit PSX frame buffer is kept as 16 pages of
   128x256 VRAM pixels (64 KB each), page = x/128 + 8*(y/256). Pages 0-4
   (x < 640) are the PSX display buffers and are never allocated. */
#define VRAM_PAGES		16
#define VRAM_PAGE_SIZE	0x10000
#define VRAM_FIRST_PAGE	5

/* CLUTs: VRAM rows 240-260, converted to the display's pixel format */
#define CLUT_BASE_Y		240
#define CLUT_MEM_SIZE	0xa400

/* pixel formats of the display surface */
#define PIXFMT_555		555
#define PIXFMT_565		565

/* Emu.kludgeflags: a full screen 8-bit picture is drawn over the frame */
#define KLUDGE_DRAWPIC	0x01	/* KludgePic in player_heap (Load8BitPicture) */
#define KLUDGE_DRAWFLIC	0x02	/* KludgeFlic in player_heap (play_flic_and_sound) */

typedef void (*CONVERTPAL)(u_short *src, u_short *dst, int count);

typedef struct {
	u_short *vram16;			/* 1024-wide 16-bit VRAM (only used by VramLoad16/VramFill16) */
	u_char *frame;				/* locked frame surface, NULL when not locked */
	int pitch;					/* frame row length in pixels */
	int pitch2;					/* pitch * 2 */
	int pitch4;					/* pitch * 4 */
	int height;					/* frame height in rows */
	int drawtpage;				/* texture page set by the last DR_MODE */
	int kludgeflags;			/* KLUDGE_* */
	u_char *vram[VRAM_PAGES];	/* VRAM pages */
	u_short *clut;				/* CLUT memory, biased so that clut + id * 16 is CLUT id */
	CONVERTPAL ConvertPalettePSX;	/* PSX BGR555 to the display pixel format */
	DISPENV *dispenv;
} EMULATOR;

extern EMULATOR Emu;
extern int gCyrax;				/* MKFATAL.C: set during Cyrax's helicopter fatality */

/* full screen picture shown by the attract screens and the options menu
   (a BMP read into player_heap by Load8BitPicture) */
typedef struct {
	char path[8];				/* the path is built here first, then overwritten */
	u_short *psxclut;			/* the BMP palette converted to PSX colours */
	u_short *pclut;				/* palette in the display format (points to clut[]) */
	u_char *pixels;				/* bottom-up 320x240 8-bit pixels */
	u_short clutcount;			/* fadepal header: colour count (256) */
	u_short clutpad;
	u_short clut[256];
	/* BITMAPINFOHEADER, colour table and pixels follow */
} KludgePic;

/* PSXEMU.C */
int EmuInit(void);
void EmuFree(void);
void EmuSetPixelFormat(int fmt);
void ConvertPSXto555(u_short *src, u_short *dst, int count);
void ConvertPSXto565(u_short *src, u_short *dst, int count);
u_char *VramAddress(int tpage, int u, int v);
u_short *ClutAddress(u_short clut);
void LoadMk3Palette(u_short *src, int count, int x, int y);

/* POLYSCAL.C */
void ScaleQuad(u_short *clut, int tpage, int u, int v, int du, int dv, int x, int y, int dx, int dy);
void ScaleQuadWide(u_short *clut, int tpage, int u, int v, int du, int dv, int x, int y, int dx, int dy);

/* BLIT.ASM: copy 8-bit texels through a CLUT, colour 0 transparent.
   h rows of w texels; after each row dst += dstskip and src += srcskip bytes. */
void Blit8(int h, int w, u_short *clut, u_char *src, u_char *dst, int dstskip, int srcskip);
void Blit8Wide(int h, int w, u_short *clut, u_char *src, u_char *dst, int dstskip, int srcskip);
void Blit8Mirror(int h, int w, u_short *clut, u_char *src, u_char *dst, int dstskip, int srcskip);
void Blit8WideMirror(int h, int w, u_short *clut, u_char *src, u_char *dst, int dstskip, int srcskip);
void VramLoad16(u_char *src, int x, int y, int wbytes, int h);
void VramFill16(u_short color, int x, int y, int w, int h);
void DoubleScanlines(u_char *frame);

/* WINVIDEO.C */
extern int f_block_flip;		/* set by the game while it freezes the screen: blit, do not flip */
extern int iWinFPS;				/* collect the timings shown by Win95_FPS (never set) */
#define g_bShowTimes	iWinFPS

/* WINDDRAW.C */
extern int ScrMode1;		/* DISPLAY_* */
extern int g_bStretchBuffer;	/* mode 7 draws into a 320x240 surface and stretches it */
u_char *Win95_LockFrame(void);
void Win95_UnlockFrame(u_char *frame);
#ifdef __cplusplus		/* WINVIDEO.C, compiled as C++ in retail: the definition's types */
void Win95_FillRect(u_char r, u_char g, u_char b, int x, int y, int w, int h);
#else
void Win95_FillRect(int r, int g, int b, int x, int y, int w, int h);	/* the definition has u_char r, g, b (VC warns C4028) */
#endif
int Win95_LogTime(char *, long, long, int);
long Win95_timeGetTime(void);
void Win95_Blast_Frame(void);

#define DISPLAY_WINDOW		0	/* "GDI StretchBlt/BitBtl": in a window */
#define DISPLAY_GDIFULL		2	/* "GDI StretchBlt Full Screen" */
#define DISPLAY_DD320		5	/* "DDraw 320x240 16Bit Color" */
#define DISPLAY_DD640STRETCH 6	/* "DDraw 640x480 DDraw Stretch": 320x240 surface stretched */
#define DISPLAY_DD640CENTER	7	/* 640x480, the game in the middle (not in the menu) */
#define DISPLAY_DD640DOUBLE	8	/* "DDraw 640x480 Transparent Stretch": lines doubled by a keyed blit */
#define DISPLAY_DD640LINES	9	/* "DDraw 640x480 Even Scanlines" */

#define WIDE_MODE()		(ScrMode1 == DISPLAY_DD640DOUBLE || ScrMode1 == DISPLAY_DD640LINES)
#define CENTER_MODE()	(ScrMode1 == DISPLAY_DD640CENTER && !g_bStretchBuffer)
/* byte offset of the centred 320x240 area in a 640x480 frame */
#define CENTER_OFFSET()	(Emu.pitch * 240 + 320)

extern u_char player_heap[];	/* MKGLOBAL.C */

/* Visual C++ 2.0 picks registers by the order its symbols were declared in, so the
   number of declarations here (not their names) shapes the game's code: the retail
   code needs 3 symbols fewer than the declarations above would give.
   Win95_LogTime's unnamed parameters take 4 away; the
   declaration below adds 1. */
void Win95_ClearSurface(u_long);

#endif /* __EMULATOR_H */
