/******************************************************************************
 LIBCD.H - the Sony CD library types PSXCD.H uses.

 Reconstructed: PSXCD.H includes <libcd.h> for CdlFILE and CdlLOC (in
 PsxCd_File); nothing of the CD library is called on Windows 95.
******************************************************************************/
#ifndef _LIBCD_H_
#define _LIBCD_H_

#include <sys/types.h>

typedef struct {
	u_char minute;
	u_char second;
	u_char sector;
	u_char track;
} CdlLOC;

typedef struct {
	CdlLOC pos;
	u_long size;
	char name[16];
} CdlFILE;

/* Visual C++ 2.0 picks registers by the order its symbols were declared in, so the
   number of declarations here (not their names) shapes the game's code. The
   original header's are unknown; these CD library calls (none is
   called on Windows 95) bring the count to
   what the retail code needs: 17 more,
   for libcd.h and libsn.h together. */
int CdInit(void);
int CdFlush(void);
int CdSync(int, u_char *);
int CdReady(int, u_char *);
int CdControl(u_char, u_char *, u_char *);
int CdControlB(u_char, u_char *, u_char *);
int CdControlF(u_char, u_char *);
int CdMix(void *);
int CdGetSector(void *, int);
int CdRead(int, u_long *, int);
int CdReadSync(int, u_char *);
void *CdReadCallback(void (*)());
CdlFILE *CdSearchFile(CdlFILE *, char *);
CdlLOC *CdIntToPos(int, CdlLOC *);
int CdPosToInt(CdlLOC *);
int CdReset(int);
int CdStatus(void);

#endif /* _LIBCD_H_ */
