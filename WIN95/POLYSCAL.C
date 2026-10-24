/******************************************************************************
 POLYSCAL.C - scaled textured quads for the PSX emulation.

 Reconstructed from mk3w.exe (retail).

 A POLY_FT4 whose screen size differs from its texture size is drawn as an
 axis-aligned rectangle: a table of texture columns is built for the screen
 columns, then the screen rows are stepped through the texture rows and
 each row is drawn by ScaleSpan (BLIT.ASM).
******************************************************************************/
#include "emulator.h"

typedef struct {
	u_char *src;			/* texel (u, v) */
	u_char *dst;			/* start of the current screen row */
	u_short *ucols;			/* texture column of each screen column */
	u_short *clut;
	int pad;
	int wbytes;				/* screen row length in bytes */
	int vrow;
	int dstskip;			/* bytes from the end of one row to the start of the next */
} SCALESPAN;

void ScaleSpan(int row, SCALESPAN *ss);

/* call span() for each visible screen row y..y+dy, stepping the
   texture row by dv/dy */
static void ScaleRows(int row, int dv, int y, int dy, void (*span)(int, SCALESPAN *), SCALESPAN *ss)
{
	int err;
	unsigned end = y + dy;

	if (end > SCREEN_H)
	{
		if ((int)end < 0)
			return;
		end = SCREEN_H;
	}
	err = dy - (dv >> 1);
	while (y < 0)
	{
		for (err -= dv; err <= 0; err += dy)
			row++;
		y++;
	}
	while (y < (int)end)
	{
		span(row, ss);
		for (err -= dv; err <= 0; err += dy)
			row++;
		y++;
	}
}

/* texture column for each of n screen columns; dx < 0 runs the
   texture backwards. Columns left of the screen (x < 0) are stepped over. */
static void BuildColumnTable(int du, int n, int dx, int x, u_short *table)
{
	int err, col, step;

	if (dx < 0)
	{
		step = -1;
		dx = -dx;
		col = du;
	}
	else
	{
		col = 0;
		step = 1;
	}
	err = dx - (du >> 1);
	if (x < 0)
	{
		x = -x;
		do
		{
			for (err -= du; err <= 0; err += dx)
				col += step;
		} while (--x != 0);
	}
	for (; n > 0; n--)
	{
		*table++ = col;
		for (err -= du; err <= 0; err += dx)
			col += step;
	}
}

void ScaleQuad(u_short *clut, int tpage, int u, int v, int du, int dv, int x, int y, int dx, int dy)
{
	int sx = x, sdx = dx;
	SCALESPAN ss;
	u_short ucols[512];

	if (dx < 0)
		dx = -dx;
	if (x >= SCREEN_W)
		return;
	dx += x;
	if (dx > SCREEN_W)
		dx = SCREEN_W;
	if (x > 0)
		dx -= x;
	else
		x = 0;
	if (dx <= 0)
		return;
	BuildColumnTable(du, dx, sdx, sx, ucols);
	ss.vrow = v - 1;
	ss.src = VramAddress(tpage, u, v);
	ss.wbytes = dx * 2;
	ss.dst = Emu.frame + x * 2;
	if (y > 0)
		ss.dst += y * Emu.pitch * 2;
	if (CENTER_MODE())
		ss.dst += CENTER_OFFSET();
	ss.ucols = ucols;
	ss.clut = clut;
	ss.dstskip = (Emu.pitch - dx) * 2;
	ScaleRows(0, dv, y, dy, ScaleSpan, &ss);
}

/* twice the columns, every other screen row */
void ScaleQuadWide(u_short *clut, int tpage, int u, int v, int du, int dv, int x, int y, int dx, int dy)
{
	int sx, sdx;
	SCALESPAN ss;
	u_short ucols[1024];

	dx *= 2;
	x *= 2;
	sx = x;
	sdx = dx;
	if (dx < 0)
		dx = -dx;
	if (x >= SCREEN_W * 2)
		return;
	dx += x;
	if (dx > SCREEN_W * 2)
		dx = SCREEN_W * 2;
	if (x > 0)
		dx -= x;
	else
		x = 0;
	if (dx <= 0)
		return;
	BuildColumnTable(du, dx, sdx, sx, ucols);
	ss.vrow = v - 1;
	ss.src = VramAddress(tpage, u, v);
	ss.wbytes = dx * 2;
	ss.dst = Emu.frame + x * 2;
	if (y > 0)
		ss.dst += y * Emu.pitch2 * 2;
	ss.ucols = ucols;
	ss.clut = clut;
	ss.dstskip = (Emu.pitch2 - dx) * 2;
	ScaleRows(0, dv, y, dy, ScaleSpan, &ss);
}
