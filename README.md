# Mortal Kombat 3 - Windows 95 source

The source code of the Windows 95 release of Mortal Kombat 3 (Williams
Entertainment / GT Interactive, 1996), matched so that it rebuilds the retail
`mk3w.exe`: the game, the reconstructed Windows 95 platform layer (the
PlayStation library emulation, DirectDraw, DirectSound, FLIC playback, input)
and the resources.

This repository holds exactly the files the build reads, plus documentation:
no game data, no compiler, SDK or tool binaries. The build takes everything
that is not source from your own retail `mk3w.exe`. See
**[BUILDING.md](BUILDING.md)** for the tools and the steps.

```
python -I build.py "<path to the retail mk3w.exe>"     (Windows)
python3 -I build.py "<path to the retail mk3w.exe>"    (Linux, macOS)
```

## What is here

| Folder | Contents |
|---|---|
| `KODE\` | The game: its 65 C modules and the headers they include. |
| `WIN95\` | The reconstructed platform layer: 13 C files and their headers, `BLIT.ASM` and the FLIC decoder (`FLIC\`), and the resource script (`MK3W.RC`). |
| `ASM\` | `OVERLAY.C` (the overlay table) and its two headers. |
| `SOUND\` | `BUILDSND.C` and `WAVE.C` (the sound bank loader), `WAVE.H` and the sound list `MK3SOUND.LST`. |
| `MATCH\` | What the build needs besides the source: the link order (`MK3W.LNK`), the symbol-count padding (`PADDING.json`), and the skeletons of the 39 original objects the game links from its `ASM` folder (`SKELETONS\`), which `build.py` fills from your `mk3w.exe`. |
| `build.py` | The whole build. |
| `WHY_NOT_100_PERCENT.md` | Why 233 bytes still differ from retail (see Status). |

## Status

- `mk3w.exe` is the same size as retail, and every section, the headers, the
  imports, the resources and the data are byte-identical, except for 233 bytes:
  one register choice in `FlicReadUncompFrame` (`WIN95\FLIC\FLICFILE.C`) that
  Visual C++ 2.0 makes differently from retail's build (`dx` in retail, `ax`
  here, one byte shorter), which moves the rest of that function by one byte
  (223 bytes of `.text`, 10 of `.reloc`). Every other function is identical.
  The evidence points to a slightly different build of the compiler's code
  generator (C2.EXE) than the boxed Visual C++ 2.0's: no C form found makes this
  one choice come out as retail's.

See **[WHY_NOT_100_PERCENT.md](WHY_NOT_100_PERCENT.md)** for the details: the
difference, how the compiler arrives at it, what was tried, and the evidence
about the original compiler.

## Licensing

Mortal Kombat 3 is © Midway / Williams Entertainment. This repository does not
contain the game's data, Microsoft's compiler, libraries or SDKs, or
third-party tools.
