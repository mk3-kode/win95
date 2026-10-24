# Building Mortal Kombat 3 (Windows 95)

This project builds `mk3w.exe`, the Windows 95 release of Mortal Kombat 3,
from source with Visual C++ 2.0, the compiler retail was built with. The result
is the same size as retail and byte-identical except for 233 bytes in one
function (see README.md, Status).

The repository holds only code, build scripts and documentation. You supply:

1. the tools listed below (none are included);
2. your own copy of the retail `mk3w.exe`. The build takes from it everything
   that is not source: the contents of the 39 original objects the game links
   from its `ASM` folder (38 data objects: art and animation tables, palettes,
   text; and `CONTEXT.OBJ`, a small assembly object), and the program's icon.

## 1. Requirements

The build runs on Windows, Linux and macOS. All paths below are relative to
the project folder; they are written with `\`, which is `/` on Linux and
macOS.

| What | Version | Where it goes |
|---|---|---|
| Windows 10/11, Linux (x86-64) or macOS | tested: Windows 11, Ubuntu 26.04 | |
| Python | 3.10 or later (tested 3.14) | on `PATH` as `python` (Windows) or `python3` |
| [Wine](https://www.winehq.org) (Linux and macOS only) | tested: 10.0 (Ubuntu's `wine` package) | `wine` on `PATH`, or set `MK3_WINE` |
| Microsoft Visual C++ 2.0 (the files listed below) | 32-bit edition, 2.00.4244 (1994) | `MSVC20\BIN`, `MSVC20\INCLUDE`, `MSVC20\LIB` |
| DirectX 2 SDK (the files listed below) | MSDN Development Platform, July 1996, disc 1 | `DXSDK\INC`, `DXSDK\LIB` |
| Win32 SDK (the files listed below) | July 1996 | `WIN32SDK\INCLUDE`, `WIN32SDK\LIB` |
| JWasm | 2.20 (Windows build) | `TOOLS\JWasm.exe` |
| The retail `mk3w.exe` | US release (linked 1996-07-05) | any folder |

Each tool folder holds a `REQUIRED.TXT` listing the files that belong in it,
with their sizes and sha256s. The build uses only those files and was verified
with exactly these and nothing else in the tool folders.

### Visual C++ 2.0 (`MSVC20\`)

From the Visual C++ 2.0 CD's `MSVC20` folder. These 10 files of its `BIN`:

| File | Size | sha256 |
|---|---:|---|
| `MSVC20\BIN\C1.EXE` (C front end) | 318464 | `35ee6187db994beb90d1028df9dbd332fc308f5f687cfb01d0d8364b349cf361` |
| `MSVC20\BIN\C1XX.EXE` (C++ front end) | 673792 | `3e414c1148884ec9945c81fb8445fb42fb8940102a39ed5777fe4dfbe13adb97` |
| `MSVC20\BIN\C2.EXE` (code generator) | 517632 | `f28504956bc659c81853a67cbfaa34dfe6bfcd56a7218c8f3d648f266e0fd8f7` |
| `MSVC20\BIN\CL.EXE` (compiler driver, C/C++ 9.00) | 33280 | `6e8a2add9b8c8ff8ec02be83ede80e142fbae402cfefeba42b88a28263692678` |
| `MSVC20\BIN\CVTRES.EXE` | 24768 | `f4e5f6c128ab2110daeca08ded5dcc89ac2ec4d03f2f5a5332160fc170fe20ed` |
| `MSVC20\BIN\DBI.DLL` | 67584 | `d8c6180a72ef272e42b0f393ba1f2030c56d7d3ce301280e21f142b48f8cb2ae` |
| `MSVC20\BIN\LINK.EXE` (LINK 2.50) | 420352 | `33471e01276cd80141e32610f531fdae2ddec4f5ded4870ce2fc52b69c8e7dd7` |
| `MSVC20\BIN\MSVCRT20.DLL` | 243200 | `e93215473c8dda1add068bbeb7f0f168084ab2aff57f0a367176884a282eb55e` |
| `MSVC20\BIN\RC.EXE` | 16336 | `294e6f3ec5b6cffb03f9494f1ae35ef43dd1fe660f00111e7a8916edc77032c5` |
| `MSVC20\BIN\RCDLL.DLL` | 97056 | `4fa6bdd7f60749d54a57e592ae0505914dd3a5cf3816cbc8bfe04be6e02c5742` |

these 7 of its `LIB`:

| File | Size | sha256 |
|---|---:|---|
| `MSVC20\LIB\ADVAPI32.LIB` | 201884 | `8b5bbe79ed4c8593d2caba4132fe19f34409efe2a3982a28ebc5860353b13bed` |
| `MSVC20\LIB\GDI32.LIB` | 221476 | `9ee676b1a75d9be4157b662bdf18fe9583893f6711b22f0ec2e2f66925d99b61` |
| `MSVC20\LIB\KERNEL32.LIB` | 427422 | `30d6729787d58b5aaefe0cd7e4cfbc55725e9711207dfa17325f7c99afe57c55` |
| `MSVC20\LIB\LIBC.LIB` | 734716 | `5b327450b4a5d9e3859d953d3fee1e35fdea250aa5fa5e8f33c866148092c64c` |
| `MSVC20\LIB\MSACM32.LIB` | 30290 | `53740762b61c653e780a747b777ccb88cc8689e0ceaef1d32df2dbe6a7680731` |
| `MSVC20\LIB\OLDNAMES.LIB` | 56646 | `0ab8b877b49fa2666aeace7e71748d9c9cdb63ed024dcf5e9baffd02b8afe250` |
| `MSVC20\LIB\USER32.LIB` | 374604 | `763cedcbd804fcd1f4cc5e167f87a1e50e664b20b195d7bd5bb6166287233c5f` |

and 15 headers of its `INCLUDE` (the C run-time headers, `SYS\STAT.H`,
`WINPERF.H` and `WINSVC.H`; see `MSVC20\INCLUDE\REQUIRED.TXT`).

- **C2.EXE** is the code generator of the boxed Visual C++ 2.0 (dated
  1994-08-27). Visual C++ 2.1 and 2.2 have another one, whose code differs from
  retail's in more places.
- **MSVCRT20.DLL** must be the one from the Visual C++ 2.0 CD (its `BIN` and
  `REDIST` hold the same file). LINK sorts the import thunks with this DLL's
  `qsort`; the `MSVCRT20.DLL` of today's Windows (and Wine's own) forwards to
  `msvcrt.dll`, whose `qsort` orders equal keys differently, which gives another
  import table. The build runs LINK next to this copy (and, under Wine, with
  `WINEDLLOVERRIDES=msvcrt20=n`).
- **Optional:** the message texts `C1.ERR`, `C23.ERR` and `LINK.ERR` from the
  same `BIN` folder (listed in `MSVC20\BIN\REQUIRED.TXT`). The tools read them
  only to print a warning or an error; without them, a failing step reports
  that the message file is missing instead of the message itself.

### DirectX 2 SDK (`DXSDK\`) and Win32 SDK (`WIN32SDK\`)

The layer was built with these headers ahead of Visual C++ 2.0's own.

| File | Size | sha256 |
|---|---:|---|
| `DXSDK\INC\DDRAW.H` | 110021 | `7980004a2431b24e375b2e90a094a91e95aab9589a23ca90c1bda5e8ba809ca7` |
| `DXSDK\INC\DSOUND.H` | 13417 | `bd8ebbba4268f1b8e9688547811d763dd2ac40e22dfb8e3c44a65f9f3f4e8d12` |
| `DXSDK\LIB\DSOUND.LIB` | 3446 | `603c0381ef47a8a093055bcb7359369f34564382af90488bcb97c32c0d7c38db` |
| `WIN32SDK\LIB\WINMM.LIB` | 183766 | `95b907366711ff88b539d83fca956a53841724660a4c09923c450f9c688a41a2` |

- **DirectX 2 SDK**: on the MSDN Development Platform disc 1 of July 1996, in
  `DIRECTX2\SDK\INC` and `DIRECTX2\SDK\LIB`.
- **Win32 SDK, July 1996**: its `MSTOOLS\INCLUDE` (50 headers, `WINDOWS.H` and
  what it includes; see `WIN32SDK\INCLUDE\REQUIRED.TXT`) and
  `MSTOOLS\LIB\I386\WINMM.LIB`. Retail's import hints for `WINMM.DLL` are this
  library's, not Visual C++ 2.0's.

### The other tools

| File | Size | sha256 |
|---|---:|---|
| `TOOLS\JWasm.exe` | 312320 | `ce95d1a4e83b2cba74fadffdf1f009438e668a30f6314c93d7b044e55be7ecee` |

- **JWasm 2.20** (<https://github.com/Baron-von-Riedesel/JWasm/releases>, the
  Windows binary) assembles `WIN95\BLIT.ASM` and the four `WIN95\FLIC\*.ASM`
  files. They are written for Phar Lap's 386|ASM; the build assembles a
  converted copy.
- **Wine** runs Visual C++ 2.0 and JWasm on Linux and macOS. Any Wine that
  runs 32-bit Windows programs works (on macOS: one with 32-bit support, such as
  the `wine-stable` Homebrew cask; macOS has not been tested). The build uses
  the default Wine prefix (`~/.wine`) or `WINEPREFIX`, and makes the prefix on
  the first run.

### The retail executable

`mk3w.exe` of the US release, as on the retail CD:

- size 2403328 bytes
- sha1 `e9ed6a82752a1ff75766f74c04bb030fa8f2c022`

The build checks the sha1 and stops on any other file.

## 2. Folder layout after setup

```
build.py                    the build
BUILDING.md, README.md
KODE\                       game source (C modules and headers)
WIN95\                      the Windows 95 platform layer (C, assembly, resources)
ASM\                        OVERLAY.C and its two headers
SOUND\                      the sound bank loader (BUILDSND.C, WAVE.C)
MATCH\                      link order, padding, data-object skeletons
MSVC20\BIN, INCLUDE, LIB    Visual C++ 2.0 files listed in section 1    <- you add these
DXSDK\INC, LIB              DirectX 2 SDK files                          <- you add these
WIN32SDK\INCLUDE, LIB       Win32 SDK files                              <- you add these
TOOLS\                      JWasm.exe                                    <- you add this
```

The tool folders already exist in a fresh checkout, each with its
`REQUIRED.TXT`.

The project folder may have spaces in its path. Visual C++ 2.0 cannot run from
one (CL starts its passes with its own folder in the command line, unquoted), so
the build runs a copy of the tools from `TMP\BIN`, from that folder's short
(8.3) name on Windows when it has spaces, or else from the system's temporary
folder (`TEMP` on Windows, `TMPDIR` elsewhere).

## 3. Build

From the project folder:

```
python -I build.py "<path to the retail mk3w.exe>"     (Windows)
python3 -I build.py "<path to the retail mk3w.exe>"    (Linux, macOS)
```

`build.py` runs these steps and stops at the first failure:

1. **Files from the retail executable.** It fills the 39 data objects'
   skeletons (`MATCH\SKELETONS`) from `mk3w.exe` and checks each against the
   original object's sha256. `MK3W.ICO`, the icon `WIN95\MK3W.RC` includes, is
   extracted from `mk3w.exe`'s icon resources (and checked) in step 4.
2. **Compile.** The 81 C files (65 game modules, the 13 layer files,
   `OVERLAY.C`, `BUILDSND.C` and `WAVE.C`) with retail's switches: `-Ox -G5`
   for the game, `-Od` (as C++) or `-Ox -G5` per layer file, with the
   symbol-count padding of `MATCH\PADDING.json` applied to copies: in
   `TMP\pad` for the modules, `TMP\w\sound` for the two sound files, and
   `TMP\w\emulator\inc` for `STDSTUFF.H` (the layer is compiled and included
   from that copy of `WIN95\`).
3. **Assemble** the five assembly files with JWasm.
4. **Resources.** RC compiles `WIN95\MK3W.RC`, with the icon from your
   `mk3w.exe`, to `MK3W.RES`.
5. **Link.** LINK 2.50 links the objects in retail's order
   (`MATCH\MK3W.LNK`) and `MK3W.RES` (LINK converts it with CVTRES, which gives
   retail's read-only resource section) with the import libraries in retail's
   order. The link and resource time stamps are set to retail's.
6. **Compare.** It compares the result with your `mk3w.exe` section by
   section.

A build takes under a minute on Windows and a few minutes under Wine. The
result is `OUT\mk3w.exe` and its map file `OUT\mk3w.map`. The comparison
prints:

```
OUT/mk3w.exe: 2403328 bytes (retail 2403328), sha1 a3f081ae339f7062e6b0d8cea6191802187d79a5
  .text: 223 bytes differ, 0x456eb4-0x456ff7
  .reloc: 10 bytes differ, 0x7857b6-0x7857c8
233 bytes differ from retail
```

These are the 233 bytes of `FlicReadUncompFrame` (README.md, Status). Any
other difference is not part of a normal build.

Environment variables:
- `MK3_WINE`: the Wine executable (Linux and macOS; default `wine` on `PATH`).
- `WINEPREFIX`: the Wine prefix (default `~/.wine`).

## 4. Troubleshooting

- **"is not the retail mk3w.exe"**: the file is another version (a patch, a
  prototype or another region's release). Compare it with the size and sha1 in
  section 1.
- **More bytes differ than the 233 above**: check that the tools match the
  checksums in section 1 (`REQUIRED.TXT` in each folder). The most likely
  causes are a `MSVCRT20.DLL` that is not Visual C++ 2.0's (import table
  differences), a Visual C++ 2.1 or 2.2 `C2.EXE` (code differences all over),
  or other SDK headers or libraries.
- **`fatal error C1007`**: CL ran from a folder with spaces; set `TEMP`
  (Windows) or `TMPDIR` to a folder without spaces.
- **Wine not found**: install it (Ubuntu/Debian: `apt install wine`; macOS:
  `brew install --cask wine-stable`) or set `MK3_WINE`.
- **Files that suddenly vanish or come back as 0 bytes**: synchronisation
  tools (OneDrive, Dropbox) can interfere with the tools writing files. Pause
  synchronisation or build outside a synchronised folder.

## 5. What comes from where

| Built from this project's source | Taken from your retail mk3w.exe |
|---|---|
| all of the game's code (65 C modules) and the platform layer (13 C files, 5 assembly files) | the contents of the 39 original objects (`MATCH\SKELETONS`): art and animation tables, palettes, text, and `CONTEXT.OBJ` |
| `OVERLAY.C` (the overlay table), `BUILDSND.C` and `WAVE.C` | the icon (`MK3W.ICO`, from the icon resources) |
| the resources (`WIN95\MK3W.RC`) | |
| the import table, relocations, headers (by LINK) | |

A skeleton is one of the original objects (Phar Lap OMF, converted to COFF as
LINK 2.50's own conversion does) with its section contents zeroed, except the 4-byte
words at relocation sites; `MATCH\SKELETONS\skeletons.json` gives, for each,
where its sections sit in `mk3w.exe` and the original object's sha256.
