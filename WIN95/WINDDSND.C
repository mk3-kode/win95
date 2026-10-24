/******************************************************************************
 WINDDSND.C - sound effects through DirectSound.

 Reconstructed from mk3w.exe (retail), compiled without
 optimisation.

 The PlayStation sound banks are replaced by .LCD files holding 16-bit mono
 samples: a count, then {sound number, size, offset} for each sound. Every
 sound becomes a static DirectSound buffer; up to six duplicates let it
 play over itself.

 .LCD file:	long count;
			struct { long num, size, offset; } entry[count];
			sample data
******************************************************************************/
/* retail compiled this file as C++ (its uninitialised globals are .bss definitions, not
   common symbols): the C interfaces of DirectX and C names for everything */
#define CINTERFACE
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <dsound.h>
#include "winddsnd.h"
#include "emulator.h"
#include "win95.h"
#include "winddraw.h"
#include "psxcd.h"

EMSOUNDOBJ sndobj[MAX_SOUND_OBJS];

static int pan_mode;					/* wess_pan_mode_set: 2 = reversed */
static LPDIRECTSOUNDBUFFER lpDSPrimary;
static GUID ds_guid;					/* DSEnumProc; not used */
static LPDIRECTSOUND lpDS;

static WAVEFORMATEX lcd_format = { WAVE_FORMAT_PCM, 1, 15625, 31250, 2, 16, 0 };
/* the RGB555 masks, nothing refers to them (in retail's .data right after
   lcd_format): three variables, as an array would be on 8 bytes */
static u_long unused_red = 0x7c00;
static u_long unused_green = 0x03e0;
static u_long unused_blue = 0x001f;

extern HWND g_hWndMain;
extern HMODULE hDSound;		/* WINMAIN.C */
extern FARPROC lpDirectSoundCreate;

/* a DirectSoundEnumerate callback cut down from the DirectX SDK
   samples (hCombo and lpTemp are left from them); not used */
BOOL CALLBACK DSEnumProc(LPGUID lpGUID, LPSTR lpszDesc, LPSTR lpszDrvName, LPVOID lpContext)
{
	HWND hCombo = *(HWND *)lpContext;
	LPGUID lpTemp = NULL;

	if (lpGUID != NULL)
	{
		memcpy(&ds_guid, lpGUID, sizeof(GUID));
		return FALSE;
	}
	return TRUE;
}

int Win95_InitDirectSound(void)
{
	int j, num;
	DWORD acmsize;
	DSCAPS dsc;
	DSBUFFERDESC dsbdesc;
	HRESULT hr;
	MMRESULT mmr;

	if (hDSound == NULL || lpDirectSoundCreate == NULL)
		return 0;
	for (num = 0; num < MAX_SOUND_OBJS; num++)
	{
		sndobj[num].pDSB = NULL;
		sndobj[num].flags = 0;
		sndobj[num].group = 0;
		for (j = 0; j < MAX_SOUND_DUPS; j++)
			sndobj[num].pDSBdup[j] = NULL;
	}
	mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &acmsize);
	if (mmr != 0)
		return 0;
	if (DirectSoundCreate(NULL, &lpDS, NULL) != DS_OK)
		return 0;
	if (lpDS->lpVtbl->SetCooperativeLevel(lpDS, g_hWndMain, DSSCL_EXCLUSIVE) != DS_OK)
		return 0;
	dsc.dwSize = sizeof(dsc);
	if (lpDS->lpVtbl->GetCaps(lpDS, &dsc) != DS_OK)
		return 0;
	memset(&dsbdesc, 0, sizeof(dsbdesc));
	dsbdesc.dwSize = sizeof(dsbdesc);
	dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
	hr = lpDS->lpVtbl->CreateSoundBuffer(lpDS, &dsbdesc, &lpDSPrimary, NULL);
	if (hr != DS_OK)
		return 0;
	lpDSPrimary->lpVtbl->Play(lpDSPrimary, 0, 0, DSBPLAY_LOOPING);
	return 1;
}

int Win95_DeinitDirectSound(void)
{
	if (lpDSPrimary != NULL)
	{
		lpDSPrimary->lpVtbl->Release(lpDSPrimary);
		lpDSPrimary = NULL;
	}
	return 0;
}

/* WM_ACTIVATEAPP */
int Win95_SoundActivate(int active)
{
	return 0;
}

/* load a sound bank; returns 1 if it was read */
int Win95_Load_LCD_File(char *name, unsigned long group, int perm)
{
	u_char *src = NULL;
	void *first = NULL;
	DWORD n1;
	void *pSecond = NULL;
	int i;
	long *block;
	char path[260];
	long got;
	HFILE fp;
	DWORD secondlen;
	long sizes;
	long *entry;
	DSBUFFERDESC dsbd;
	LPDIRECTSOUNDBUFFER sbuf;
	HRESULT hr;

	if (lpDS == NULL)
		return 0;
	psxcd_stop();
	BuildFullPath(path, name);
	fp = _lopen(path, OF_READ);
	if (fp == HFILE_ERROR)
		return 0;
	sizes = _llseek(fp, 0, 2);
	if (sizes == -1)
	{
		sizes = 0;
		_lclose(fp);
		return 0;
	}
	block = (long *)malloc(sizes);
	if (block == NULL)
	{
		free(block);
		_lclose(fp);
		return 0;
	}
	_llseek(fp, 0, 0);
	got = _hread(fp, block, sizes);
	_lclose(fp);
	if (got != sizes)
	{
		free(block);
		return 0;
	}
	for (i = 0; i < block[0]; i++)
	{
		entry = block + i * 3 + 1;
		if (entry[1] == 0)
			continue;
		sndobj[entry[0]].group |= group;
		if (perm)
			sndobj[entry[0]].flags |= SND_PERM;
		if (sndobj[entry[0]].flags & SND_LOADED)
			continue;
		memset(&dsbd, 0, sizeof(dsbd));
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = 0;
		dsbd.dwFlags |= DSBCAPS_LOCSOFTWARE;
		dsbd.dwFlags |= DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;
		dsbd.dwBufferBytes = entry[1];
		dsbd.lpwfxFormat = &lcd_format;
		hr = lpDS->lpVtbl->CreateSoundBuffer(lpDS, &dsbd, &sbuf, NULL);
		if (hr != DS_OK)
			return 0;
		hr = sbuf->lpVtbl->Lock(sbuf, 0, entry[1], &first, &n1, &pSecond, &secondlen, 0);
		if (hr != DS_OK)
			return 0;
		src = &((u_char *)block)[entry[2]];
		CopyLongs(first, src, n1);
		if (secondlen != 0)
			CopyLongs(pSecond, &src[n1], secondlen);
		hr = sbuf->lpVtbl->Unlock(sbuf, first, entry[1], NULL, 0);
		if (hr != DS_OK)
			return 0;
		hr = sbuf->lpVtbl->SetVolume(sbuf, 0);
		if (hr != DS_OK)
			return 0;
		hr = sbuf->lpVtbl->SetPan(sbuf, 0);
		if (hr != DS_OK)
			return 0;
		sndobj[entry[0]].flags |= SND_LOADED;
		sndobj[entry[0]].pDSB = sbuf;
	}
	free(block);
	return 1;
}

/* not called */
void Wav95_SetVolume(char vol)
{
	lpDSPrimary->lpVtbl->SetVolume(lpDSPrimary, (int)vol << 2);
	return;
}

/* wess_pan_mode_set */
void Wav95_SetPanMode(char mode)
{
	pan_mode = mode;
	return;
}

/* pan 0 (left) - 127 (right). When the sound and a duplicate are
   both playing, the duplicates after that one are released and the sound is
   not played. */
void Wav95_PlayWav(int num, int pan)
{
	HRESULT hr;
	int i;
	DWORD status;

	if (num < 0 || num > MAX_SOUND_OBJS)
		return;
	pan = (pan * 20000) / 127 - 10000;
	if (pan_mode == 2)
		pan *= -1;
	if (sndobj[num].pDSB != NULL && (sndobj[num].flags & SND_LOADED))
	{
		sndobj[num].pDSB->lpVtbl->GetStatus(sndobj[num].pDSB, &status);
		if (status & DSBSTATUS_PLAYING)
		{
			for (i = 0; i < MAX_SOUND_DUPS; i++)
			{
				if (sndobj[num].pDSBdup[i] != NULL)
				{
					sndobj[num].pDSBdup[i]->lpVtbl->GetStatus(sndobj[num].pDSBdup[i], &status);
					if (!(status & DSBSTATUS_PLAYING))
					{
						sndobj[num].pDSBdup[i]->lpVtbl->SetPan(sndobj[num].pDSBdup[i], pan);
						sndobj[num].pDSBdup[i]->lpVtbl->Play(sndobj[num].pDSBdup[i], 0, 0, 0);
						return;
					}
					for (i++; i < MAX_SOUND_DUPS; i++)
					{
						if (sndobj[num].pDSBdup[i] != NULL)
						{
							sndobj[num].pDSBdup[i]->lpVtbl->Stop(sndobj[num].pDSBdup[i]);
							sndobj[num].pDSBdup[i]->lpVtbl->Release(sndobj[num].pDSBdup[i]);
							sndobj[num].pDSBdup[i] = NULL;
						}
					}
				}
				else
				{
					hr = lpDS->lpVtbl->DuplicateSoundBuffer(lpDS, sndobj[num].pDSB, &sndobj[num].pDSBdup[i]);
					if (hr != DS_OK)
					{
						sndobj[num].pDSBdup[i] = NULL;
						return;
					}
					if (sndobj[num].pDSBdup[i] != NULL)
					{
						sndobj[num].pDSBdup[i]->lpVtbl->SetPan(sndobj[num].pDSBdup[i], pan);
						sndobj[num].pDSBdup[i]->lpVtbl->Play(sndobj[num].pDSBdup[i], 0, 0, 0);
					}
					return;
				}
			}
		}
		else
		{
			sndobj[num].pDSB->lpVtbl->SetPan(sndobj[num].pDSB, pan);
			sndobj[num].pDSB->lpVtbl->Play(sndobj[num].pDSB, 0, 0, 0);
		}
	}
	return;
}

/* release a sound unless it is permanent */
void Wav95_ReleaseWav(int num)
{
	int i;

	if (num < 0 || num > MAX_SOUND_OBJS)
		return;
	if (sndobj[num].flags & SND_PERM)
		return;
	if (sndobj[num].pDSB != NULL)
	{
		sndobj[num].pDSB->lpVtbl->Stop(sndobj[num].pDSB);
		sndobj[num].pDSB->lpVtbl->Release(sndobj[num].pDSB);
		sndobj[num].pDSB = NULL;
	}
	for (i = 0; i < MAX_SOUND_DUPS; i++)
	{
		if (sndobj[num].pDSBdup[i] != NULL)
		{
			sndobj[num].pDSBdup[i]->lpVtbl->Stop(sndobj[num].pDSBdup[i]);
			sndobj[num].pDSBdup[i]->lpVtbl->Release(sndobj[num].pDSBdup[i]);
			sndobj[num].pDSBdup[i] = NULL;
		}
	}
	sndobj[num].flags = 0;
	sndobj[num].group = 0;
	return;
}

/* wess_seq_stop */
void Wav95_StopWav(int num)
{
	int i;

	if (num < 0 || num > MAX_SOUND_OBJS)
		return;
	if (sndobj[num].pDSB != NULL)
	{
		sndobj[num].pDSB->lpVtbl->Stop(sndobj[num].pDSB);
		for (i = 0; i < MAX_SOUND_DUPS; i++)
			if (sndobj[num].pDSBdup[i] != NULL)
				sndobj[num].pDSBdup[i]->lpVtbl->Stop(sndobj[num].pDSBdup[i]);
	}
	return;
}

/* wess_seq_stopall */
void Wav95_StopWavAll(void)
{
	int i, num;

	for (num = 0; num < MAX_SOUND_OBJS; num++)
	{
		if (sndobj[num].pDSB != NULL)
			sndobj[num].pDSB->lpVtbl->Stop(sndobj[num].pDSB);
		for (i = 0; i < MAX_SOUND_DUPS; i++)
			if (sndobj[num].pDSBdup[i] != NULL)
				sndobj[num].pDSBdup[i]->lpVtbl->Stop(sndobj[num].pDSBdup[i]);
	}
	return;
}

/* PsxSoundLoadClear */
void Wav95_ReleaseWavAll(void)
{
	int num, i;

	for (num = 0; num < MAX_SOUND_OBJS; num++)
	{
		if (sndobj[num].flags & SND_PERM)
			continue;
		if (sndobj[num].pDSB != NULL)
		{
			sndobj[num].pDSB->lpVtbl->Stop(sndobj[num].pDSB);
			sndobj[num].pDSB->lpVtbl->Release(sndobj[num].pDSB);
		}
		for (i = 0; i < MAX_SOUND_DUPS; i++)
		{
			if (sndobj[num].pDSBdup[i] != NULL)
			{
				sndobj[num].pDSBdup[i]->lpVtbl->Stop(sndobj[num].pDSBdup[i]);
				sndobj[num].pDSBdup[i]->lpVtbl->Release(sndobj[num].pDSBdup[i]);
				sndobj[num].pDSBdup[i] = NULL;
			}
		}
		sndobj[num].pDSB = NULL;
		sndobj[num].flags = 0;
		sndobj[num].group = 0;
	}
	if (lpDS != NULL)
		lpDS->lpVtbl->Compact(lpDS);
	return;
}

#ifdef __cplusplus
}
#endif
