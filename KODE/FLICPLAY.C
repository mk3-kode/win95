#include "win_port.h"

/* INCLUDES */
#include "system.h"

#ifdef SONY_PSX
#include "mksony.h"
#endif

#include "mkbkgd.h"
#include "mkobj.h"
#include "mkamode.h"
#include "mkos.h"
#include "mkgame.h"
#include "mkutil.h"
#include "emulator.h"
#include "bigstack.h"

#include "../flic/flicfile.h"


static int read_flic_frame(KludgeFlic *kf)
{
int err;
u_short *pclut;
u_short *pclutmax;
FlicRGB *palette;

	if((err = CallOnBigStack(FlicReadUncompFrame,&kf->flic,
							  &kf->rast,kf->decomp_buff)) >= 0)
	{
		pclut = kf->rast.clutpsx;
		pclutmax = (u_short *)OPTR(pclut,(sizeof(u_short)*0xff));

		palette = kf->rast.palette;

		while(pclut < pclutmax)
		{	
			*pclut = PSX_RGB(palette->r,palette->g,palette->b);
			++palette;
			++pclut;
		}
		Emu.ConvertPalettePSX(kf->rast.clutpsx, kf->rast.clut, 256);
	}
	return(err);
}

void close_kludgeflic()
{
KludgeFlic *kf = (KludgeFlic *)player_heap;

    if(Emu.kludgeflags & KLUDGE_DRAWFLIC)
	{
		Emu.kludgeflags = 0;
		CallOnBigStack(FlicClose,&kf->flic);
	}
}
void play_flic_and_sound(char *path, char *sound)
{
KludgeFlic *kf = (KludgeFlic *)player_heap;
int i;

	PsxSoundLoadLogo();

	object_initialize();  // just to make sure everything is cleared out

	// set up kludgey flic

	kf->rast.bpr     = 320;
	kf->rast.rect.x  = 0;
	kf->rast.rect.y  = 0;
	kf->rast.rect.w  = 320;
	kf->rast.rect.h  = 240;
	kf->rast.clut    = (void *)(kf + 1);
	kf->rast.clutpsx = OPTR(kf->rast.clut,(sizeof(u_short) * 0x0ff));
	kf->rast.palette = OPTR(kf->rast.clutpsx,(sizeof(u_short) * 0x0ff));
	kf->rast.pixels  = OPTR(kf->rast.palette,(sizeof(FlicRGB) * 0x0ff));
	kf->decomp_buff  = (FlicFrame *)OPTR(kf->rast.pixels,320 * 240);
	
	BuildFullPath(kf->decomp_buff,path);


 	if(CallOnBigStack(FlicOpen,kf->decomp_buff,&kf->flic,0) < 0)
		goto error;

	if(CallOnBigStack(FlicSeekFirst,&kf->flic) < 0)
		goto error;
	
	// clear screen for first time
	process_sleep(30);

	// read flic twice. first one without blitting, second one setting
	// kludgeflic flag.  The first pass should get the file in the windows disk
	// cache which seems to work ok on my 32 meg machine.

	i = 2;
	Emu.kludgeflags |= KLUDGE_DRAWFLIC;
	for(;;)
	{
static int firsttime;
		if(read_flic_frame(kf) < 0)
			goto error;


if ( !firsttime )
{
	firsttime = 1;	
	triple_sound(0xa3);
}

		if(++i > kf->flic.hdr.frame_count) // just did ring frame back to first
		{
			if(Emu.kludgeflags & KLUDGE_DRAWFLIC)
				break;
		
#if 0			
			i = 2;
			if(CallOnBigStack(FlicSeekSecond,&kf->flic) < 0)
				goto error;
			Emu.kludgeflags |= KLUDGE_DRAWFLIC;
			triple_sound(0xa3);
#endif

		}
		if(Emu.kludgeflags & KLUDGE_DRAWFLIC)		
			process_sleep(5);
	 	else
		{
			if(GetKeyNowait() != 0)
				break;
			if(JoyGetNative(0))
				break;
			if(JoyGetNative(1))
				break;
		}
	}
		
error:

    DoSoundBlockClear(8);

	close_kludgeflic();
	smooth_black_retp();
   	return;
}
