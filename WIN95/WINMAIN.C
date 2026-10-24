/******************************************************************************
 WINMAIN.C - start-up and shut-down of the Windows 95 version.

 Reconstructed from mk3w.exe (retail).

 WinMain finds the game CD, loads DirectDraw and DirectSound if they are
 installed, reads mk3w.cfg, creates the window, sets the display mode and
 runs the game's main() (MKINIT.C). mk3w.cfg is written back on exit.

 mk3w.cfg:	RECT window;  int display_mode;  ten 2-byte game options;
			the joystick buttons (JOYSTICK.C), the key map (KEYBOARD.C),
			the high score initials (MKMAIN.C).
******************************************************************************/
/* retail's JOYCAPS here is the 0x48-byte Windows 3.1 layout (WINVER < 0x400; the Win95
   fields are missing), while the other layer files use Win95 definitions (WM_SIZING) */
#ifdef _MSC_VER			/* (mingw-w64's headers need WINVER >= 0x400) */
#define WINVER 0x030a
#endif
/* retail compiled this file as C++ (its uninitialised globals are .bss definitions, not
   common symbols): the C interfaces of DirectX and C names for everything */
#define CINTERFACE
#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "emulator.h"
#include "win95.h"
#include "winddraw.h"
#include "winddsnd.h"
#include "psxcd.h"

#define CD_LABEL		"MK3PCCDROM"
#define CD_CHECKFILE	"%c:\\attract\\controlp.bin"
#define REG_KEY			"\\SOFTWARE\\Mortal Kombat III"

#define ERROR_NO_WINDOW	"Error: DirectX is not working correctly\nNo full screen mode avaible.\n" \
						"Error: You are not running in High Color (16bit) mode\n" \
						"Unable to run MK3 in a window\nRecomend Re-Installing MK3\nor\n" \
						"To run MK3 in a window change the color palette in the\n" \
						"display properties dialog to High Color (16Bit)\n"
#define WARNING_NO_DX	"Error: DirectX is not working correctly\n" \
						"Without DriectX game preformance may suffer\n" \
						"and you will be unable to run the game fullscreen.\n" \
						"Recommend Re-Installing MK3"

/* the globals of this file are defined in WINDDRAW.C: retail has them in that object's .bss */

/* game options saved in mk3w.cfg (MKGLOBAL.C) */
extern short cmos_diff, f_no_vs, f_no_clock, f_shang_morph, f_no_violence;
extern short f_no_blood, f_stereo, f_no_pan, f_music, f_no_sfx;

extern int main(void);						/* MKINIT.C */
extern void dipinit(void);					/* MKINIT.C */
extern void ReadHighScores(int fileno);		/* MKMAIN.C */
extern void WriteHighScores(int fileno);

/* the path of a game file */
void BuildFullPath(char *outpath, char *subpath)
{
	wsprintf(outpath, "%s%s", g_datadir, subpath);
	return;
}

/* g_InstallDir and g_datadir (the directory the game was started
   from, then the CD), the install path in the registry. Returns 0 if the
   player gives up inserting the CD. */
static int SetupInstallPath(void)
{
	int have = 0;
	WIN32_FIND_DATA data;
	DWORD fsflags;
	DWORD maxlen;
	int n;
	HKEY hkey;
	LONG err;
	DWORD created;
	char path2[260];
	u_long drvbits;
	char *exe;
	char *cl;
	int result;
	char drivename[260];
	char volume[260];

	cl = GetCommandLine();
	exe = strstr(cl, "mk3w.exe");
	exe = strstr(cl, "xxxxx");
	if (exe == NULL)
	{
		GetCurrentDirectory(260, g_datadir);
		strcat(g_datadir, "\\");
		strcpy(g_InstallDir, g_datadir);
	}
	else
	{
		*exe = 0;
		strcpy(g_datadir, cl);
		strcpy(g_InstallDir, g_datadir);
	}
	if (GetDriveType(g_datadir) != DRIVE_CDROM && GetDriveType(g_datadir) != DRIVE_REMOVABLE
		&& GetDriveType(g_datadir) != DRIVE_RAMDISK)
	{
		err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REG_KEY, 0, "MK3", 0, KEY_ALL_ACCESS, NULL, &hkey, &created);
		err = RegSetValueEx(hkey, "path", 0, REG_SZ, (BYTE *)g_datadir, strlen(g_datadir));
		RegFlushKey(hkey);
		RegCloseKey(hkey);
	}
	drvbits = GetLogicalDrives();
	do
	{
		for (n = 0; n < 32; n++)
		{
			if (drvbits & (1 << n))
			{
				wsprintf(drivename, "%c:\\", 'a' + n);
				if (GetDriveType(drivename) == DRIVE_CDROM)
				{
					volume[0] = 0;
					GetVolumeInformation(drivename, volume, 260, NULL, &maxlen, &fsflags, NULL, 0);
					strupr(volume);
					if (strcmp(volume, CD_LABEL) == 0)
					{
						have = 1;
						wsprintf(path2, CD_CHECKFILE, 'a' + n);
						if (FindFirstFile(path2, &data) != NULL)
							strcpy(g_datadir, drivename);
						break;
					}
				}
			}
		}
		if (!have && (result = MessageBox(NULL, "Please insert the Mortal Kombat III CD", "MKIII Win95",
										  MB_RETRYCANCEL)) == IDCANCEL)
			return 0;
	} while (!have);
	return 1;
}

static HMODULE LoadDDrawLibrary(void)
{
	char path[260];

	GetSystemDirectory(path, 260);
	strcat(path, "\\ddraw.dll");
	return LoadLibrary(path);
}

static HMODULE LoadDSoundLibrary(void)
{
	char path[260];

	GetSystemDirectory(path, 260);
	strcat(path, "\\dsound.dll");
	return LoadLibrary(path);
}

static BOOL RegisterMainWindowClass(HINSTANCE hInst)
{
	WNDCLASS wc;

	wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(104));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "Display";
	return RegisterClass(&wc);
}

/* KONTROLS screen, Shift lets the arrow keys be mapped */
int ShiftKeyState(void)
{
	return CallOnBigStack(GetKeyState, VK_SHIFT) & 0x8000;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int outfd;
	char path2[260];
	int bad;
	RECT isect;
	RECT screen;
	int file;
	char cfgname[260];
	JOYCAPS jcs;
	u_int raster = 0;
	HDC display;
	int code;
	OSVERSIONINFO vi;

	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);
	if (vi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS && vi.dwMajorVersion < 4)
	{
		MessageBox(NULL, "The Windows version of Mortal Kombat III can only run under Win95",
				   "MKIII ERROR", MB_ICONEXCLAMATION);
		return 0;
	}
	if (!SetupInstallPath())
		return 0;
	hInstApp = hInstance;
	iWinExit = 0;
	g_JoyCount = 0;
	if (joyGetDevCaps(0, &jcs, sizeof(jcs)) == JOYERR_NOERROR)
		g_JoyCount = 1;
	if (joyGetDevCaps(1, &jcs, sizeof(jcs)) == JOYERR_NOERROR)
		g_JoyCount = 2;
	hDSound = LoadDSoundLibrary();
	if (hDSound != NULL)
		lpDirectSoundCreate = GetProcAddress(hDSound, "DirectSoundCreate");
	hDDrawInst_ = LoadDDrawLibrary();
	if (hDDrawInst_ != NULL)
	{
		pDirectDrawCreate = GetProcAddress(hDDrawInst_, "DirectDrawCreate");
		ScrMode1 = DISPLAY_DD320;
		g_DIBFlags = 4;
	}
	else
	{
		ScrMode1 = DISPLAY_WINDOW;
		g_DDStatus = -1;
		g_DIBFlags = -2;
	}
	/* a window needs a 16-bit desktop without a palette */
	display = CreateIC("DISPLAY", NULL, NULL, NULL);
	if (display != NULL)
	{
		raster = GetDeviceCaps(display, RASTERCAPS) & RC_PALETTE;
		if (raster || GetDeviceCaps(display, BITSPIXEL) != 16)
			force_full_screen = 1;
		DeleteDC(display);
	}
	if (ScrMode1 == DISPLAY_WINDOW)
	{
		if (force_full_screen)
		{
			MessageBox(NULL, ERROR_NO_WINDOW, "MKIII ERROR", MB_ICONEXCLAMATION);
			goto quit;
		}
		else
		{
			MessageBox(NULL, WARNING_NO_DX, "MKIII WARNING", MB_ICONEXCLAMATION);
			g_DIBFlags = 4;
		}
	}
	if (!EmuInit())
		return 0;

	strcpy(cfgname, g_InstallDir);
	strcat(cfgname, "mk3w.cfg");
	file = open(cfgname, O_BINARY | O_RDWR);
	dipinit();
	xFrame = GetSystemMetrics(SM_CXFRAME) * 2;
	yFrame = GetSystemMetrics(SM_CYFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) - 1;
	WindowRect.left = 0;
	WindowRect.top = 0;
	WindowRect.right = xFrame + SCREEN_W;
	WindowRect.bottom = yFrame + SCREEN_H;
	if (file != -1)
	{
		read(file, &WindowRect, sizeof(WindowRect));
		read(file, &ScrMode1, sizeof(ScrMode1));
		read(file, &cmos_diff, 2);
		read(file, &f_no_vs, 2);
		read(file, &f_no_clock, 2);
		read(file, &f_shang_morph, 2);
		read(file, &f_no_violence, 2);
		read(file, &f_no_blood, 2);
		read(file, &f_stereo, 2);
		read(file, &f_no_pan, 2);
		read(file, &f_music, 2);
		read(file, &f_no_sfx, 2);
		ReadButtonmap(file);
		ReadKeymap(file);
		ReadHighScores(file);
		close(file);
	}

	/* keep the window on the desktop */
	GetWindowRect(GetDesktopWindow(), &screen);
	if (!IntersectRect(&isect, &WindowRect, &screen))
		OffsetRect(&WindowRect, -WindowRect.left, -WindowRect.top);
	else
	{
		bad = 0;
		if (isect.right < 30)
			bad = 1;
		if (isect.bottom < 30)
			bad = 1;
		if (screen.right - isect.left < 30)
			bad = 1;
		if (screen.bottom - isect.top < 30)
			bad = 1;
		if (bad)
			OffsetRect(&WindowRect, -WindowRect.left, -WindowRect.top);
	}
	if (!RegisterMainWindowClass(hInstance))
		return 0;
	g_hWndMain = CreateWindowEx(0, "Display", "Mortal Kombat III", WS_POPUP | WS_VISIBLE,
							0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	g_hWinDC = GetDC(g_hWndMain);
	if (force_full_screen && ScrMode1 < 5)
		ScrMode1 = DISPLAY_DD320;
	ScrMode1 = SetDisplayMode(ScrMode1);
	g_SavedMode = ScrMode1;
	if (ScrMode1 == -1)
	{
		if (ScrMode1 > 7)
		{
			ScrMode1 = SetDisplayMode(DISPLAY_DD320);
			g_SavedMode = ScrMode1;
		}
		if (ScrMode1 == -1)
		{
			if (force_full_screen)
			{
				MessageBox(NULL, ERROR_NO_WINDOW, "MKIII ERROR", MB_ICONEXCLAMATION);
				goto quit;
			}
			else
				MessageBox(NULL, WARNING_NO_DX, "MKIII WARNING", MB_ICONEXCLAMATION);
		}
		force_full_screen = 0;
		g_DDStatus = -1;
		g_DIBFlags = -2;
		ScrMode1 = SetDisplayMode(DISPLAY_WINDOW);
		g_SavedMode = ScrMode1;
	}
	g_hAccelerators = LoadAccelerators(hInstance, MAKEINTRESOURCE(101));
	code = main();
	if (code == 0 && code == -1)
		MessageBox(NULL, "Out of Memory\n\rIf you have other applications open\n\rtry closing them and try again",
				   "Win95 MKIII ERROR", MB_ICONEXCLAMATION);

quit:
	ReleaseDC(g_hWndMain, g_hWinDC);
	EmuFree();
	DestroyFrameDIB(hInstApp);
	DDRelease(hInstApp);
	if (hDDrawInst_ != NULL)
		FreeLibrary(hDDrawInst_);
	if (hDSound != NULL)
		FreeLibrary(hDSound);
	Win95_DeinitDirectSound();
	psxcd_exit();

	strcpy(path2, g_InstallDir);
	strcat(path2, "mk3w.cfg");
	outfd = open(path2, O_BINARY | O_CREAT | O_TRUNC | O_RDWR, S_IREAD | S_IWRITE);
	if (force_full_screen && ScrMode1 < 5)
		ScrMode1 = g_LastDDMode;
	if (outfd != -1)
	{
		write(outfd, &WindowRect, sizeof(WindowRect));
		write(outfd, &ScrMode1, sizeof(ScrMode1));
		write(outfd, &cmos_diff, 2);
		write(outfd, &f_no_vs, 2);
		write(outfd, &f_no_clock, 2);
		write(outfd, &f_shang_morph, 2);
		write(outfd, &f_no_violence, 2);
		write(outfd, &f_no_blood, 2);
		write(outfd, &f_stereo, 2);
		write(outfd, &f_no_pan, 2);
		write(outfd, &f_music, 2);
		write(outfd, &f_no_sfx, 2);
		WriteButtonmap(outfd);
		WriteKeymap(outfd);
		WriteHighScores(outfd);
		close(outfd);
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
