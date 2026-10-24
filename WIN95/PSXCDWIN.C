/******************************************************************************
 PSXCDWIN.C - the PSXCD.H music calls on Windows 95: CD audio through MCI.

 Reconstructed from mk3w.exe (retail).

 The music is red book audio on the game CD. MCI only says when a track is
 done through a notification the game never asks for, so the end of a track
 is predicted from a table of track lengths: psxcd_loop_callback, called
 every frame by main(), stops a track that should have ended and restarts
 the looping track when its time is up.
******************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "psxcd.h"
#include "emulator.h"
#include "win95.h"

#define LONGEST_TRACK_MS	148000		/* a time further ahead than this is bogus */

/* declared before mmsystem.h: Visual C++ 2.0 -Ox materialises the loads a join needs in
   declaration order, and retail loads cd_device and g_cd_looping before mciSendCommand
   (psxcd_loop_callback) and cd_device before g_cd_paused (psxcd_pause) */
static UINT cd_device = (UINT)-1;
static int g_cd_paused;
int g_cd_looping;					/* psxcd_play_at_andloop */

#include <mmsystem.h>

static MCI_PLAY_PARMS play_parms;
static MCI_PLAY_PARMS resume_parms;
static MCI_SEEK_PARMS seek_parms;		/* psxcd_seek_for_play_at */
static MCI_PLAY_PARMS g_loopparms1;

static long play_end;			/* when the playing track ends */
static int loop_track;
extern short f_music;			/* MKGLOBAL.C: 0 = music off */
extern HWND g_hWndMain;


/* length of each track in seconds, indexed by the track number - 1 (track 1 is the data track) */
static int track_seconds[47] = {
	0, 107, 77, 6, 6, 6, 7, 111, 6, 59, 6, 78, 6, 67, 13, 22,
	6, 106, 20, 6, 105, 22, 6, 113, 16, 6, 18, 8, 106, 17, 6, 26,
	6, 124, 14, 6, 118, 15, 6, 141, 16, 6, 113, 15, 6, 147, 87
};
static long pause_time = 0;		/* psxcd_pause */

int Loop_Time;					/* when the looping track ends (MKOPT.C sets it to 1: restart now) */
int cd_playing;					/* psxcd_play_at */

/* called every frame */
void psxcd_loop_callback(void)
{
	long now = timeGetTime();

	if (g_cd_paused)
		return;
	if (cd_playing)
	{
		if (play_end < now || play_end - now > LONGEST_TRACK_MS || g_cd_looping)
		{
			CallOnBigStack(mciSendCommand, cd_device, MCI_STOP, 0, 0);
			cd_playing = 0;
			Loop_Time = 0;
			g_cd_looping = 0;
		}
	}
	if (g_cd_looping && Loop_Time != 0 && (Loop_Time < now || Loop_Time - now > LONGEST_TRACK_MS))
	{
		if (f_music == 0)
		{
			Loop_Time = 0;
			CallOnBigStack(mciSendCommand, cd_device, MCI_PAUSE, 0, 0);
			return;
		}
		Loop_Time = now + track_seconds[loop_track - 1] * 1000;
		CallOnBigStack(mciSendCommand, cd_device, MCI_PLAY, MCI_FROM, &g_loopparms1);
	}
}

void psxcd_init(void)
{
	MCI_OPEN_PARMS open;
	MCI_SET_PARMS set;

	cd_playing = 0;
	g_cd_looping = 0;
	Loop_Time = 0;
	g_cd_paused = 0;
	open.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
	if (mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD)&open) == 0)
	{
		cd_device = open.wDeviceID;
		set.dwTimeFormat = MCI_FORMAT_TMSF;
		if (mciSendCommand(open.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&set) != 0)
			mciSendCommand(cd_device, MCI_CLOSE, 0, 0);
	}
}

/* called by WinMain on exit */
void psxcd_exit(void)
{
	cd_playing = 0;
	g_cd_looping = 0;
	Loop_Time = 0;
	g_cd_paused = 0;
	if (cd_device != (MCIDEVICEID)-1)
	{
		CallOnBigStack(mciSendCommand, cd_device, MCI_STOP, 0, 0);
		CallOnBigStack(mciSendCommand, cd_device, MCI_CLOSE, 0, 0);
	}
}

/* a volume of 0 only stops the CD */
void psxcd_play_at(int track, int vol, int sectoroffset)
{
	long start = timeGetTime();
	int seconds, minutes;

	cd_playing = 0;
	g_cd_looping = 0;
	Loop_Time = 0;
	g_cd_paused = 0;
	if (cd_device == (MCIDEVICEID)-1)
		return;
	CallOnBigStack(mciSendCommand, cd_device, MCI_STOP, 0, 0);
	if (vol == 0)
		return;
	seconds = sectoroffset / SECTORS_PER_SECOND;
	minutes = seconds / SECONDS_PER_MINUTE;
	play_parms.dwFrom = MCI_MAKE_TMSF(track, minutes, seconds - minutes * SECONDS_PER_MINUTE, 0);
	play_parms.dwCallback = (DWORD)g_hWndMain;
	play_end = timeGetTime() + track_seconds[track - 1] * 1000;
	cd_playing = 1;
	CallOnBigStack(mciSendCommand, cd_device, MCI_PLAY, MCI_FROM, &play_parms);
	if (g_bShowTimes)
		Win95_LogTime("Windows Time", start, timeGetTime(), 5);
}

/* plays track and keeps playing it; the second track, volume
   and offsets are not used */
void psxcd_play_at_andloop(int track, int vol, int sectoroffset, int fadeuptime,
						   int tracktoloop, int volforloop, int loopstartsectoroffset, int loopstartfadeuptime)
{
	long start = timeGetTime();
	int seconds, minutes;

	cd_playing = 0;
	g_cd_looping = 0;
	Loop_Time = 0;
	g_cd_paused = 0;
	if (cd_device == (MCIDEVICEID)-1)
		return;
	CallOnBigStack(mciSendCommand, cd_device, MCI_STOP, 0, 0);
	if (vol == 0)
		return;
	seconds = sectoroffset / SECTORS_PER_SECOND;
	minutes = seconds / SECONDS_PER_MINUTE;
	g_loopparms1.dwFrom = MCI_MAKE_TMSF(track, minutes, seconds - minutes * SECONDS_PER_MINUTE, 0);
	g_loopparms1.dwTo = (u_char)(track + 1);
	g_loopparms1.dwCallback = (DWORD)g_hWndMain;
	loop_track = track;
	g_cd_looping = 1;
	Loop_Time = timeGetTime() + track_seconds[track - 1] * 1000;
	CallOnBigStack(mciSendCommand, cd_device, MCI_PLAY, MCI_FROM, &g_loopparms1);
	if (g_bShowTimes)
		Win95_LogTime("Windows Time", start, timeGetTime(), 5);
}

/* not called (the call in MKMAIN.C's round_intro_fx is in #if 0) */
void psxcd_seek_for_play_at(int track, int sectoroffset)
{
	long start = timeGetTime();
	int seconds, minutes;

	cd_playing = 0;
	g_cd_looping = 0;
	Loop_Time = 0;
	g_cd_paused = 0;
	if (cd_device == (MCIDEVICEID)-1)
		return;
	CallOnBigStack(mciSendCommand, cd_device, MCI_STOP, 0, 0);
	seconds = sectoroffset / SECTORS_PER_SECOND;
	minutes = seconds / SECONDS_PER_MINUTE;
	seek_parms.dwTo = MCI_MAKE_TMSF(track, minutes, seconds - minutes * SECONDS_PER_MINUTE, 0);
	CallOnBigStack(mciSendCommand, cd_device, MCI_SEEK, MCI_TO, &seek_parms);
	if (g_bShowTimes)
		Win95_LogTime("Windows Time", start, timeGetTime(), 5);
}

void psxcd_stop(void)
{
	if (cd_playing || g_cd_looping)
	{
		g_cd_paused = 0;
		cd_playing = 0;
		g_cd_looping = 0;
		Loop_Time = 0;
		if (cd_device != (MCIDEVICEID)-1)
			CallOnBigStack(mciSendCommand, cd_device, MCI_STOP, 0, 0);
	}
}

void psxcd_pause(void)
{
	if (cd_device == (MCIDEVICEID)-1)
		return;
	if (!g_cd_paused)
		pause_time = timeGetTime();
	CallOnBigStack(mciSendCommand, cd_device, MCI_PAUSE, 0, 0);
	g_cd_paused = 1;
}

/* the predicted ends move by the time spent paused */
void psxcd_restart(int vol)
{
	if (g_cd_paused && (g_cd_paused = 0, cd_device != (MCIDEVICEID)-1))
	{
		pause_time = timeGetTime() - pause_time;
		play_end += pause_time;
		Loop_Time += pause_time;
		if (vol != 0)
		{
			resume_parms.dwFrom = 0;
			resume_parms.dwTo = 0;
			resume_parms.dwCallback = 0;
			CallOnBigStack(mciSendCommand, cd_device, MCI_PLAY, 0, &resume_parms);
		}
	}
}

int psxcd_seeking_for_play(void)
{
	return 0;
}

int psxcd_async_on(void)
{
	return 0;
}

int psxcd_waiting_for_pause(void)
{
	return 0;
}

/* not called by the game */
int psxcd_play_status(void)
{
	return 0;
}

/* "already playing for a long time" */
int psxcd_elapsed_sectors(void)
{
	return 0x8fffffff;
}

void psxcd_set_stereo(int stereo_true)
{
}

void psxcd_set_loop_volume(int volforloop)
{
}

/* PSXSPU.H */

void psxspu_start_cd_fade(int msec, int destvol)
{
}

int psxspu_get_cd_fade_status(void)
{
	return 0;
}

void psxspu_set_cd_vol(int vol)
{
}
