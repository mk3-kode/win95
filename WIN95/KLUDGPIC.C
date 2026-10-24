/******************************************************************************
 KLUDGPIC.C - full screen pictures (the attract mode logos, the options
 screens): a 320x240 8-bit BMP drawn over the frame by mod_DrawOTag while
 Emu.kludgeflags has KLUDGE_DRAWPIC.

 Reconstructed from mk3w.exe (retail).

 The file is read into the KludgePic (player_heap) after its fixed part.
 Its colour table is converted in place to PSX colours (psxclut); pclut
 holds them in the display format. Both have the two-word header fadepal
 expects in front of them, so MKFADE.C can fade the picture.
******************************************************************************/
/* retail compiled this file as C++ (its uninitialised globals are .bss definitions, not
   common symbols): the C interfaces of DirectX and C names for everything */
#define CINTERFACE
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include "emulator.h"
#include "win95.h"
#include "psxcd.h"

void update_kpic_clut(KludgePic *pic)
{
	Emu.ConvertPalettePSX(pic->psxclut, pic->pclut, 256);
	return;
}

static void LoadKludgePic(char *name, KludgePic *pic)
{
	BITMAPFILEHEADER bf;
	KludgePic *kp = pic;
	int fd;
	int i;
	long *info;
	u_short *dst;
	u_char *rgb;

	kp->pclut = NULL;
	BuildFullPath(pic->path, name);
	fd = open(pic->path, O_BINARY);
	if (fd == -1)
		goto close_file;
	if ((unsigned)read(fd, &bf, sizeof(bf)) < sizeof(bf))
		goto close_file;
	bf.bfSize -= sizeof(bf);
	dst = kp->clut;
	info = (long *)&dst[256];
	if (read(fd, info, bf.bfSize) < (int)bf.bfSize)
		goto close_file;
	rgb = &((u_char *)info)[info[0]];
	kp->psxclut = (u_short *)rgb;
	kp->pixels = &((u_char *)info)[bf.bfOffBits - sizeof(bf)];
	kp->pclut = dst;
	((u_long *)dst)[-1] = 256;			/* fadepal header: count 256 */
	dst = (u_short *)rgb;
	((u_long *)dst)[-1] = 256;
	for (i = 0; i < 256; i++)
	{
		/* RGBQUAD to PSX BGR555 */
		*dst = ((rgb[1] & 0xf8) << 2) | (rgb[2] >> 3) | ((rgb[0] & 0xf8) << 7);
		rgb += 4;
		dst++;
	}
	update_kpic_clut(kp);
close_file:
	if (fd != -1)
		close(fd);
	return;
}

void Load8BitPicture(char *name, KludgePic *pic)
{
	psxcd_stop();
	CallOnBigStack(LoadKludgePic, name, pic);
	return;
}

#ifdef __cplusplus
}
#endif
