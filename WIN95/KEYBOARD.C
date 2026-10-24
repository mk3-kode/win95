/******************************************************************************
 KEYBOARD.C - keyboard as PlayStation pads, and the key remapping used by
 the KONTROLS screen (MKOPT.C).

 Reconstructed from mk3w.exe (retail); tables from its
 initialised data.

 Every virtual key (0-223) has a button number and a name. Buttons 1-16 are
 the bits of pad 1 (bit = 1 << (button - 1)), 17-32 the bits of pad 2.
******************************************************************************/
#include <windows.h>
#include <io.h>
#include "emulator.h"
#include "win95.h"

#define NUM_KEYS		224
#define NUM_BUTTONS		33
#define KEYNAME_CHAR	40		/* the key's name is its own character */

#define BUTTON_P1START	12
#define BUTTON_P2START	28

typedef struct {
	u_char button;
	u_char name;
} KEYMAP;

typedef struct {
	u_char button;
	u_char vk;
} KEYDEFAULT;

extern short gstate;			/* MKGLOBAL.C */
extern int remap_input;			/* MKOPT.C: 0 = keys give the default pad bits */

u_long key_pad_bits;			/* pad bits of the keys held down */
static int last_key;
static char key_char[2];
static int unused_two = 2;		/* not referred to (first in this file's .data) */

static char *key_names[KEYNAME_CHAR] = {
	NULL,
	"PAD 0", "PAD 1", "PAD 2", "PAD 3", "PAD 4", "PAD 5", "PAD 6", "PAD 7",
	"PAD 8", "PAD 9", "PAD STAR", "PAD PLUS", "PAD -", "PAD DOT", "PAD SLASH",
	"LEFT", "UP", "RIGHT", "DOWN", "INSERT", "HOME", "PAGE UP", "PAGE DOWN",
	"END", "DELETE", "LBRACKET", "RBRACKET", "BACKSPACE", "ENTER", "BSLASH",
	"SLASH", ".", "COMMA", "EQUAL", "-", "LQUOTE", "RQUOTE", "SEMI", "SPACE"
};

static KEYMAP keymap[NUM_KEYS] = {
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0,28}, { 0, 0}, { 0, 0}, { 0, 0},	/* 08=BACK */
	{ 0, 0}, {12,29}, { 0, 0}, { 0, 0},	/* 0d=RETURN */
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{28,39}, { 0,22}, { 0,23}, { 0,24},	/* 20=SPACE 21=PRIOR 22=NEXT 23=END */
	{ 0,21}, {16,16}, {13,17}, {14,18},	/* 24=HOME 25=LEFT 26=UP 27=RIGHT */
	{15,19}, { 0, 0}, { 0, 0}, { 0, 0},	/* 28=DOWN */
	{ 0, 0}, { 0,20}, { 0,25}, { 0, 0},	/* 2d=INSERT 2e=DELETE */
	{ 0,40}, { 0,40}, { 0,40}, { 0,40},	/* 30=0 31=1 32=2 33=3 */
	{ 0,40}, { 0,40}, { 0,40}, { 0,40},	/* 34=4 35=5 36=6 37=7 */
	{ 0,40}, { 0,40}, { 0, 0}, { 0, 0},	/* 38=8 39=9 */
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0,40}, {32,40}, { 0,40}, { 0,40},	/* 40=@ 41=A 42=B 43=C */
	{30,40}, { 0,40}, { 0,40}, {23,40},	/* 44=D 45=E 46=F 47=G */
	{17,40}, { 0,40}, {22,40}, { 0,40},	/* 48=H 49=I 4a=J 4b=K */
	{ 0,40}, {19,40}, { 0,40}, { 0,40},	/* 4c=L 4d=M 4e=N 4f=O */
	{ 0,40}, { 0,40}, { 0,40}, {31,40},	/* 50=P 51=Q 52=R 53=S */
	{24,40}, {21,40}, { 0,40}, {29,40},	/* 54=T 55=U 56=V 57=W */
	{ 0,40}, {19,40}, { 0,40}, { 0, 0},	/* 58=X 59=Y 5a=Z */
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 1, 1}, { 7, 2}, { 3, 3}, { 6, 4},	/* 60=NUMPAD0 61=NUMPAD1 62=NUMPAD2 63=NUMPAD3 */
	{ 8, 5}, { 0, 6}, { 5, 7}, { 0, 8},	/* 64=NUMPAD4 65=NUMPAD5 66=NUMPAD6 67=NUMPAD7 */
	{ 0, 9}, { 0,10}, { 0,11}, { 0,12},	/* 68=NUMPAD8 69=NUMPAD9 6a=MULTIPLY 6b=ADD */
	{ 0, 0}, { 0,13}, { 0,14}, { 0,15},	/* 6d=SUBTRACT 6e=DECIMAL 6f=DIVIDE */
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0,38}, { 0,34},	/* ba=; bb== */
	{ 0,33}, { 0,35}, { 0,32}, { 0,31},	/* bc=, bd=- be=. bf=/ */
	{ 0,37}, { 0, 0}, { 0, 0}, { 0, 0},	/* c0=` */
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0, 0},
	{ 0, 0}, { 0, 0}, { 0, 0}, { 0,26},	/* db=[ */
	{ 0,30}, { 0,27}, { 0,36}, { 0, 0},	/* dc=BSLASH dd=] de=' */
};

/* default key of each button */
static KEYDEFAULT key_defaults[NUM_BUTTONS] = {
	{  0, 0 },
	{  1, VK_NUMPAD0 },
	{  0, 0 },
	{  3, VK_NUMPAD2 },
	{  0, 0 },
	{  5, VK_NUMPAD6 },
	{  6, VK_NUMPAD3 },
	{  7, VK_NUMPAD1 },
	{  8, VK_NUMPAD4 },
	{  0, 0 },
	{  0, 0 },
	{  0, 0 },
	{ 12, VK_RETURN },
	{ 13, VK_UP },
	{ 14, VK_RIGHT },
	{ 15, VK_DOWN },
	{ 16, VK_LEFT },
	{ 17, 'H' },
	{  0, 0 },
	{ 19, 'M' },
	{  0, 0 },
	{ 21, 'U' },
	{ 22, 'J' },
	{ 23, 'G' },
	{ 24, 'T' },
	{  0, 0 },
	{  0, 0 },
	{  0, 0 },
	{ 28, VK_SPACE },
	{ 29, 'W' },
	{ 30, 'D' },
	{ 31, 'S' },
	{ 32, 'A' },
};

/* saved in mk3w.cfg */
void WriteKeymap(int fd)
{
	write(fd, keymap, sizeof(keymap));
}

void ReadKeymap(int fd)
{
	read(fd, keymap, sizeof(keymap));
}

/* the key mapped to a button, 0 if none */
int GetButtonMapkey(int buttonix)
{
	KEYMAP *k = keymap;
	int vk;

	if (buttonix != 0)
		for (vk = 0; vk < NUM_KEYS; k++, vk++)
			if (k->button == buttonix)
				return vk;
	return 0;
}

/* map a key to a button. Returns the button the key had (whose
   default key gets it back if that key is free), or -1 when nothing changed. */
int SetButtonMapkey(int buttonix, int mapchar)
{
	int oldkey = GetButtonMapkey(buttonix);
	int oldbutton;
	int vk;

	if (oldkey == mapchar)
		return -1;
	if (keymap[mapchar].name == 0)
		return -1;
	oldbutton = keymap[mapchar].button;
	keymap[mapchar].button = buttonix;
	keymap[oldkey].button = 0;
	if (oldbutton != 0)
	{
		vk = key_defaults[oldbutton].vk;
		if (keymap[vk].button == 0)
			keymap[vk].button = oldbutton;
	}
	return oldbutton;
}

/* a button left without a key gets its default key back if that
   key is free. Returns 1 if the default key is free. */
int CheckForDefautNoKeys(int buttonix)
{
	int vk;

	if (buttonix == 0)
		return 0;
	vk = key_defaults[buttonix].vk;
	if (keymap[vk].button == 0)
	{
		if (GetButtonMapkey(buttonix) == 0)
			keymap[vk].button = key_defaults[buttonix].button;
		return 1;
	}
	return 0;
}

void SetDefaultKeymap(void)
{
	KEYMAP *k;
	KEYDEFAULT *d;

	for (k = keymap; k < &keymap[NUM_KEYS]; k++)
		k->button = 0;
	for (d = key_defaults; d < &key_defaults[NUM_BUTTONS]; d++)
		keymap[d->vk].button = d->button;
}

/* the name of a key, NULL if it cannot be mapped */
char *GetKeyString(int key)
{
	int name;

	if ((u_int)key >= NUM_KEYS)
		return NULL;
	name = keymap[key].name;
	if (name == KEYNAME_CHAR)
	{
		key_char[0] = key;
		key_char[1] = 0;
		return key_char;
	}
	return key_names[name];
}

/* name of the key that is the player's START */
char *MapStartKeyToString(int player)
{
	int button = BUTTON_P1START;
	KEYMAP *k = keymap;
	int vk;

	if (player == 1)
		button = BUTTON_P2START;
	for (vk = 0; vk < NUM_KEYS; k++, vk++)
		if (k->button == button)
			return GetKeyString(vk);
	return NULL;
}

void ClearKeyBuffer(void)
{
	key_pad_bits = 0;
	last_key = 0;
}

/* the last key pressed (0 if none since the last call) */
int GetKeyNowait(void)
{
	int key = last_key;

	last_key = 0;
	return key;
}

/* pad bits of a key when remapping is off */
static u_long DefaultKeyBit(int vk)
{
	switch (vk)
	{
		case VK_LEFT:
			return PAD_LEFT;
		case VK_UP:
			return PAD_UP;
		case VK_RIGHT:
			return PAD_RIGHT;
		case VK_DOWN:
			return PAD_DOWN;
		default:
			return PAD_START | PAD_TRIANGLE;
	}
}

/* WM_KEYDOWN / WM_KEYUP from Win95_Pump */
void Win95_KeyEvent(int down, int vk, long keydata)
{
	u_long bit;

	if ((u_int)vk >= NUM_KEYS)
		return;
	if (remap_input)
	{
		bit = keymap[vk].button;
		if (bit != 0)
			bit = 1 << (bit - 1);
	}
	else
		bit = DefaultKeyBit(vk);
	if (gstate == GS_AMODE && vk == VK_SPACE)
		bit = PAD_START;
	if (down)
	{
		last_key = vk;
		key_pad_bits |= bit;
		return;
	}
	key_pad_bits &= ~bit;
}
