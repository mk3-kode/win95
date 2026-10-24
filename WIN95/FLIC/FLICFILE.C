/******************************************************************************
 FLICFILE.C - reading Autodesk .FLI/.FLC animations.

 Reconstructed from mk3w.exe (retail).

 Only what play_flic_and_sound (FLICPLAY.C) needs: open, seek to the first
 frame, read and decode a frame. The first read loads the whole file into
 memory, so that playing it does not wait for the CD; the load stops early
 if a key or a joystick button is pressed.
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "flicfile.h"

#define LOAD_CHUNK	30000

extern int GetKeyNowait(void);			/* KEYBOARD.C */
extern unsigned long JoyGetNative(int id);	/* JOYSTICK.C */

static unsigned char *g_read_ptr;		/* next frame in file_buf */
static long file_size;
static unsigned char *file_buf;
static long read_pos;				/* never moved */
char touch;					/* bytes read back to keep the buffer in memory */
static int seek_modes[3] = { SEEK_SET, SEEK_CUR, SEEK_END };
static int loaded = 0;

static int FlicFileOpen(int *fd, char *path, int mode)
{
	if ((*fd = open(path, mode)) == -1)
		return -1;
	return 0;
}

static int FlicFileClose(int *fd)
{
	if (*fd != -1)
		close(*fd);
	*fd = -1;
	return 0;
}

static int FlicFileRead(int fd, void *buf, int size)
{
	return read(fd, buf, size);
}

static long FlicFileSeek(int fd, long offset, unsigned mode)
{
	if (mode >= 3)
		return -1;
	return lseek(fd, offset, seek_modes[mode]);
}

static int FlicFileReadAt(int fd, void *buf, int size, long offset)
{
	if (lseek(fd, offset, SEEK_SET) >= 0 && read(fd, buf, size) == size)
		return 0;
	return -1;
}

/* the error of a short read */
static int FlicIoError(int fd)
{
	return -1;
}

static int FlicScale(unsigned short v, unsigned short mul, unsigned short div)
{
	return (int)((unsigned)v * mul) / div;
}

/* the largest frame the decompression buffer can take */
static long FlicBufferSize(int width, int height, int colors)
{
	long size = colors * 3 + 0x19c8;

	size += width * 2 > 200 ? width * 2 : 200;
	return height * width + size;
}

static int FlicOpenHeader(char *path, FlicHead *hdr, int *fd, int mode)
{
	int err;
	int f;

	err = FlicFileOpen(fd, path, mode);
	if (err >= 0)
	{
		f = *fd;
		if ((unsigned)FlicFileRead(f, hdr, sizeof(FlicHead)) < sizeof(FlicHead))
		{
			if ((err = FlicIoError(f)) != -1)
				goto done;
fail:
			err = -1;
		}
		else
		{
			if (hdr->type == FLIC_FLI)
				hdr->speed = FlicScale(*(unsigned short *)&hdr->speed, 1000, 70);	/* FLI: a word of 1/70 s */
			else if (hdr->type != FLIC_FLC)
				goto fail;
			if (hdr->depth == 0)
				hdr->depth = 8;
			if (hdr->aspect_dx == 0 || hdr->aspect_dy == 0)
			{
				hdr->aspect_dy = 1;
				hdr->aspect_dx = 1;
			}
			return 0;
		}
	}
done:
	FlicFileClose(fd);
	return err;
}

int FlicClose(Flic *flic)
{
	if (file_buf != NULL)
		free(file_buf);
	FlicFileClose(&flic->fd);
	return 0;
}

/* open and find the first two frames (after a prefix chunk) */
int FlicOpen(char *path, Flic *flic, int mode)
{
	FlicChunk chunk;
	int err;

	err = FlicOpenHeader(path, &flic->hdr, &flic->fd, O_RDONLY | O_BINARY);
	if (err < 0)
		goto error;
	if (flic->hdr.frame_count != 0)
	{
		if ((unsigned)FlicFileRead(flic->fd, &chunk, 6) < 6)
		{
			err = FlicIoError(flic->fd);
			goto error;
		}
		if (chunk.type == FLIC_FRAME)
		{
			flic->hdr.frame1_oset = sizeof(FlicHead);
			flic->hdr.frame2_oset = chunk.size + sizeof(FlicHead);
		}
		else if (chunk.type == FLIC_PREFIX)
		{
			flic->hdr.frame1_oset = chunk.size + sizeof(FlicHead);
			if ((err = FlicFileReadAt(flic->fd, &chunk, 6, flic->hdr.frame1_oset)) < 0)
				goto error;
			if (chunk.type != FLIC_FRAME)
				goto bad;
			flic->hdr.frame2_oset = flic->hdr.frame1_oset + chunk.size;
			return 0;
		}
		else
			goto bad;
	}
	return 0;
bad:
	err = -1;
error:
	if (err == -1)
		err = -1;
	FlicClose(flic);
	flic->err = err;
	return err;
}

int FlicSeekFirst(Flic *flic)
{
	long ret;

	if ((ret = FlicFileSeek(flic->fd, flic->hdr.frame1_oset, 0)) < 0)
		return ret;
	return 0;
}

int FlicSeekSecond(Flic *flic)
{
	long ret;

	if ((ret = FlicFileSeek(flic->fd, flic->hdr.frame2_oset, 0)) < 0)
		return ret;
	return 0;
}

/* the position is added to the source, not the destination
   (it is always 0) */
static void FlicDecodeLC(FlicRaster *r, unsigned char *data, int x, int y, int w, int h)
{
	data += x + y * r->bpr;
	DecodeLC(data, r->pixels, r->bpr, w);
}

static void FlicDecodeDeltaFLC(FlicRaster *r, unsigned char *data, int x, int y, int w, int h)
{
	data += x + y * r->bpr;
	DecodeDeltaFLC(data, r->pixels, r->bpr, w);
}

static void FlicDecodeBlack(FlicRaster *r, unsigned char *data, int x, int y, int w, int h)
{
}

static void FlicDecodeCopy(FlicRaster *r, unsigned char *data, int x, int y, int w, int h)
{
}

static void FlicDecodeChunks(FlicRaster *r, FlicFrame *frame, FlicRect *rect)
{
	int i;
	FlicChunk *chunk = (FlicChunk *)&frame[1];

	for (i = 0; i < frame->chunks; i++)
	{
		switch (chunk->type)
		{
			case FLIC_COLOR_256:
				DecodeColor256(&((unsigned char *)chunk)[6], r->palette);
				break;
			case FLIC_DELTA_FLC:
				FlicDecodeDeltaFLC(r, &((unsigned char *)chunk)[6], rect->x, rect->y, rect->w, rect->h);
				break;
			case FLIC_COLOR_64:
				DecodeColor64(&((unsigned char *)chunk)[6], r->palette);
				break;
			case FLIC_DELTA_FLI:
				FlicDecodeLC(r, &((unsigned char *)chunk)[6], rect->x, rect->y, rect->w, rect->h);
				break;
			case FLIC_BLACK:
				FlicDecodeBlack(r, NULL, rect->x, rect->y, rect->w, rect->h);
				break;
			case FLIC_BYTE_RUN:
				DecodeBrun(r, &((unsigned char *)chunk)[6], rect->x, rect->y, rect->w, rect->h);
				break;
			case FLIC_COPY:
				FlicDecodeCopy(r, &((unsigned char *)chunk)[6], rect->x, rect->y, rect->w, rect->h);
				break;
		}
		chunk = (FlicChunk *)((unsigned char *)chunk + chunk->size);
	}
}

static int FlicDecodeFrame(FlicRaster *r, FlicFrame *frame)
{
	FlicRect rect;

	rect.x = 0;
	rect.y = 0;
	rect.w = r->rect.w;
	rect.h = r->rect.h;
	FlicDecodeChunks(r, frame, &rect);
	return 0;
}

/* the next frame into frame, decoded into rast unless it is NULL */
int FlicReadUncompFrame(Flic *flic, FlicRaster *rast, FlicFrame *frame)
{
	long start, left, pos;
	int size, got, n, err;
	char c;
	unsigned char *p;

	if (!loaded)
	{
		loaded = 1;
		start = lseek(flic->fd, 0, SEEK_CUR);
		file_size = lseek(flic->fd, 0, SEEK_END) - start;
		lseek(flic->fd, start, SEEK_SET);
		file_buf = malloc(file_size);
		if (file_buf != NULL)
		{
			left = file_size;
			g_read_ptr = file_buf;
			while (left > 0)
			{
				if (GetKeyNowait())
					return 0;
				if (JoyGetNative(0))
					return 0;
				if (JoyGetNative(1))
					return 0;
				if (left > LOAD_CHUNK)
					size = LOAD_CHUNK;
				else
					size = left;
				got = read(flic->fd, g_read_ptr, size);
				if (size > got)
				{
					if ((err = FlicIoError(flic->fd)) == -1)
						err = -1;
					return err;
				}
				left -= got;
				g_read_ptr += got;
				for (n = 30; n != 0; n--)
				{
					pos = (rand() * (file_size - left)) / 0x7fff;
					if (pos >= 0 && pos < file_size)
					{
						p = file_buf + pos;
						c = *p;
						if (c == 0xff && touch == 0xff)
							c = *p - 1;
						touch = c;
					}
				}
			}
		}
		g_read_ptr = file_buf;
	}
	for (n = 30; n != 0; n--)
	{
		pos = (rand() * (file_size - read_pos)) / 0x7fff;
		if (read_pos + pos >= 0 && read_pos + pos < file_size)
		{
			c = (file_buf + read_pos)[pos];
			if (c == 0xff && touch == 0xff)
				c--;
			touch = c;
		}
	}
	memcpy(frame, g_read_ptr, sizeof(FlicFrame));
	g_read_ptr += sizeof(FlicFrame);
	touch += 0x16;
	if (frame->type != FLIC_FRAME)
		return -1;
	if (FlicBufferSize(flic->hdr.width, flic->hdr.height, 255) <= frame->size)
		return -1;
	size = frame->size - sizeof(FlicFrame);
	memcpy(frame + 1, g_read_ptr, size);
	g_read_ptr += size;
	if (rast != NULL)
		return FlicDecodeFrame(rast, frame);
	return 0;
}
