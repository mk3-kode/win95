/******************************************************************************
 JOYSTICK.C - Windows joysticks as PlayStation pads.

 Reconstructed from mk3w.exe (retail), compiled without
 optimisation, unlike the code around it.

 Each joystick has seven button assignments (the KONTROLS screen changes the
 joystick buttons of each); the stick gives the direction bits.
******************************************************************************/
/* retail compiled this file as C++ (its uninitialised globals are .bss definitions, not
   common symbols): the C interfaces of DirectX and C names for everything */
#define CINTERFACE
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include "emulator.h"
#include "win95.h"

#define JOY_ASSIGNS		7

/* 6 bytes each: WriteButtonmap writes 0x54 bytes for the 14 */
#pragma pack(2)
typedef struct {
	u_long joybuttons;		/* JOY_BUTTONn bits that must all be down */
	u_short padbit;			/* PAD_* bit they give */
} JOYASSIGN;

/* joy_assign and initialised data nothing refers to (in retail's .data: two ints
   and seven 6-byte {0x08f5, bit} records, names and use not known). Visual C++ puts objects of
   more than 4 bytes on 8, and this data follows joy_assign's 0x54 bytes on 4, so it was one
   object with joy_assign. */
typedef struct {
	u_short value;
	u_long mask;
} UNUSED6;

static struct {
	JOYASSIGN assign[2][JOY_ASSIGNS];
	int unused_int0;
	int unused_int1;
	UNUSED6 unused_bits[7];
} joy_data = {
	{
		{ { 0, PAD_START }, { 1, PAD_SQUARE }, { 2, PAD_TRIANGLE }, { 4, PAD_CROSS },
		  { 8, PAD_CIRCLE }, { 5, PAD_L1 }, { 10, PAD_L2 } },
		{ { 0, PAD_START }, { 1, PAD_SQUARE }, { 2, PAD_TRIANGLE }, { 4, PAD_CROSS },
		  { 8, PAD_CIRCLE }, { 5, PAD_L1 }, { 10, PAD_L2 } },
	},
	0, 1,
	{ { 0x08f5, 0x02 }, { 0x08f5, 0x04 }, { 0x08f5, 0x08 }, { 0x08f5, 0x10 },
	  { 0x08f5, 0x20 }, { 0x08f5, 0x40 }, { 0x08f5, 0 } },
};
#define joy_assign joy_data.assign
#pragma pack()

static u_char joy_defaults[2][JOY_ASSIGNS] = {
	{ 0, 1, 2, 4, 8, 5, 10 },
	{ 0, 1, 2, 4, 8, 5, 10 },
};

static u_long joy_native0;		/* direction bits << 16 | joystick buttons, joystick 1 */
static u_long joy_native1;		/* joystick 2 (retail has them as two variables) */

/* saved in mk3w.cfg */
void WriteButtonmap(int fd)
{
	write(fd, joy_assign, sizeof(joy_assign));
	return;
}

void ReadButtonmap(int fd)
{
	read(fd, joy_assign, sizeof(joy_assign));
	return;
}

void SetDefaultButtonmap(void)
{
	int i;

	for (i = 0; i < 2 * JOY_ASSIGNS; i++)
		joy_assign[0][i].joybuttons = joy_defaults[0][i];
	return;
}

/* the joystick buttons of a pad button (1-16 pad 1, 17-32 pad 2) */
u_long GetBitButtons(int buttonix)
{
	JOYASSIGN *ja;
	int i;
	u_long mask;

	ja = joy_assign[0];
	if (buttonix > 16)
	{
		buttonix -= 16;
		ja += JOY_ASSIGNS;
	}
	mask = 1 << (buttonix - 1);
	for (i = 0; i < JOY_ASSIGNS; i++)
	{
		if (ja->padbit == mask)
			return ja->joybuttons;
		ja++;
	}
	return 0;
}

/* returns the joystick buttons that changed */
u_long SetBitButtons(int buttonix, u_long bits)
{
	/* names chosen for Visual C++'s stack layout and operand order (their hash
	   buckets: padb 3, oldbits 6, ja 9) */
	JOYASSIGN *ja;
	u_long oldbits;				/* the loop counter, then the old buttons */
	u_long padb;

	ja = joy_assign[0];
	if (buttonix > 16)
	{
		buttonix -= 16;
		ja += JOY_ASSIGNS;
	}
	padb = 1 << (buttonix - 1);
	for (oldbits = 0; oldbits < JOY_ASSIGNS; oldbits++)
	{
		if (ja->padbit == padb)
		{
			oldbits = ja->joybuttons;
			ja->joybuttons = bits;
			return oldbits ^ bits;
		}
		ja++;
	}
	return 0;
}

/* the raw state read by the last ReadJoystick */
u_long JoyGetNative(int id)
{
	if (id != 0)
		return joy_native1;
	return joy_native0;
}

/* pad bits of joystick id (pad 2 bits are in the top half) */
u_long ReadJoystick(u_int id)
{
	/* Visual C++ lays locals out by the hash bucket of their names: these names
	   give retail's stack layout; their declaration order gives
	   the operand order of the compares (stop/entry, wbuttons/chord) */
	u_long pads;			/* bucket 4 */
	u_long wbuttons;		/* 7 */
	JOYASSIGN *stop;		/* 14 */
	JOYASSIGN *entry;		/* 10 */
	u_long chord;			/* 15: the buttons an entry needs */
	u_long *native;			/* 9 */
	JOYINFO joyinfo;		/* 11 */

	if (id == 0)
	{
		pads = 0;
		entry = joy_assign[0];
		native = &joy_native0;
	}
	else
	{
		pads = 1;
		entry = joy_assign[0] + JOY_ASSIGNS;
		native = &joy_native1;
	}
	pads = joyGetPos(id, &joyinfo);		/* JOYERR_NOERROR (0): no pad bits yet */
	if (pads != JOYERR_NOERROR)
		return 0;
	if (joyinfo.wXpos > 0xc000)
		pads |= PAD_RIGHT;
	else if (joyinfo.wXpos < 0x4000)
		pads |= PAD_LEFT;
	if (joyinfo.wYpos > 0xc000)
		pads |= PAD_DOWN;
	else if (joyinfo.wYpos < 0x4000)
		pads |= PAD_UP;
	wbuttons = joyinfo.wButtons;
	*native = pads << 16 | (wbuttons & 0x0fffffff);
	if (wbuttons != 0)
	{
		stop = entry + JOY_ASSIGNS;
		for (; stop > entry; entry++)
		{
			chord = entry->joybuttons;
			if (chord != 0 && (chord & wbuttons) == chord)
				pads |= entry->padbit;
		}
	}
	if (id != 0)
		pads <<= 16;
	return pads;
}

#ifdef __cplusplus
}
#endif
