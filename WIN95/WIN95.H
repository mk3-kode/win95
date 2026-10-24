/******************************************************************************
 WIN95.H - declarations shared by the Windows 95 platform layer.

 Reconstructed; names of things the game does not see are descriptive.
******************************************************************************/
#ifndef __WIN95_H
#define __WIN95_H

#include "bigstack.h"

/* PlayStation pad bits */
#define PAD_L2			0x0001
#define PAD_R2			0x0002
#define PAD_L1			0x0004
#define PAD_R1			0x0008
#define PAD_TRIANGLE	0x0010
#define PAD_CIRCLE		0x0020
#define PAD_CROSS		0x0040
#define PAD_SQUARE		0x0080
#define PAD_SELECT		0x0100
#define PAD_START		0x0800
#define PAD_UP			0x1000
#define PAD_RIGHT		0x2000
#define PAD_DOWN		0x4000
#define PAD_LEFT		0x8000

#define GS_AMODE		0x01	/* MKGAME.H */

/* KEYBOARD.C */
extern u_long key_pad_bits;
void WriteKeymap(int fd);
void ReadKeymap(int fd);
int GetButtonMapkey(int buttonix);
int SetButtonMapkey(int buttonix, int mapchar);
int CheckForDefautNoKeys(int buttonix);
void SetDefaultKeymap(void);
char *GetKeyString(int key);
char *MapStartKeyToString(int player);
void ClearKeyBuffer(void);
int GetKeyNowait(void);
void Win95_KeyEvent(int down, int vk, long keydata);

/* JOYSTICK.C */
void WriteButtonmap(int fd);
void ReadButtonmap(int fd);
void SetDefaultButtonmap(void);
u_long GetBitButtons(int buttonix);
u_long SetBitButtons(int buttonix, u_long bits);
u_long JoyGetNative(int id);
u_long ReadJoystick(u_int id);

/* WINMAIN.C */
void BuildFullPath(char *outpath, char *subpath);
extern int g_JoyCount;			/* joysticks found at start-up (0-2) */
int ShiftKeyState(void);

#endif /* __WIN95_H */
