# Why the rebuilt mk3w.exe is not 100% byte-identical

Target: retail `mk3w.exe`, linked 1996-07-05 13:07 with LINK 2.50 (SHA-1 `e9ed6a82752a1ff75766f74c04bb030fa8f2c022`;
the same file as the leaked "Jul 5, 1996 prototype").
Toolchain used for the rebuild: Visual C++ 2.0 (C1/C2 9.00, LINK 2.50), real C source only (no `__asm _emit` of
retail bytes).

## Status

| | bytes |
|---|---|
| File size | 2,403,328 (same as retail) |
| Identical at the same offset (all sections) | 2,402,071 / 2,402,304 (99.99%) |
| Different | 233 |

All 233 bytes come from a single instruction in one function.

| Where | Bytes | Cause |
|---|---|---|
| `.text` 0x456eb4-0x456ff7 (`FlicReadUncompFrame`, `WIN95\FLIC\FLICFILE.C`) | 223 | one register choice (below) |
| `.reloc` | 10 | relocation entries inside that function shift by 1 byte |

Every other function, the whole `.data`, `.idata`, `.rsrc` contents and the section layout are byte-identical.

## The one difference

`FlicReadUncompFrame`, second "touch" loop (the loop that reads random bytes of the file buffer to keep it paged in):

```
retail                                   ours
mov  ecx, [read_pos]                     mov  ecx, [read_pos]
mov  edx, [file_buf]                     mov  edx, [file_buf]
add  ecx, edx                            add  ecx, edx
mov  cl, [ecx+eax]                       mov  cl, [ecx+eax]
movsx dx, cl          ; 66 0F BE D1      movsx ax, cl          ; 66 0F BE C1
cmp  dx, 0FFh         ; 66 81 FA FF 00   cmp  ax, 0FFh         ; 66 3D FF 00
```

The compiler gives the temporary that holds `c` a different register. The rest follows from that:

- **One byte shorter:** x86 has a 1-byte-shorter `cmp` for the accumulator (`cmp ax, imm`), so our function is 736
  bytes and retail's is 737.
- **Jumps shift:** the code after that point moves by one byte, which changes 8 jump displacements and the
  relocation entries inside the function.
- **The next function stays put:** the function's end-of-function alignment padding (`lea esp,[esp]`) absorbs the
  byte, so the next function starts at the same address and nothing after it changes.

## Why the source can't express it

The register comes from C2's (the code generator's) register allocator, not from anything that maps directly to C.
The allocator was traced with a debugger:

1. **Two passes:** C2 colours the graph first, then runs a per-block Pentium "retarget" pass. That pass moves a value
   off its register if that register was one of the last two registers written (two slots at 0x475f40 and
   0x4760b0, reset at every label).
2. **Confirmed with the debugger:** forcing `eax` into those slots just before the byte load makes our C2 produce
   retail's code exactly (score 0).
3. **What retail's code implies:** the compiler's input must have had an instruction writing `eax` in that block,
   right before the load, which C2 later deleted. A short-lived copy that C2 merges away has this effect. Retail's
   load order (`read_pos` before `file_buf`) must also hold at the same time.
4. **No C form found:** C1 (the front end) does no optimization; its intermediate language is a 1:1 image of the C.
   So the question is whether any C makes C2 build that state. No C form, declaration, ordering or context found
   so far does.

## What was tried, all with the result "retail order + `ax`" or worse:

| Experiment | Size |
|---|---|
| Local declaration orders and names, block-scope variables, dead stores, `volatile` / `extern` / `static` `touch` | exhaustive per category |
| Index and address forms, casts, mismatched load types, the reload in the condition, copies of `pos`, post-increment, inline accessors, bare expression statements | ~1,100 variants |
| Declaration types of every variable in loop 2 | 846 combinations |
| Loops written as `while` / `do` / `for`, `goto`, `switch`, structs, C++ | many |
| Symbol numbering: pads before the function, gaps between globals, all 17⁴ residues of the four loop-2 globals' C1 symbol ids (C2 hashes ids with period 17) | 83,521 compiles; 9,901 give `dx`, all with the wrong load order |
| C2 state carried over from earlier functions: every differing static dword forced at the function's start | 1,237 debugger runs; never changes the function |
| Single-byte mutations of the IL files (`ex`/`gl`/`sy`/`in`) | ~19,000 |
| decomp-permuter runs, scored with C2's own register trace | several runs of 200k-1.4M iterations; best still `ax` |
| Every CL option, every hidden C2 switch (`-nosched`, `-norisc`, `-sched#`, ...), `/Zi` `/Z7` `/Gy` ... | all |
| VC 2.x IDE default options: browse info `/FR` `/Fr`, `/W3 /GX /YX /ML`, `/O2` | 20 sets x 51 pads; `-Ox` sets identical to baseline, `/O2` a different loop shape |
| Other compilers: VC 2.1 / 2.2 (= VC 2.0 + VCFDIV patch), VC 4.0 | worse |

Pattern: our C2 produces `dx` only together with the wrong load order. It produces retail's load order only with
`ax`. It never produces both, which is what retail has.

## Most likely explanation: a different compiler build

The function was most likely compiled by a C2 build slightly different from the boxed VC 2.0 one. The evidence:

- **Every original build has `dx`:** the function appears in five builds, all linked with LINK 2.50 and all with
  `dx`: 1996-05-15, 06-03, 07-05 13:00 (a debug build), 07-05 13:07 (retail) and 07-13.
- **Two source revisions, same result:** the source changed between May 15 and June 3 (the key/joystick abort was
  added to the load loop), so it was recompiled, and both compiles gave `dx`. Our VC 2.0 gives `ax` for the May form
  too, at every pad from 0 to 50.
- **More than one toolchain on the machine:** the game was built under `D:\mk3p` (debug PDB path
  `D:\mk3p\vcpp-obj\MK3.pdb`). The same tree has VC 4.0 objects ("Compiler Version 10.00", `vc40.pdb`) dated
  1996-07-05 to 07-28.
- **The layer had its own tree:** the emulator layer, which FLICFILE belongs to, was built under
  `E:\Mk3p\emulator`. That project uses VC 2.x `WinDebug` folders, and there's a December
  1995 VC 2.x IDE log for `WinDebug/emulator.exe`. So the layer lived on a different drive, perhaps
  a different machine, from the game's.
- **No known compiler fits:** only two x86 C2 builds of the 9.00 generation are documented: VC 2.0 (1994-08-27) and
  VC 2.1/2.2/VCFDIV (1994-12-14). VC 2.0 matches every other function in the exe; the other is worse here. No
  other public C2 patch for VC 2.x exists.

None of this names the exact build. Nothing that survives records the compiler version for the game's objects.

## What would settle it

Any one of these:

- **`MK3.pdb`** from `D:\mk3p\vcpp-obj`: VC 2.x PDBs record each module's compiler version.
- **An original `FLICFILE.OBJ`**, or any object from the `emulator` tree.

Checked and of no help: the "Jul 5, 1996 prototype" leak (identical to the retail target), and the 1995 DOS version
(a separate Watcom-built program).
