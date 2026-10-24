/******************************************************************************
 WINDDSND.H - DirectSound sound effects (WINDDSND.C).

 Reconstructed: PSXSND.C includes <winddsnd.h> and uses sndobj, its group
 and pDSB members, MAX_SOUND_OBJS and the calls below.
******************************************************************************/
#ifndef __WINDDSND_H
#define __WINDDSND_H


struct IDirectSoundBuffer;
struct pcmwaveformat_tag;

#define MAX_SOUND_OBJS	500
#define MAX_SOUND_DUPS	6		/* copies of a sound playing at once */

/* EMSOUNDOBJ.flags */
#define SND_PERM		0x01	/* loaded by PsxSoundLoadDataLoadPerm, never released */
#define SND_LOADED		0x02

typedef struct {
	unsigned long flags;
	unsigned long group;		/* PSXSND.C groups that loaded this sound */
	struct IDirectSoundBuffer *pDSB;	/* LPDIRECTSOUNDBUFFER; the game does not include dsound.h */
	struct IDirectSoundBuffer *pDSBdup[MAX_SOUND_DUPS];
} EMSOUNDOBJ;

extern EMSOUNDOBJ sndobj[MAX_SOUND_OBJS];

int Win95_InitDirectSound(void);
int Win95_DeinitDirectSound(void);
int Win95_SoundActivate(int active);
int Win95_Load_LCD_File(char *name, unsigned long group, int perm);
/* no such function (its place in the exe is DSEnumProc's, WINDDSND.C); kept because PSXSND.C's match
   depends on this header's symbol count */
int Wav95_SetFormat(struct pcmwaveformat_tag *format);
void Wav95_SetVolume(char vol);
void Wav95_SetPanMode(char mode);
void Wav95_PlayWav(int num, int pan);
void Wav95_ReleaseWav(int num);
void Wav95_StopWav(int num);
void Wav95_StopWavAll(void);
void Wav95_ReleaseWavAll(void);

#endif /* __WINDDSND_H */
