/******************************************************************************
 WIN_PORT.H - included first by the game modules of the Windows 95 version.

 Reconstructed. The original was in the layer's include directory
 (\mk3p\emulator\inc). Each build switch below is set the way
 the retail executable shows it was compiled:

   BLOCK_FILE_IO              main() does not call module_io_init
   USE_LINKED_DATA            generic/async/level_overlay_load just return 0
   USE_OVERLAY_LOADER         MKFILE.C loads textures with LoadOverlay
   BLOCK_MEMOVERFLOW_CHECK    no process end markers (MKOS.C)
   BLOCK_PAL_TICK             get_fore_pal / get_back_pal ignore paltime
   BLOCK_IRQ_TIMER            main() calls yzsort once per frame
   BLOCK_VBLANK_INTERUPT      wipeout does not wait for a vblank
   BLOCK_FPS                  no FPS_Display process
   FORCE_IGNORE_BAD_PALLETTES fadepal returns on NULL palettes
   BLOCK_MKBT_ADDITIONAL_ASSEMBLY  MKBT.C is C only (table_o_mods comes from it)

 and these are not set: MEM_OBJECTS (MKOBJ.C has no memory objects),
 FORCE_DRONE_FATALITY, FORCE_CHEAT_MODE.
******************************************************************************/
#ifndef __WIN_PORT_H
#define __WIN_PORT_H

#define BLOCK_FILE_IO
#define USE_LINKED_DATA
#define USE_OVERLAY_LOADER
#define BLOCK_MEMOVERFLOW_CHECK
#define BLOCK_PAL_TICK
#define BLOCK_IRQ_TIMER
#define BLOCK_VBLANK_INTERUPT
#define BLOCK_FPS
#define FORCE_IGNORE_BAD_PALLETTES
#define BLOCK_MKBT_ADDITIONAL_ASSEMBLY

#include "stdstuff.h"
/* bigstack.h is not included: its two symbols would be 2 too many */

/* KONTROLS screen (MKOPT.C): button numbers of KEYBOARD.C and JOYSTICK.C
   (pad 1; pad 2 adds IOBIX_P2), values from the tables in the executable */
#define IOBIX_NONE		0
#define IOBIX_RUN		1
#define IOBIX_BLOCK		3
#define IOBIX_HIKICK	5
#define IOBIX_LOKICK	6
#define IOBIX_LOPUNCH	7
#define IOBIX_HIPUNCH	8
#define IOBIX_START		12
#define IOBIX_JUP		13
#define IOBIX_JRIGHT	14
#define IOBIX_JDOWN		15
#define IOBIX_JLEFT		16
#define IOBIX_P2		16

/* direction bits of JoyGetNative */
#define NATIVE_JUP		0x10000000
#define NATIVE_JRIGHT	0x20000000
#define NATIVE_JDOWN	0x40000000
#define NATIVE_JLEFT	0x80000000

#endif /* __WIN_PORT_H */
