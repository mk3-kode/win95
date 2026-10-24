/******************************************************************************
 PSXLIB.H - PlayStation library types and calls as emulated on Windows 95.

 Reconstructed from mk3w.exe (retail, linked 1996-07-05). The original header
 (psxlib.h, included by the game through win_port.h) has not survived;
 the layouts below are the ones the emulation code in mk3w.exe reads.

 Differences from the Sony libgpu.h:
   - the primitive tag is two longs (next pointer, length) instead of one
     packed long, so every primitive is 4 bytes longer and the length the
     Set* calls store is the Sony length + 2;
   - the ordering table is an array of such two-long tags.
******************************************************************************/
#ifndef __PSXLIB_H
#define __PSXLIB_H

/* NULL is a plain 0: fadepal (MKFADE.C) tests its WORD pa2 against NULL, and retail
   compares the 16-bit value, not the value widened to a pointer */
#define NULL 0
#include <stddef.h>
#include <sys/types.h>		/* SYS\TYPES.H: the PSX u_char & co. */

/* the retail game has every __inline function as an ordinary one, called and
   kept even when unused (yzsort in MKOBJ.C): the keyword was defined away */
#define __inline

/* abs is a macro for the game: retail evaluates the argument twice
   (q_corner_backf_land and q_proj_jclose in MKDRONE.C call get_his_y_vel and
   his_proj_front_x twice). MKINIT.C, the one module with psxtowin.h, keeps
   the library abs: it takes the abs of a float there. */
#ifndef __PSXTOWIN_H
#define abs(x) ((x) < 0 ? -(x) : (x))
#endif

/* the game's types, which MKSONY.H defined on the PlayStation. LONG is
   unsigned, as in the layer's stdstuff.h (win_port.h, MKINIT.H); the layer
   files that include windows.h keep its signed LONG. */
typedef unsigned char	u_byte;
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
#if !defined(__STDSTUFF_H) && !defined(_WINDEF_)
typedef unsigned long	LONG;
#endif
typedef unsigned long	ADDRESS;

typedef struct {
	short x, y;
	short w, h;
} RECT_PSX;

typedef struct {
	u_long *tag;		/* next primitive in the list, NULL at the end */
	u_long len;			/* length in longs after the tag; 0 = empty OT entry */
} P_TAG;

typedef P_TAG OTOBJ;	/* an ordering table entry (MKSONY.H) */

/* primitive codes (Sony values) */
#define PRIM_BLK_FILL	0x02
#define PRIM_POLY_F4	0x28
#define PRIM_POLY_FT4	0x2c
#define PRIM_SPRT		0x64
#define PRIM_DR_MODE	0xe1

typedef struct {
	P_TAG tag;
	u_char r0, g0, b0, code;
	u_short x0, y0;		/* unsigned, as in Sony's libgpu.h */
	u_short w, h;
} BLK_FILL;

typedef struct {
	P_TAG tag;
	u_char r0, g0, b0, code;
	short x0, y0;
	short x1, y1;
	short x2, y2;
	short x3, y3;
} POLY_F4;

typedef struct {
	P_TAG tag;
	u_char r0, g0, b0, code;
	short x0, y0;
	u_char u0, v0; u_short clut;
	short x1, y1;
	u_char u1, v1; u_short tpage;
	short x2, y2;
	u_char u2, v2; u_short pad1;
	short x3, y3;
	u_char u3, v3; u_short pad2;
} POLY_FT4;

typedef struct {
	P_TAG tag;
	u_char r0, g0, b0, code;
	short x0, y0;
	u_char u0, v0; u_short clut;
	short w, h;
} SPRT;

typedef struct {
	P_TAG tag;
	u_char pad[3], code;
	u_short tpage;		/* texture page the following sprites use */
	u_short pad2;
} DR_MODE;

typedef struct {
	RECT_PSX clip;		/* +0x00 */
	short pad0;
	short ofs[2];		/* +0x0a */
	RECT_PSX tw;		/* +0x0e */
	u_char dtd;			/* +0x16 */
	u_char dfe;			/* +0x17 */
	u_char isbg;		/* +0x18 */
	u_char r0, g0, b0;	/* +0x19 */
	u_long dr_env[17];
} DRAWENV;				/* 0x60 bytes */

typedef struct {
	RECT_PSX disp;
	RECT_PSX screen;
	u_char isinter, isrgb24, pad0, pad1;
} DISPENV;

/* root counters (kernel.h values) */
#define DescRC			0xf2000000
#define RCntCNT0		(DescRC | 0x00)
#define RCntCNT1		(DescRC | 0x01)
#define RCntCNT2		(DescRC | 0x02)
#define RCntCNT3		(DescRC | 0x03)
#define RCntMdNOINTR	0x2000

/* a PSX colour (BGR555) */
#define PSX_RGB(r,g,b)	((((b) & 0xf8) << 7) | (((g) & 0xf8) << 2) | ((r) >> 3))

/* libgpu macros the game uses */
#define setRECT(r,_x,_y,_w,_h)	((r)->x = (_x), (r)->y = (_y), (r)->w = (_w), (r)->h = (_h))
#define setRGB0(p,r,g,b)		((p)->r0 = (r), (p)->g0 = (g), (p)->b0 = (b))
#define setXY0(p,_x0,_y0)		((p)->x0 = (_x0), (p)->y0 = (_y0))
#define setXY4(p,_x0,_y0,_x1,_y1,_x2,_y2,_x3,_y3) \
	((p)->x0 = (_x0), (p)->y0 = (_y0), (p)->x1 = (_x1), (p)->y1 = (_y1), \
	 (p)->x2 = (_x2), (p)->y2 = (_y2), (p)->x3 = (_x3), (p)->y3 = (_y3))
#define setUVWH(p,_u0,_v0,_w,_h) \
	((p)->u0 = (_u0), (p)->v0 = (_v0), (p)->u1 = (_u0) + (_w), (p)->v1 = (_v0), \
	 (p)->u2 = (_u0), (p)->v2 = (_v0) + (_h), (p)->u3 = (_u0) + (_w), (p)->v3 = (_v0) + (_h))
#define setWH(p,_w,_h)			((p)->w = (_w), (p)->h = (_h))
#define setXYWH(p,_x0,_y0,_w,_h) \
	((p)->x0 = (_x0), (p)->y0 = (_y0), (p)->x1 = (_x0) + (_w), (p)->y1 = (_y0), \
	 (p)->x2 = (_x0), (p)->y2 = (_y0) + (_h), (p)->x3 = (_x0) + (_w), (p)->y3 = (_y0) + (_h))
#define getTPage(tp,abr,x,y) \
	((((tp) & 0x3) << 7) | (((abr) & 0x3) << 5) | (((y) & 0x100) >> 4) | (((x) & 0x3ff) >> 6))
#define getClut(x,y)			(((y) << 6) | (((x) >> 4) & 0x3f))

/* PSXEMU.C */
#ifdef LoadImage
#undef LoadImage		/* windows.h */
#endif
int ResetGraph(int mode);
void SetGraphDebug(int level);
void SetDispMask(int mask);
int DrawSync(int mode);
void VSync(int mode);
void VSyncCallback(void (*f)(void));
void ResetCallback(void);
void EnterCriticalSection_PSX(void);
void ExitCriticalSection_PSX(void);
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h);
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h);
DRAWENV *PutDrawEnv(DRAWENV *env);
DISPENV *PutDispEnv(DISPENV *env);
void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT_PSX *tw);
void SetBlockFill(BLK_FILL *p);
void SetPolyF4(POLY_F4 *p);
void SetPolyFT4(POLY_FT4 *p);
void SetSprt(SPRT *p);
void SetSemiTrans(void *p, int abe);
P_TAG *ClearOTag(P_TAG *ot, int n);
void AddPrim(void *ot, void *p);
void DrawPrim(void *p);
int mod_DrawOTag(void *ot);		/* as MKINIT.C declares it */
void LoadImage(RECT_PSX *rect, u_long *p);
u_short LoadTPage(u_long *pix, int tp, int abr, int x, int y, int w, int h);
u_short GetTPage(int tp, int abr, int x, int y);
long SetRCnt(unsigned long spec, unsigned short target, long mode);
long StartRCnt(unsigned long spec);
int PadInit(int mode);
u_long PadRead(int id);

/* Visual C++ 2.0 picks registers by the order its symbols were declared in, so the
   number of declarations here (not their names) shapes the game's code. The
   original header's are unknown; these Sony library calls the game
   does not use bring the count to
   what the retail code needs: 13 more, of which 5 now come
   from SYS\TYPES.H (Visual C++'s full set of system types). */
int StoreImage(RECT_PSX *, u_long *);
int MoveImage(RECT_PSX *, int, int);
int ClearImage(RECT_PSX *, u_char, u_char, u_char);
u_short GetClut(int, int);
void SetPolyF3(void *);
void SetPolyFT3(void *);
void SetPolyG3(void *);
void SetPolyG4(void *);

#endif /* __PSXLIB_H */
