/******************************************************************************
 PSXEMU.C - PlayStation GPU, pad and counter calls emulated on Windows 95.

 Reconstructed from mk3w.exe (retail). Function names
 are the Sony ones where the game calls them; the others are descriptive.

 The game draws through Sony-style ordering tables. Primitives are rendered
 in software straight into the locked display surface (Emu.frame), 16-bit
 pixels in the format chosen by EmuSetPixelFormat. Textures are 8-bit and come
 from the emulated VRAM pages; CLUTs are kept converted to the display format.
 The "wide" handlers are used by the 640x480 doubled modes: every texel is
 written twice and only every other line is drawn (Emu.pitch2 per row).
******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "emulator.h"
#include "win95.h"
#include "flic/flicfile.h"

/* gCyrax belongs to MKFATAL.C; retail allocates it with this module's common symbols (just
   before Emu), so this file declared it too */
int gCyrax;
EMULATOR Emu;

/* not static: retail allocates them with Emu among the common symbols at the end of .bss, in
   ascending order of Visual C++'s name hash % 1024 (the names are fitted to that order) */
DRAWENV *pdrawenv1;					/* set by SetDefDrawEnv */
DRAWENV drawenv1;					/* the current drawing environment */
int g_display_on;						/* SetDispMask */
long composite_begin;				/* time the first primitive of an OT was drawn */
long composite_end;
int last_fill_width;
static int pixel_format = 0;		/* initialised: retail has it in .data */
/* two initialised variables nothing refers to (in retail's .data): a zero and 256
   halfwords of 15; their names and use are not known */
static int unused_zero = 0;
static u_short unused_fifteen[256] = {
#define F16 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
	F16, F16, F16, F16, F16, F16, F16, F16, F16, F16, F16, F16, F16, F16, F16, F16
#undef F16
};
static int clut_max = 0;
static int clut_min = 0x0fffffff;

static int AllocVramPage(u_char **page)
{
	return (int)(*page = malloc(VRAM_PAGE_SIZE));
}

static void FreeVramPage(u_char **page)
{
	if (*page != NULL)
	{
		free(*page);
		*page = NULL;
	}
}

/* address of texel (u,v) of a texture page */
u_char *VramAddress(int tpage, int u, int v)
{
	return &Emu.vram[(tpage & 0x1e) >> 1][u + v * 256];
}

/* PSX colours (BGR555) to RGB555 */
void ConvertPSXto555(u_short *src, u_short *dst, int count)
{
	u_short c;

	for (; count > 0; count--)
	{
		c = *src++;
		*dst++ = ((c & 0x1f) << 10) | ((c & 0x7c00) >> 10) | (c & 0x3e0);
	}
}

/* PSX colours (BGR555) to RGB565 */
void ConvertPSXto565(u_short *src, u_short *dst, int count)
{
	u_short c;

	for (; count > 0; count--)
	{
		c = *src++;
		*dst++ = (((c << 10) | (c & 0x3e0)) << 1) | ((c & 0x7c00) >> 10);
	}
}

static void Convert565to555(u_short *p, int count)
{
	u_short c;

	for (; count != 0; count--)
	{
		c = *p;
		*p++ = ((c & 0xffc1) >> 1) | (c & 0x1f);
	}
}

static void Convert555to565(u_short *p, int count)
{
	u_short c;

	for (; count != 0; count--)
	{
		c = *p;
		*p++ = ((c & 0xffe0) << 1) | (c & 0x1f);
	}
}

/* the display surface format changed; convert the loaded CLUTs */
void EmuSetPixelFormat(int fmt)
{
	if (fmt == PIXFMT_555)
	{
		Emu.ConvertPalettePSX = ConvertPSXto555;
		if (pixel_format == PIXFMT_565)
		{
			Convert565to555((u_short *)&((u_char *)Emu.clut)[CLUT_BASE_Y * 2048], CLUT_MEM_SIZE / 2);
			pixel_format = fmt;
			return;
		}
	}
	else
	{
		Emu.ConvertPalettePSX = ConvertPSXto565;
		if (pixel_format == PIXFMT_555)
			Convert555to565((u_short *)&((u_char *)Emu.clut)[CLUT_BASE_Y * 2048], CLUT_MEM_SIZE / 2);
	}
	pixel_format = fmt;
}

void EmuFree(void)
{
	u_char **page;

	for (page = &Emu.vram[VRAM_FIRST_PAGE]; page < &Emu.vram[VRAM_PAGES]; page++)
		FreeVramPage(page);
	if (Emu.clut != NULL)
		free(&((u_char *)Emu.clut)[CLUT_BASE_Y * 2048]);
	Emu.clut = NULL;
}

/* called by WinMain before the game starts */
int EmuInit(void)
{
	u_char **page;

	g_display_on = 1;
	Emu.frame = NULL;
	Emu.clut = malloc(CLUT_MEM_SIZE);
	if (Emu.clut != NULL)
	{
		Emu.clut = (u_short *)((u_char *)Emu.clut - CLUT_BASE_Y * 2048);
		for (page = &Emu.vram[VRAM_FIRST_PAGE]; AllocVramPage(page); )
		{
			if (++page >= &Emu.vram[VRAM_PAGES])
			{
				Emu.ConvertPalettePSX = ConvertPSXto555;
				pixel_format = 0;
				ResetGraph(0);
				return 1;
			}
		}
	}
	EmuFree();
	return 0;
}

/* address of a CLUT given its PSX id */
u_short *ClutAddress(u_short clut)
{
	if (clut > clut_max)
		clut_max = clut;
	if (clut < clut_min)
		clut_min = clut;
	return &Emu.clut[clut * 16];
}

/* transfer_palettes (MKPAL.C) */
void LoadMk3Palette(u_short *src, int count, int x, int y)
{
	u_short *dst = &Emu.clut[y * 1024 + x];

	Emu.ConvertPalettePSX(src, dst, count);
}

int PadInit(int mode)
{
	return 1;
}

/* keyboard bits, plus the joysticks found at start-up */
u_long PadRead(int id)
{
	u_long bits = key_pad_bits;

	if (g_JoyCount != 0)
	{
		bits |= ReadJoystick(0);
		if (g_JoyCount == 2)
			bits |= ReadJoystick(1);
	}
	return bits;
}

void EnterCriticalSection_PSX(void)
{
}

void ExitCriticalSection_PSX(void)
{
}

/* a PSX library stub that nothing calls (the game's calls are in code the
   Windows build leaves out), so its name is not known */
int PsxStub_402e10(void)
{
	return 1;
}

/* a PSX library stub that nothing calls (the game's calls are in code the
   Windows build leaves out), so its name is not known */
int PsxStub_402e20(void)
{
	return 1;
}

void ResetCallback(void)
{
}

int ResetGraph(int mode)
{
	u_char **page;

	for (page = Emu.vram; page < &Emu.vram[VRAM_PAGES]; page++)
		if (*page != NULL)
			memset(*page, 0xff, VRAM_PAGE_SIZE);
	return 0;
}

void SetGraphDebug(int level)
{
}

DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h)
{
	env->clip.x = x;
	env->clip.y = y;
	env->clip.w = w;
	env->clip.h = h;
	env->r0 = 0;
	env->ofs[0] = x;
	env->g0 = 0;
	env->ofs[1] = y;
	env->b0 = 0;
	env->dtd = 1;
	env->dfe = 0;
	pdrawenv1 = env;
	return env;
}

DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h)
{
	env->disp.x = x;
	env->disp.y = y;
	env->disp.w = w;
	env->disp.h = h;
	Emu.dispenv = env;
	return env;
}

DRAWENV *PutDrawEnv(DRAWENV *env)
{
	drawenv1 = *pdrawenv1;
	return env;
}

/* copies the drawing environment too, as in the original */
DISPENV *PutDispEnv(DISPENV *env)
{
	drawenv1 = *pdrawenv1;
	return env;
}

/* psxlib.h (and so the game's calls) has an int tpage; the definition had a
   u_short one (retail loads 16 bits). Visual C++ only warns (C4028) about the
   mismatch; other compilers refuse it. */
#ifdef _MSC_VER
void SetDrawMode(DR_MODE *p, int dfe, int dtd, u_short tpage, RECT_PSX *tw)
#else
void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT_PSX *tw)
#endif
{
	p->tpage = tpage;
	p->tag.tag = NULL;
	p->tag.len = 0x10;
	p->code = PRIM_DR_MODE;
}

void SetBlockFill(BLK_FILL *p)
{
	p->tag.len = 5;
	p->code = PRIM_BLK_FILL;
}

void SetPolyF4(POLY_F4 *p)
{
	p->tag.len = 7;
	p->code = PRIM_POLY_F4;
}

void SetPolyFT4(POLY_FT4 *p)
{
	p->tag.len = 11;
	p->code = PRIM_POLY_FT4;
}

void SetSprt(SPRT *p)
{
	p->tag.len = 6;
	p->code = PRIM_SPRT;
}

/* fill a VRAM rectangle (x in VRAM pixels). The width counter is
   used up by the first row, so only the first row is filled. */
static void FillVramRect(int color, int x, int y, int w, int h)
{
	int page = x >> 7;
	int lower_h, lower_w, lower_page, lower_x;
	int n, rows, skip;
	u_char **pp;
	u_short *dst;

	x &= 0x7f;
	if (y >= 256)
	{
		y -= 256;
		page += 8;
		lower_h = 0;
	}
	else
	{
		lower_h = y + h - 256;			/* part below y = 256 */
		if (lower_h > 0)
		{
			h -= lower_h;
			lower_page = page;
			lower_w = w;
			lower_x = x;
		}
	}
	y <<= 7;
	for (;;)
	{
		if (w > 0)
		{
			pp = &Emu.vram[page];
			do
			{
				dst = &((u_short *)*pp)[y + x];
				n = w + x;
				if (n > 128)
					n = 128;
				n -= x;
				w -= n;
				pp++;
				x = 0;
				if (pp >= &Emu.vram[VRAM_FIRST_PAGE + 1])
				{
					last_fill_width = n;
					skip = (128 - n) * 2;
					for (rows = h; rows > 0; rows--)
					{
						for (; n > 0; n--)
							*dst++ = color;
						dst = (u_short *)((u_char *)dst + skip);
					}
				}
			} while (w > 0);
		}
		if (lower_h <= 0)
			break;
		x = lower_x;
		y = 0;
		w = lower_w;
		h = lower_h;
		page = lower_page + 8;
		lower_h = 0;
	}
}

/* BLK_FILL. A fill on the screen clears the whole frame (and
   locks it for the primitives that follow); elsewhere it fills VRAM black. */
static void DrawBlockFill(BLK_FILL *p)
{
	if (p->x0 <= 640 && p->y0 < 240 && p->w <= 320)
	{
		Win95_FillRect(p->r0, p->g0, p->b0, p->x0, p->y0, p->w, p->h);
		Emu.frame = Win95_LockFrame();
		return;
	}
	FillVramRect(0, p->x0, p->y0, p->w, p->h);
}

static void DrawBlockFillWide(BLK_FILL *p)
{
	if (p->x0 <= 640 && p->y0 < 240 && p->w <= 320)
	{
		Win95_FillRect(p->r0, p->g0, p->b0, p->x0, p->y0, p->w, p->h);
		Emu.frame = Win95_LockFrame();
		return;
	}
	FillVramRect(0, p->x0, p->y0, p->w, p->h);
}

/* SPRT, texture page from the last DR_MODE */
static void DrawSprite(SPRT *p)
{
	int tpage = Emu.drawtpage;
	int u, v, x, y, w, h;
	int srcskip, dstskip;
	u_short *clut;
	u_char *src, *dst;

	x = p->x0;
	y = p->y0;
	u = p->u0;
	v = p->v0;
	h = p->h;
	w = p->w;

	if (x >= SCREEN_W)
		return;
	w += x;
	if (w > SCREEN_W)
		w = SCREEN_W;
	if (x > 0)
		w -= x;
	else
	{
		u -= x;
		x = 0;
	}
	if (w <= 0)
		return;
	if (y >= SCREEN_H)
		return;
	h += y;
	if (h > SCREEN_H)
		h = SCREEN_H;
	if (y > 0)
		h -= y;
	else
	{
		v -= y;
		y = 0;
	}
	if (h <= 0)
		return;
	clut = ClutAddress(p->clut);
	srcskip = 256 - w;
	dstskip = (Emu.pitch - w) * 2;
	src = VramAddress(tpage, u, v);
	dst = Emu.frame + (Emu.pitch * y + x) * 2;
	if (CENTER_MODE())
		dst += CENTER_OFFSET();
	Blit8(h, w, clut, src, dst, dstskip, srcskip);
}

static void DrawSpriteWide(SPRT *p)
{
	int tpage = Emu.drawtpage;
	int u, v, x, y, w, h;
	int srcskip, dstskip;
	u_short *clut;
	u_char *src, *dst;

	x = p->x0;
	y = p->y0;
	u = p->u0;
	v = p->v0;
	h = p->h;
	w = p->w;

	if (x >= SCREEN_W)
		return;
	w += x;
	if (w > SCREEN_W)
		w = SCREEN_W;
	if (x > 0)
		w -= x;
	else
	{
		u -= x;
		x = 0;
	}
	if (w <= 0)
		return;
	if (y >= SCREEN_H)
		return;
	h += y;
	if (h > SCREEN_H)
		h = SCREEN_H;
	if (y > 0)
		h -= y;
	else
	{
		v -= y;
		y = 0;
	}
	if (h <= 0)
		return;
	clut = ClutAddress(p->clut);
	srcskip = 256 - w;
	dstskip = (Emu.pitch - w) * 4;
	src = VramAddress(tpage, u, v);
	dst = Emu.frame + (Emu.pitch2 * y + x * 2) * 2;
	Blit8Wide(h, w, clut, src, dst, dstskip, srcskip);
}

/* POLY_F4, drawn as the rectangle x0..x1, y0..y2 */
static void DrawFlatQuad(POLY_F4 *p)
{
	int x = p->x0, y = p->y0;
	int w = p->x1 - x, h = p->y2 - y;
	int skip, n;
	u_short color;
	u_char *dst;

	if (pixel_format == PIXFMT_555)
		color = ((((p->r0 & 0xf8) << 5) | (p->g0 & 0xf8)) << 2) | (p->b0 >> 3);
	else
		color = (((p->g0 & 0xfc) << 3) | ((p->r0 & 0xf8) << 8)) | (p->b0 >> 3);
	if (y >= SCREEN_H)
		return;
	h += y;
	if (h > SCREEN_H)
		h = SCREEN_H;
	if (y > 0)
		h -= y;
	else
		y = 0;
	if (h <= 0)
		return;
	if (x >= SCREEN_W)
		return;
	w += x;
	if (w > SCREEN_W)
		w = SCREEN_W;
	if (x > 0)
		w -= x;
	else
		x = 0;
	if (w <= 0)
		return;
	dst = Emu.frame + (Emu.pitch * y + x) * 2;
	skip = (Emu.pitch - w) * 2;
	if (CENTER_MODE())
		dst += CENTER_OFFSET();
	while (--h >= 0)
	{
		for (n = w; n > 0; n--)
		{
			*(u_short *)dst = color;
			dst += 2;
		}
		dst += skip;
	}
}

static void DrawFlatQuadWide(POLY_F4 *p)
{
	int x, y, w, h;
	int skip, n;
	u_short color;
	u_char *dst;

	x = p->x0;
	w = p->x1 - x;
	y = p->y0;
	h = p->y2 - y;

	if (pixel_format == PIXFMT_555)
		color = ((((p->r0 & 0xf8) << 5) | (p->g0 & 0xf8)) << 2) | (p->b0 >> 3);
	else
		color = (((p->g0 & 0xfc) << 3) | ((p->r0 & 0xf8) << 8)) | (p->b0 >> 3);
	if (y >= SCREEN_H)
		return;
	h += y;
	if (h > SCREEN_H)
		h = SCREEN_H;
	if (y > 0)
		h -= y;
	else
		y = 0;
	if (h <= 0)
		return;
	if (x >= SCREEN_W)
		return;
	w += x;
	if (w > SCREEN_W)
		w = SCREEN_W;
	if (x > 0)
		w -= x;
	else
		x = 0;
	if (w <= 0)
		return;
	dst = Emu.frame + (Emu.pitch2 * y + x * 2) * 2;
	skip = (Emu.pitch - w) * 4;
	while (--h >= 0)
	{
		for (n = w * 2; n > 0; n--)
		{
			*(u_short *)dst = color;
			dst += 2;
		}
		dst += skip;
	}
}

/* POLY_FT4. An unscaled quad (texture size = screen size) is
   blitted directly, mirrored if x1 < x0 and upside down if y2 < y0; a
   scaled one goes to ScaleQuad unless it is upside down. */
static void DrawTexQuad(POLY_FT4 *p)
{
	int dy;
	int u = p->u0, v = p->v0;
	int dv = p->v2 - v, du = p->u1 - u;
	int x1 = p->x1, x0 = p->x0, dx = x1 - x0;
	int y = p->y0;
	int flipv, skip, srcskip;
	u_short *clut;
	u_char *src, *dst;

	if (p->y2 < p->y0)
	{
		flipv = 1;
		dy = y - p->y2;
	}
	else
	{
		flipv = 0;
		dy = p->y2 - y;
	}
	clut = ClutAddress(p->clut);
	if (dx < 0)
	{
		/* mirrored: drawn from x0 back to x1 */
		dx = -dx;
		if (du != dx || dv != dy)
		{
			if (!flipv)
			{
				ScaleQuad(clut, p->tpage, u, v, du, dv, x1, y, -dx, dy);
				return;
			}
		}
		if (flipv)
		{
			/* upside down: drawn from y0 up */
			if (gCyrax != 1)
				dy++;
			if (y < 0)
				return;
			if (y >= SCREEN_H)
			{
				v += y - SCREEN_H;
				dy = dy - y + SCREEN_H;
				y = SCREEN_H;
			}
			if (y - dy <= 0)
				dy = y + 1;
		}
		else
		{
			if (y >= SCREEN_H)
				return;
			dy += y;
			if (dy > SCREEN_H)
				dy = SCREEN_H;
			if (y > 0)
				dy -= y;
			else
			{
				v -= y;
				y = 0;
			}
			if (dy <= 0)
				return;
		}
		u++;
		if (x1 >= SCREEN_W)
			return;
		dx += x1;
		if (dx > SCREEN_W)
		{
			u += dx - SCREEN_W;
			dx = SCREEN_W;
		}
		if (x1 > 0)
			dx -= x1;
		else
			x1 = 0;
		if (dx <= 0)
			return;
		src = VramAddress(p->tpage, u, v);
		srcskip = 256 - dx;
		dst = Emu.frame + (Emu.pitch * y + dx + x1) * 2 - 2;
		if (CENTER_MODE())
			dst += CENTER_OFFSET();
		skip = flipv ? (dx - Emu.pitch) * 2 : (Emu.pitch + dx) * 2;
		Blit8Mirror(dy, dx, clut, src, dst, skip, srcskip);
		return;
	}
	if (du != dx || dv != dy)
	{
		if (!flipv)
		{
			ScaleQuad(clut, p->tpage, u, v, du, dv, x0, y, dx, dy);
			return;
		}
	}
	if (flipv)
	{
		/* upside down: drawn from y0 up */
		if (gCyrax != 1)
			dy++;
		if (y < 0)
			return;
		if (y >= SCREEN_H)
		{
			v += y - SCREEN_H;
			dy = dy - y + SCREEN_H;
			y = SCREEN_H;
		}
		if (y - dy <= 0)
			dy = y + 1;
	}
	else
	{
		if (y >= SCREEN_H)
			return;
		dy += y;
		if (dy > SCREEN_H)
			dy = SCREEN_H;
		if (y > 0)
			dy -= y;
		else
		{
			v -= y;
			y = 0;
		}
		if (dy <= 0)
			return;
	}
	if (x0 >= SCREEN_W)
		return;
	dx += x0;
	if (dx > SCREEN_W)
		dx = SCREEN_W;
	if (x0 > 0)
		dx -= x0;
	else
	{
		u -= x0;
		x0 = 0;
	}
	if (dx <= 0)
		return;
	src = VramAddress(p->tpage, u, v);
	srcskip = 256 - dx;
	dst = Emu.frame + (Emu.pitch * y + x0) * 2;
	if (CENTER_MODE())
		dst += CENTER_OFFSET();
	skip = flipv ? -(Emu.pitch + dx) * 2 : (Emu.pitch - dx) * 2;
	Blit8(dy, dx, clut, src, dst, skip, srcskip);
}

static void DrawTexQuadWide(POLY_FT4 *p)
{
	int dy;
	int u = p->u0, v = p->v0;
	int dv = p->v2 - v, du = p->u1 - u;
	int x1 = p->x1, x0 = p->x0, dx = x1 - x0;
	int y = p->y0;
	int flipv, skip, srcskip;
	u_short *clut;
	u_char *src, *dst;

	if (p->y2 < p->y0)
	{
		flipv = 1;
		dy = y - p->y2;
	}
	else
	{
		flipv = 0;
		dy = p->y2 - y;
	}
	clut = ClutAddress(p->clut);
	if (dx < 0)
	{
		/* mirrored: drawn from x0 back to x1 */
		dx = -dx;
		if (du != dx || dv != dy)
		{
			if (!flipv)
			{
				ScaleQuadWide(clut, p->tpage, u, v, du, dv, x1, y, -dx, dy);
				return;
			}
		}
		if (flipv)
		{
			/* upside down: drawn from y0 up */
			if (gCyrax != 1)
				dy++;
			if (y < 0)
				return;
			if (y >= SCREEN_H)
			{
				v += y - SCREEN_H;
				dy = dy - y + SCREEN_H;
				y = SCREEN_H;
			}
			if (y - dy <= 0)
				dy = y + 1;
		}
		else
		{
			if (y >= SCREEN_H)
				return;
			dy += y;
			if (dy > SCREEN_H)
				dy = SCREEN_H;
			if (y > 0)
				dy -= y;
			else
			{
				v -= y;
				y = 0;
			}
			if (dy <= 0)
				return;
		}
		u++;
		if (x1 >= SCREEN_W)
			return;
		dx += x1;
		if (dx > SCREEN_W)
		{
			u += dx - SCREEN_W;
			dx = SCREEN_W;
		}
		if (x1 > 0)
			dx -= x1;
		else
			x1 = 0;
		if (dx <= 0)
			return;
		src = VramAddress(p->tpage, u, v);
		srcskip = 256 - dx;
		dst = Emu.frame + (Emu.pitch2 * y + (x1 + dx) * 2) * 2 - 4;
		skip = flipv ? (dx - Emu.pitch) * 4 : (Emu.pitch + dx) * 4;
		Blit8WideMirror(dy, dx, clut, src, dst, skip, srcskip);
		return;
	}
	if (du != dx || dv != dy)
	{
		if (!flipv)
		{
			ScaleQuadWide(clut, p->tpage, u, v, du, dv, x0, y, dx, dy);
			return;
		}
	}
	if (flipv)
	{
		/* upside down: drawn from y0 up */
		if (gCyrax != 1)
			dy++;
		if (y < 0)
			return;
		if (y >= SCREEN_H)
		{
			v += y - SCREEN_H;
			dy = dy - y + SCREEN_H;
			y = SCREEN_H;
		}
		if (y - dy <= 0)
			dy = y + 1;
	}
	else
	{
		if (y >= SCREEN_H)
			return;
		dy += y;
		if (dy > SCREEN_H)
			dy = SCREEN_H;
		if (y > 0)
			dy -= y;
		else
		{
			v -= y;
			y = 0;
		}
		if (dy <= 0)
			return;
	}
	if (x0 >= SCREEN_W)
		return;
	dx += x0;
	if (dx > SCREEN_W)
		dx = SCREEN_W;
	if (x0 > 0)
		dx -= x0;
	else
	{
		u -= x0;
		x0 = 0;
	}
	if (dx <= 0)
		return;
	src = VramAddress(p->tpage, u, v);
	srcskip = 256 - dx;
	dst = Emu.frame + (Emu.pitch2 * y + x0 * 2) * 2;
	skip = flipv ? -(Emu.pitch + dx) * 4 : (Emu.pitch - dx) * 4;
	Blit8Wide(dy, dx, clut, src, dst, skip, srcskip);
}

static void DrawPrimitiveWide(SPRT *p)
{
	switch (p->code)
	{
		case PRIM_BLK_FILL:
			DrawBlockFillWide((BLK_FILL *)p);
			return;
		case PRIM_POLY_F4:
			DrawFlatQuadWide((POLY_F4 *)p);
			return;
		case PRIM_POLY_FT4:
			DrawTexQuadWide((POLY_FT4 *)p);
			return;
		case PRIM_SPRT:
			DrawSpriteWide(p);
			return;
		case PRIM_DR_MODE:
			Emu.drawtpage = ((DR_MODE *)p)->tpage;
	}
}

static void DrawPrimitive(SPRT *p)
{
	switch (p->code)
	{
		case PRIM_BLK_FILL:
			DrawBlockFill((BLK_FILL *)p);
			return;
		case PRIM_POLY_F4:
			DrawFlatQuad((POLY_F4 *)p);
			return;
		case PRIM_POLY_FT4:
			DrawTexQuad((POLY_FT4 *)p);
			return;
		case PRIM_SPRT:
			DrawSprite(p);
			return;
		case PRIM_DR_MODE:
			Emu.drawtpage = ((DR_MODE *)p)->tpage;
	}
}

P_TAG *ClearOTag(P_TAG *ot, int n)
{
	int i;

	for (i = 0; i < n - 1; i++)
	{
		ot[i].tag = (u_long *)&ot[i + 1];
		ot[i].len = 0;
	}
	ot[n - 1].tag = NULL;
	ot[n - 1].len = 0;
	return ot;
}

void AddPrim(void *ot, void *p)
{
	((P_TAG *)p)->tag = ((P_TAG *)ot)->tag;
	((P_TAG *)ot)->tag = p;
}

void DrawPrim(void *p)
{
	if (!g_display_on)
		return;
	if (((SPRT *)p)->code != PRIM_BLK_FILL && (Emu.frame = Win95_LockFrame()) == NULL)
		return;
	if (WIDE_MODE())
		DrawPrimitiveWide(p);
	else
		DrawPrimitive(p);
	Win95_UnlockFrame(Emu.frame);
}

/* Returns 1 when nothing could be drawn. */
static int DrawOTagNormal(P_TAG *ot)
{
	P_TAG *p;

	if (!g_display_on)
		return g_display_on;
	if ((p = (P_TAG *)ot->tag) == NULL)
		return 1;
	composite_begin = 0;
	while (p != NULL)
	{
		if (p->len != 0)
			DrawPrimitive((SPRT *)p);
		if (Emu.frame == NULL)
			return 1;
		p = (P_TAG *)p->tag;
		if (composite_begin == 0)
			composite_begin = Win95_timeGetTime();
	}
	composite_end = Win95_timeGetTime();
	Win95_UnlockFrame(Emu.frame);
	if (g_bShowTimes)
		Win95_LogTime("Compositing time", composite_begin, composite_end, 1);
	return 0;
}

static int DrawOTagDoubled(P_TAG *ot)
{
	P_TAG *p;

	if (!g_display_on)
		return g_display_on;
	if ((p = (P_TAG *)ot->tag) == NULL)
		return 1;
	composite_begin = 0;
	while (p != NULL)
	{
		if (p->len != 0)
			DrawPrimitiveWide((SPRT *)p);
		if (Emu.frame == NULL)
			return 1;
		p = (P_TAG *)p->tag;
		if (composite_begin == 0)
			composite_begin = Win95_timeGetTime();
	}
	composite_end = Win95_timeGetTime();
	Win95_UnlockFrame(Emu.frame);
	if (g_bShowTimes)
		Win95_LogTime("Compositing time", composite_begin, composite_end, 1);
	return 0;
}

/* the KludgePic over the frame (bottom-up BMP rows) */
static void DrawKludgePic(void)
{
	KludgePic *pic = (KludgePic *)player_heap;
	u_char *src, *dst;

	if (Emu.frame == NULL || pic->pclut != pic->clut)
		return;
	src = pic->pixels + SCREEN_W * (SCREEN_H - 1);
	dst = Emu.frame;
	if (CENTER_MODE())
		dst += CENTER_OFFSET();
	if (!WIDE_MODE())
		Blit8(SCREEN_H, SCREEN_W, pic->pclut, src, dst, Emu.pitch2 - 640, -640);
	else
		Blit8Wide(SCREEN_H, SCREEN_W, pic->pclut, src, dst, Emu.pitch4 - 1280, -640);
}

/* the current FLIC frame over the frame */
static void DrawKludgeFlic(void)
{
	KludgeFlic *kf = (KludgeFlic *)player_heap;
	u_char *src, *dst;

	if (Emu.frame == NULL)
		return;
	src = kf->rast.pixels;
	dst = Emu.frame;
	if (CENTER_MODE())
		dst += CENTER_OFFSET();
	if (!WIDE_MODE())
		Blit8(SCREEN_H, SCREEN_W, kf->rast.clut, src, dst, Emu.pitch2 - 640, 0);
	else
		Blit8Wide(SCREEN_H, SCREEN_W, kf->rast.clut, src, dst, Emu.pitch4 - 1280, 0);
}

/* DrawOTag; returns 0 when the frame is ready to show */
int mod_DrawOTag(void *ot)
{
	int ret;

	if (!g_display_on)
		return g_display_on;
	if (WIDE_MODE())
		ret = DrawOTagDoubled(ot);
	else
		ret = DrawOTagNormal(ot);
	if (Emu.kludgeflags & (KLUDGE_DRAWPIC | KLUDGE_DRAWFLIC))
	{
		if (Emu.kludgeflags & KLUDGE_DRAWPIC)
		{
			DrawKludgePic();
			return ret;
		}
		DrawKludgeFlic();
	}
	return ret;
}

/* clr_scrn (MKUTIL.C, which declares it void) calls this on the big stack */
void DrawOTag(void *ot)
{
	mod_DrawOTag(ot);
}

/* turning the display off shows a black frame */
void SetDispMask(int mask)
{
	g_display_on = mask;
	if (mask == 0)
	{
		CallOnBigStack(Win95_FillRect, 0, 0, 0, SCREEN_W, SCREEN_H);
		CallOnBigStack(Win95_Blast_Frame);
	}
}

void SetSemiTrans(void *p, int abe)
{
}

int DrawSync(int mode)
{
	return 0;
}

void VSync(int mode)
{
}

void VSyncCallback(void (*f)(void))
{
}

/* copy an image into the VRAM pages (x in VRAM pixels,
   wbytes bytes per row) */
static void LoadVramPages(u_char *src, int x, int y, int wbytes, int h)
{
	int page;
	int lower_h, lower_page, lower_col, n, rows, w;
	u_char *lower_src, *s, *d;
	u_char **pp;

	if (src == NULL)
		return;
	x *= 2;
	page = x >> 8;
	if (y >= 256)
	{
		y -= 256;
		page += 8;
		lower_h = 0;
	}
	else
	{
		lower_h = y + h - 256;
		if (lower_h > 0)
		{
			h -= lower_h;
			lower_col = x;
			lower_page = page;
			lower_src = &src[h * wbytes];
		}
	}
	x &= 0xff;
	y <<= 8;
	w = wbytes;
	for (;;)
	{
		if (wbytes > 0)
		{
			pp = &Emu.vram[page];
			do
			{
				d = &(*pp)[y + x];
				n = wbytes + x;
				if (n > 256)
					n = 256;
				n -= x;
				s = src;
				for (rows = h; rows > 0; rows--)
				{
					memcpy(d, s, n);
					d += 256;
					s += w;
				}
				src += n;
				wbytes -= n;
				pp++;
				x = 0;
			} while (wbytes > 0);
		}
		if (lower_h <= 0)
			break;
		wbytes = w;
		h = lower_h;
		x = lower_col;
		page = lower_page + 8;
		y = 0;
		src = lower_src;
		lower_h = 0;
	}
}

u_short LoadTPage(u_long *pix, int tp, int abr, int x, int y, int w, int h)
{
	if (tp == 2)
		w *= 2;
	LoadVramPages((u_char *)pix, x, y, w, h);
	return (((tp & 3) << 2 | (abr & 3)) << 5) | ((x >> 6) & 0xf) | ((y >> 4) & 0x10);
}

void LoadImage(RECT_PSX *rect, u_long *p)
{
	if (rect->w >= 0)
		LoadVramPages((u_char *)p, rect->x, rect->y, rect->w * 2, rect->h);
}

u_short GetTPage(int tp, int abr, int x, int y)
{
	return (((tp & 3) << 2 | (abr & 3)) << 5) | ((y >> 4) & 0x10) | ((x >> 6) & 0xf);
}

/* a PSX library stub that nothing calls (the game's calls are in code the
   Windows build leaves out), so its name is not known */
void PsxStub_4048c0(void)
{
}

/* a PSX library stub that nothing calls (the game's calls are in code the
   Windows build leaves out), so its name is not known */
int PsxStub_4048d0(void)
{
	return 0;
}

long StartRCnt(unsigned long spec)
{
	return 1;
}

long SetRCnt(unsigned long spec, unsigned short target, long mode)
{
	return 1;
}

/* a PSX library stub that nothing calls (the game's calls are in code the
   Windows build leaves out), so its name is not known */
int PsxStub_404900(void)
{
	return 0;
}
