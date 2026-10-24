/******************************************************************************
 WESSWIN.C - the WESSAPI.H calls the game makes, on the Windows 95 sound.

 Reconstructed from mk3w.exe (retail). The WESS sound
 driver is not linked in the Windows version: a sequence number is the
 number of a sound in the .LCD banks (WINDDSND.C). The work is done on the
 big stack because DirectSound needs more stack than a game process has.
******************************************************************************/
#include "wessapi.h"
#include "winddsnd.h"
#include "bigstack.h"

void wess_seq_stop(int sequence_number)
{
	CallOnBigStack(Wav95_StopWav, sequence_number);
}

void wess_seq_stopall(void)
{
	CallOnBigStack(Wav95_StopWavAll);
}

void wess_master_sfx_vol_set(char volume)
{
}

void wess_master_mus_vol_set(char volume)
{
}

void wess_pan_mode_set(char mode)
{
	CallOnBigStack(Wav95_SetPanMode, mode);
}

/* pan 0, which Wav95_PlayWav makes hard left */
void wess_seq_trigger(int seq_num)
{
	CallOnBigStack(Wav95_PlayWav, seq_num, 0, 0);
}

void wess_seq_trigger_special(int seq_num, TriggerPlayAttr *attr)
{
	CallOnBigStack(Wav95_PlayWav, seq_num, (unsigned char)attr->pan, 0);
}
