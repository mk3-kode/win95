/******************************************************************************
 PSXTOWIN.H - the Windows 95 calls of the main loop (MKINIT.C).

 Reconstructed. MKINIT.C declares mod_DrawOTag, iWinActiveState, iWinExit
 and iWinFPS itself; the rest of what it calls is here.
******************************************************************************/
#ifndef __PSXTOWIN_H
#define __PSXTOWIN_H

int Win95_Pump(void);				/* 1 when the game must quit */
long Win95_timeGetTime();			/* MKINIT.C passes it an argument */
void Win95_Blast_Frame(void);
char *Win95_FPS(int skip_cnt, int skipped);

/* Visual C++ 2.0 picks registers by the order its symbols were declared in, so the
   number of declarations here (not their names) shapes the game's code. The
   original header's are unknown; these WINVIDEO.C function bring the count to
   what the retail code needs: 1 more. */
void Win95_OUT(char *);

#endif /* __PSXTOWIN_H */
