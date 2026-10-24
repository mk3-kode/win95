/******************************************************************************
 FLICFILE.H - reading Autodesk .FLI/.FLC animations (FLICFILE.C and the four
 FLIC*.ASM files).

 Reconstructed: FLICPLAY.C includes "../flic/flicfile.h" and uses Flic,
 FlicRaster, FlicFrame, FlicRGB, KludgeFlic and the calls below. The layouts
 are the ones the code in mk3w.exe uses.
******************************************************************************/
#ifndef __FLICFILE_H
#define __FLICFILE_H

#define FLIC_FLI		0xaf11		/* header types */
#define FLIC_FLC		0xaf12
#define FLIC_PREFIX		0xf100		/* chunk types */
#define FLIC_FRAME		0xf1fa

#define FLIC_COLOR_256	4			/* frame chunk types */
#define FLIC_DELTA_FLC	7
#define FLIC_COLOR_64	11
#define FLIC_DELTA_FLI	12
#define FLIC_BLACK		13
#define FLIC_BYTE_RUN	15
#define FLIC_COPY		16

typedef struct {
	unsigned char r, g, b;
} FlicRGB;

typedef struct {
	unsigned short w, h;
	short x, y;
} FlicRect;

typedef struct {
	FlicRect rect;
	unsigned char *pixels;
	long bpr;					/* bytes per row */
	FlicRGB *palette;			/* the COLOR chunks are decoded here */
	unsigned short *clutpsx;	/* PSX colours of the palette (FLICPLAY.C) */
	unsigned short *clut;		/* in the display format */
} FlicRaster;

#pragma pack(1)
typedef struct {
	long size;
	unsigned short type;		/* FLIC_FLI or FLIC_FLC */
	unsigned short frame_count;
	unsigned short width;
	unsigned short height;
	unsigned short depth;
	unsigned short flags;
	long speed;					/* ms per frame (FLI: converted from 1/70 s) */
	unsigned char pad1[0x26 - 0x14];
	unsigned short aspect_dx;
	unsigned short aspect_dy;
	unsigned char pad2[0x50 - 0x2a];
	long frame1_oset;
	long frame2_oset;
	unsigned char pad3[0x80 - 0x58];
} FlicHead;

typedef struct {
	long size;
	unsigned short type;		/* FLIC_FRAME */
	short chunks;
	unsigned char pad[8];
} FlicFrame;

typedef struct {
	long size;
	unsigned short type;
} FlicChunk;
#pragma pack()

typedef struct {
	FlicHead hdr;
	int fd;
	int err;
} Flic;

/* the flic shown by play_flic_and_sound, in player_heap */
typedef struct {
	FlicRaster rast;
	Flic flic;
	FlicFrame *decomp_buff;
} KludgeFlic;

int FlicOpen(char *path, Flic *flic, int mode);
int FlicClose(Flic *flic);
int FlicSeekFirst(Flic *flic);
int FlicSeekSecond(Flic *flic);
int FlicReadUncompFrame(Flic *flic, FlicRaster *rast, FlicFrame *frame);

/* FLICCOL.ASM, FLICBRUN.ASM, FLICLC.ASM and FLICSS2.ASM */
void DecodeColor256(void *data, FlicRGB *palette);
void DecodeColor64(void *data, FlicRGB *palette);
void *DecodeBrun(FlicRaster *r, void *data, int x, int y, int w, int h);
void *DecodeLC(void *data, unsigned char *pixels, long bpr, int width);	/* width unused */
void *DecodeDeltaFLC(void *data, unsigned char *pixels, long bpr, int width);

#endif /* __FLICFILE_H */
