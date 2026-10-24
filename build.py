r"""Build mk3w.exe (Mortal Kombat 3 for Windows 95) from source - see BUILDING.md.

Usage (from this folder):  python3 -I build.py <retail mk3w.exe>   (Windows: python)
Output: OUT/mk3w.exe and OUT/mk3w.map."""
import hashlib, json, os, re, shutil, struct, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.abspath(__file__))
TMP = os.path.join(ROOT, "TMP")
OUT = os.path.join(ROOT, "OUT")
MATCH = os.path.join(ROOT, "MATCH")
VC = os.path.join(ROOT, "MSVC20")
RETAIL_SHA1 = "e9ed6a82752a1ff75766f74c04bb030fa8f2c022"
# the Windows tools run directly on Windows and through Wine elsewhere
WINE = None if sys.platform == "win32" else (os.environ.get("MK3_WINE") or shutil.which("wine") or "wine")
JWASM = os.path.join(ROOT, "TOOLS", "JWasm.exe")

# include path, in this order: the layer's headers, the game's, then the DirectX 2 SDK's and the
# July 1996 Win32 SDK's ahead of Visual C++ 2.0's own
# the layer (WIN95\) is compiled and included from its copy, whose headers carry their symbol-count padding
LAYER_COPY = os.path.join("TMP", "w", "emulator", "inc")
INCLUDE = [LAYER_COPY, os.path.join(LAYER_COPY, "FLIC"), "KODE", "ASM", os.path.join("DXSDK", "INC"),
           os.path.join("WIN32SDK", "INCLUDE"), os.path.join("MSVC20", "INCLUDE")]
DEFS = ["-DWIN32", "-D_WINDOWS", "-DNDEBUG"]
OX, OD = ["-Ox", "-G5"], ["-Od", "--cpp"]			# retail compiled the layer's -Od files as C++
GAME_OPTS = {"BKGDBELL": ["-G5", "-Oi"]}			# game modules with other switches than -Ox -G5
LAYER = [("PSXEMU", OX), ("POLYSCAL", OX), ("KEYBOARD", OX), ("JOYSTICK", OD), ("WINVIDEO", OD), ("KLUDGPIC", OD),
         ("WINDDSND", OD), ("WESSWIN", OX), ("WINDDRAW", OD), ("WINMAIN", OD), ("PSXCDWIN", OX), ("MEMFILL", OX),
         (os.path.join("FLIC", "FLICFILE"), OX)]
ASM_FILES = [os.path.join("WIN95", "BLIT.ASM")] + [os.path.join("WIN95", "FLIC", f + ".ASM")
                                                for f in ("FLICCOL", "FLICBRUN", "FLICLC", "FLICSS2")]
# retail starts WINVIDEO, KLUDGPIC and WINMAIN right after the object before them: 1-byte .text alignment
ALIGN1 = ["WINVIDEO", "KLUDGPIC", "WINMAIN"]
LIBS = [os.path.join("DXSDK", "LIB", "DSOUND.LIB"), os.path.join("MSVC20", "LIB", "MSACM32.LIB"),
        os.path.join("WIN32SDK", "LIB", "WINMM.LIB")] + \
       [os.path.join("MSVC20", "LIB", l) for l in ("KERNEL32.LIB", "USER32.LIB", "GDI32.LIB", "ADVAPI32.LIB")]
# the link and resource time stamps written into the exe: retail's
BUILD_TIME = (0x31dd1380, 0x31dd1376)


BIN_FILES = ["C1.EXE", "C1XX.EXE", "C2.EXE", "CL.EXE", "CVTRES.EXE", "DBI.DLL", "LINK.EXE", "MSVCRT20.DLL", "RC.EXE",
             "RCDLL.DLL"]
# the tools' message texts: read only to print a warning or an error, copied when present
BIN_MESSAGES = ["C1.ERR", "C23.ERR", "LINK.ERR"]
BIN = None


def bin_dir():
    """a copy of the Visual C++ 2.0 tools in a folder without spaces: CL starts its passes, and LINK
    CVTRES, with their own folder in the command line, unquoted. TMP/BIN, or its short (8.3) name on
    Windows, or one in the system's temporary folder. The copy also keeps LINK on Visual C++ 2.0's
    own MSVCRT20.DLL (see link())."""
    for d in (os.path.join(TMP, "BIN"), os.path.join(tempfile.gettempdir(), "mk3w-vc20-bin")):
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(d)
        for f in BIN_FILES + BIN_MESSAGES:
            if f in BIN_FILES or os.path.exists(ci(os.path.join(VC, "BIN", f))):
                shutil.copy(ci(os.path.join(VC, "BIN", f)), os.path.join(d, f))
        if " " not in d:
            return d
        if sys.platform == "win32":
            import ctypes
            buf = ctypes.create_unicode_buffer(1024)
            if ctypes.windll.kernel32.GetShortPathNameW(d, buf, 1024) and " " not in buf.value:
                return buf.value
    raise SystemExit("no folder without spaces for the Visual C++ tools: set TMPDIR (TEMP on Windows) to one")


def win(path):
    return path.replace("/", "\\")


def rel(path, cwd):
    return win(os.path.relpath(path, cwd))


def tool(exe, args, cwd, env=None):
    """run a Windows tool with relative paths (Visual C++ 2.0 passes paths to its passes unquoted)"""
    e = dict(os.environ, **(env or {}))
    e.update(TMP=".", TEMP=".")
    if WINE:
        e.setdefault("WINEDEBUG", "-all")
        e["WINEDLLOVERRIDES"] = "msvcrt20=n"			# Visual C++'s own MSVCRT20.DLL: see link()
        cmd = [WINE, exe] + args
    else:
        cmd = [exe] + args
    r = subprocess.run(cmd, cwd=cwd, env=e, capture_output=True, text=True, errors="replace")
    return r.returncode, r.stdout + r.stderr


def ci(path):
    """PATH as it exists on disk, found without regard to case"""
    if os.path.exists(path):
        return path
    head, tail = os.path.split(path)
    head = ci(head)
    if os.path.isdir(head):
        for n in os.listdir(head):
            if n.lower() == tail.lower():
                return os.path.join(head, n)
    return os.path.join(head, tail)


def read_text(path):
    raw = open(path, "rb").read().decode("latin1")
    return ("\r\n" if "\r\n" in raw else "\n"), raw.replace("\r\n", "\n")


def write_text(path, nl, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "wb").write(nl.join(text.split("\n")).encode("latin1"))


# ---------------------------------------------------------------- the retail executable
def pe_sections(d):
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    ns, osz = struct.unpack_from("<H", d, pe + 6)[0], struct.unpack_from("<H", d, pe + 20)[0]
    return [(d[pe + 24 + osz + 40 * i:pe + 32 + osz + 40 * i].rstrip(b"\0").decode("latin1"),)
            + struct.unpack_from("<IIII", d, pe + 24 + osz + 40 * i + 8) for i in range(ns)]


def file_offset(d, addr):
    rva = addr - 0x400000
    for name, vs, va, rs, rp in pe_sections(d):
        if va <= rva < va + max(vs, rs):
            return rp + rva - va
    raise KeyError(hex(addr))


ICON_SHA256 = "64e8a283252f5f0360f69eafb46b163e158dbe9916f8137659b943d1a1627285"


def extract_icon(retail, dst):
    """MK3W.ICO (the icon MK3W.RC includes) from retail's icon resources: the RT_GROUP_ICON directory with
    each entry's resource id replaced by the file offset of its RT_ICON image"""
    pe = struct.unpack_from("<I", retail, 0x3c)[0]
    base = file_offset(retail, 0x400000 + struct.unpack_from("<I", retail, pe + 24 + 96 + 2 * 8)[0])

    def entries(o):
        named, ids = struct.unpack_from("<HH", retail, base + o + 12)
        return [struct.unpack_from("<II", retail, base + o + 16 + 8 * k) for k in range(named + ids)]

    def data(o):						# the first language's data under a directory entry
        while o & 0x80000000:
            o = entries(o & 0x7fffffff)[0][1]
        rva, size = struct.unpack_from("<II", retail, base + o)
        at = file_offset(retail, 0x400000 + rva)
        return retail[at:at + size]

    types = dict(entries(0))
    images = {n: data(t) for n, t in entries(types[3] & 0x7fffffff)}		# RT_ICON
    group = data(entries(types[14] & 0x7fffffff)[0][1])			# RT_GROUP_ICON
    count = struct.unpack_from("<H", group, 4)[0]
    ico, body = bytearray(group[:6]), b""
    for i in range(count):
        w, h, colors, res, planes, bits, size, nid = struct.unpack_from("<BBBBHHIH", group, 6 + 14 * i)
        ico += struct.pack("<BBBBHHII", w, h, colors, res, planes, bits, size, 6 + 16 * count + len(body))
        body += images[nid]
    ico += body
    if hashlib.sha256(ico).hexdigest() != ICON_SHA256:
        raise SystemExit("MK3W.ICO: the icon from the retail executable is not the original")
    open(dst, "wb").write(bytes(ico))


# ---------------------------------------------------------------- skeleton objects
def coff_sections(b):
    """[(file offset of the contents, size, set of relocated byte offsets)] of a COFF object's sections"""
    nsec, optsz = struct.unpack_from("<H", b, 2)[0], struct.unpack_from("<H", b, 16)[0]
    out = []
    for s in range(nsec):
        h = 20 + optsz + 40 * s
        size, rawptr, relptr = struct.unpack_from("<III", b, h + 16)
        nrel, = struct.unpack_from("<H", b, h + 32)
        keep = set()
        for r in range(nrel):
            at, = struct.unpack_from("<I", b, relptr + 10 * r)
            keep.update(range(at, at + 4))
        out.append((rawptr, size, keep))
    return out


def fill_skeletons(retail, dst):
    """the original data objects: each skeleton (the object with its section contents zeroed, except the
    4-byte words at relocation sites) with the contents copied from retail"""
    skel = os.path.join(MATCH, "SKELETONS")
    manifest = json.load(open(os.path.join(skel, "skeletons.json")))
    os.makedirs(dst, exist_ok=True)
    for name, entry in manifest.items():
        b = bytearray(open(os.path.join(skel, name + ".SKL"), "rb").read())
        secs = coff_sections(b)
        for k, addr in entry["sections"].items():
            rawptr, size, keep = secs[int(k) - 1]
            src = file_offset(retail, int(addr, 16))
            for i in range(size):
                if i not in keep:
                    b[rawptr + i] = retail[src + i]
        if hashlib.sha256(b).hexdigest() != entry["sha256"]:
            raise SystemExit(f"{name}: the filled object is not the original")
        open(os.path.join(dst, name + ".obj"), "wb").write(bytes(b))


# ---------------------------------------------------------------- symbol-count padding
# Visual C++ 2.0's register choices depend on how many symbols the compiler has seen. The headers of the
# original build declared more than the ones that survive; MATCH\PADDING.json gives, per file, how many
# declarations stand in for them and where ("@top": after the last top-level #include; FUNC: before the
# definition of FUNC; "=NAME": before the definition of the variable NAME; "I:FUNC|REGEX": inside FUNC,
# before its first line matching REGEX; "A:REGEX" / "B:REGEX": after / before the first line matching REGEX;
# "H:HEADER|REGEX": in a copy of HEADER, before its first line matching REGEX, or at its top without one).
# Its "headers" are padded for every file that includes them.
NOTE = "/* symbol count, MATCH\\PADDING.json */"


def points(text, funcs):
    lines = text.split("\n")
    inc = [i for i, l in enumerate(lines) if re.match(r"\s*#\s*include", l)]
    res = {"@top": inc[-1] + 1 if inc else 0}
    depth = 0
    for i, l in enumerate(lines):
        if depth == 0:
            m = re.match(r"^[A-Za-z_][\w \t\*]*?\b(\w+)\s*\(", l)
            if m and m.group(1) in funcs and m.group(1) not in res and not l.rstrip().endswith(";"):
                res[m.group(1)] = i
        if re.match(r'\s*extern\s+"C"\s*\{', l):		# a C++ file's extern "C" { ... } wrapper
            continue
        depth = max(depth + l.count("{") - l.count("}"), 0)
    return res


def pad(src, pads, dst_dir, hdir, prefix):
    """write a padded copy of SRC (and the header copies its "H:" points ask for) into DST_DIR"""
    os.makedirs(dst_dir, exist_ok=True)
    hcopies = {}
    for p, n in pads.items():
        if p.startswith("H:") and n:
            hspec, _, regex = p[2:].partition("|")
            hname = os.path.basename(ci(os.path.join(hdir, hspec)))
            if hname not in hcopies:
                hcopies[hname] = read_text(os.path.join(hdir, hname))
            hnl, htext = hcopies[hname]
            hl = htext.split("\n")
            i = next(k for k, l in enumerate(hl) if re.search(regex, l)) if regex else 0
            hname_tag = os.path.splitext(hname)[0].lower() + re.sub(r"\W", "", regex)[:12]
            name = f"{prefix}_h_{hname_tag}" if prefix == "modpad" else f"{prefix}_h"
            hl[i:i] = [f"extern int {name}{'_' if prefix == 'modpad' else ''}{k};\t{NOTE}" for k in range(n)]
            hcopies[hname] = (hnl, "\n".join(hl))
    for hname, (hnl, htext) in hcopies.items():
        write_text(os.path.join(dst_dir, hname), hnl, htext)
    pads = {p: n for p, n in pads.items() if not p.startswith("H:")}
    nl, text = read_text(src)
    pts = points(text, set(pads) | {p[2:].split("|")[0] for p in pads if p.startswith("I:")})
    lines = text.split("\n")
    for p in pads:
        if p.startswith("I:"):
            fn, regex = p[2:].split("|", 1)
            pts[p] = next(i for i in range(pts[fn], len(lines)) if re.search(regex, lines[i]))
        elif p.startswith("A:"):
            pts[p] = next(i for i, l in enumerate(lines) if re.search(p[2:], l)) + 1
        elif p.startswith("B:"):
            pts[p] = next(i for i, l in enumerate(lines) if re.search(p[2:], l))
        elif p.startswith("="):
            pat = re.compile(r"^(static\s+)?[A-Za-z_][\w \t\*]*\b%s\b\s*(\[|=|;|,)" % re.escape(p[1:]))
            pts[p] = next(i for i, l in enumerate(lines) if pat.match(l))
    for p, n in sorted(pads.items(), key=lambda kv: -pts[kv[0]]):
        tag = "" if p.startswith(("A:", "B:")) else re.sub(r"\W", "_", p.strip("@=")) + "_"
        lines[pts[p]:pts[p]] = [f"extern int {prefix}_{tag}{k};\t{NOTE}" for k in range(n)]
    out = os.path.join(dst_dir, os.path.basename(src))
    write_text(out, nl, "\n".join(lines))
    return out


# ---------------------------------------------------------------- compile, assemble, resources
def compile_jobs():
    """[(source, options, object folder)] of every C file the build compiles"""
    padding = json.load(open(os.path.join(MATCH, "PADDING.json")))
    # the layer's copy (LAYER_COPY), with its headers padded; BUILDSND.C includes ../emulator/inc/stdstuff.h
    # and ../code/mk3snd.h (the layer's headers, WIN95\, were emulator\inc and the game, KODE\, was code)
    layer = os.path.join(ROOT, LAYER_COPY)
    shutil.rmtree(os.path.join(TMP, "w"), ignore_errors=True)
    shutil.copytree(os.path.join(ROOT, "WIN95"), layer)
    shutil.copytree(os.path.join(ROOT, "KODE"), os.path.join(TMP, "w", "code"))
    for name, pads in padding["headers"].items():
        pad(ci(os.path.join(layer, name)), pads, layer, layer, "hdrpad_" + os.path.splitext(name)[0].lower())
    jobs = []
    code = os.path.join(ROOT, "KODE")
    for f in sorted(os.listdir(code)):
        base, ext = os.path.splitext(f)
        if ext.lower() != ".c":
            continue
        src = os.path.join(code, f)
        p = padding["game"].get(base.upper(), {})
        if any(p.values()):
            src = pad(src, p, os.path.join(TMP, "pad", "game", base.upper()), code, "modpad")
        jobs.append((src, GAME_OPTS.get(base.upper(), OX), "game"))
    for f, opts in LAYER:
        name = os.path.basename(f).upper()
        src = os.path.join(layer, f + ".C")
        p = padding["layer"].get(name, {})
        if any(p.values()):
            src = pad(src, p, os.path.join(TMP, "pad", "layer", name), layer, "layerpad")
        jobs.append((src, opts + DEFS, "layer"))
    jobs.append((os.path.join(ROOT, "ASM", "OVERLAY.C"), ["-Od"] + DEFS, "other"))
    snd = os.path.join(TMP, "w", "sound")
    os.makedirs(snd)
    for f in os.listdir(os.path.join(ROOT, "SOUND")):
        shutil.copy(os.path.join(ROOT, "SOUND", f), snd)
    for name, pads in padding["sound"].items():
        p = pad(ci(os.path.join(snd, name + ".C")), pads, snd, snd, "soundpad")
        jobs.append((p, OX + DEFS, "other"))
    return jobs


def compile_all():
    jobs = compile_jobs()
    for src, opts, sub in jobs:
        cwd = os.path.join(TMP, "obj", sub)
        os.makedirs(cwd, exist_ok=True)
        cpp = "--cpp" in opts
        opts = [o for o in opts if o != "--cpp"]
        obj = os.path.splitext(os.path.basename(src))[0] + ".obj"
        inc = ";".join(rel(os.path.join(ROOT, d), cwd) for d in INCLUDE)
        rc, out = tool(os.path.join(BIN, "CL.EXE"), ["/nologo", "/c"] + opts +
                       ["/Fo" + obj, ("/Tp" if cpp else "") + rel(src, cwd)], cwd, {"INCLUDE": inc})
        if rc or not os.path.exists(os.path.join(cwd, obj)):
            raise SystemExit(f"compile failed: {os.path.relpath(src, ROOT)}\n{out[-3000:]}")
    for n in ALIGN1:
        set_text_align1(os.path.join(TMP, "obj", "layer", n + ".obj"))
    print(f"compiled {len(jobs)} C files")


def set_text_align1(path):
    d = bytearray(open(path, "rb").read())
    nsec, optsz = struct.unpack_from("<H", d, 2)[0], struct.unpack_from("<H", d, 16)[0]
    for i in range(nsec):
        o = 20 + optsz + 40 * i
        if d[o:o + 8].rstrip(b"\0") == b".text":
            ch = struct.unpack_from("<I", d, o + 36)[0]
            struct.pack_into("<I", d, o + 36, (ch & ~0x00f00000) | 0x00100000)
    open(path, "wb").write(bytes(d))


def masm(src, dst):
    """the sources use Phar Lap 386|ASM syntax: a JWasm copy (#label -> L_proc_label, untyped EXTERN ->
    EXTERN name:DWORD, forward jumps near, EXTERNs inside the code segment addressed through cs:)"""
    t = open(src, encoding="latin-1").read()
    out, proc = [], "top"
    for line in t.split("\n"):
        m = re.match(r"\s*(\w+)\s+proc\b", line, re.I)
        if m:
            proc = m.group(1)
        line = re.sub(r"#([A-Za-z_]\w*)", lambda x: f"L_{proc}_{x.group(1)}", line)
        line = re.sub(r"(?i)^(\s*)repne\s+(movs[bwd]|stos[bwd])\b", r"\1db 0f2h\n\1\2", line)
        # [r1+r2]: the original takes r1 as the base register, JWasm takes r2
        line = re.sub(r"(?i)\[(e[a-ds][xip]|e[sd]i)\+(e[a-ds][xip]|e[sd]i)([-+\]])", r"[\2+\1\3", line)
        out.extend(line.split("\n"))
    labels = {}
    for n, line in enumerate(out):
        m = re.match(r"\s*(\w+):", line)
        if m:
            labels[m.group(1).lower()] = n
    for n, line in enumerate(out):
        m = re.match(r"(?i)^(\s*)(j[a-z]+)\s+(\w+)\s*(;.*)?$", line)
        if m and m.group(3).lower() not in ("short", "near") and labels.get(m.group(3).lower(), -1) > n:
            out[n] = f"{m.group(1)}{m.group(2)} near ptr {m.group(3)}"
    incode = False
    for n, line in enumerate(out):
        if re.match(r"\s*BEGIN_CODE\b", line):
            incode = True
        m = re.match(r"(?i)\s*EXTERN\s+(\w+)", line)
        if m and incode:
            for k in range(n + 1, len(out)):
                out[k] = re.sub(r"(?i)\bptr\s+(\(?\s*%s\b)" % re.escape(m.group(1)), r"ptr cs:\1", out[k])
    t = "\n".join(out)
    t = re.sub(r"(?mi)^(include\s+win95\.i.*)$", r"\1\n\toption noscoped", t)
    t = re.sub(r"(?mi)^(\s*EXTERN\s+)(\w+)\s*(;.*)?$", r"\1\2:DWORD \3", t)
    open(dst, "w", encoding="latin-1").write(t)


def assemble():
    work = os.path.join(TMP, "obj", "asm")
    os.makedirs(work, exist_ok=True)
    shutil.copy(os.path.join(ROOT, "WIN95", "WIN95.I"), os.path.join(work, "win95.i"))
    for f in ASM_FILES:
        name = os.path.splitext(os.path.basename(f))[0]
        masm(os.path.join(ROOT, f), os.path.join(work, name + ".ASM"))
        rc, out = tool(JWASM, ["-nologo", "-coff", "-Fo" + name + ".obj", name + ".ASM"], work)
        if rc or not os.path.exists(os.path.join(work, name + ".obj")):
            raise SystemExit(f"assembly failed: {f}\n{out}")
    print(f"assembled {len(ASM_FILES)} files")


def resources(retail):
    work = os.path.join(TMP, "res")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    shutil.copy(os.path.join(ROOT, "WIN95", "MK3W.RC"), work)
    extract_icon(retail, os.path.join(work, "MK3W.ICO"))
    inc = ";".join(rel(os.path.join(ROOT, d), work) for d in INCLUDE)
    rc, out = tool(os.path.join(BIN, "RC.EXE"), ["/r", "/foMK3W.RES", "MK3W.RC"], work, {"INCLUDE": inc})
    if rc or not os.path.exists(os.path.join(work, "MK3W.RES")):
        raise SystemExit(f"RC failed\n{out}")


# ---------------------------------------------------------------- link
def link():
    """LINK 2.50 in retail's object order (MATCH\\MK3W.LNK), the .RES last: LINK runs CVTRES -r itself, which
    gives retail's read-only .rsrc. LINK must load Visual C++ 2.0's own MSVCRT20.DLL (from its folder; under
    Wine, WINEDLLOVERRIDES): it sorts the import thunks with that DLL's qsort, and the MSVCRT20.DLL of
    today's Windows (and Wine's) forwards to msvcrt.dll, whose qsort orders equal keys differently."""
    order = [l.strip() for l in open(os.path.join(MATCH, "MK3W.LNK")) if l.strip() and not l.startswith(";")]
    work = os.path.join(TMP, "L")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    # LINK 2.50 fails with an internal error in pass 1 when the input paths are long: short names
    names = []
    for i, o in enumerate(order):
        src = os.path.join(TMP, "res", o) if o.upper().endswith(".RES") else os.path.join(TMP, "obj", *o.split("/"))
        n = "O%03d%s" % (i, os.path.splitext(o)[1].upper())
        shutil.copy(src, os.path.join(work, n))
        names.append(n)
    for l in LIBS:
        shutil.copy(os.path.join(ROOT, l), work)
    open(os.path.join(work, "L.RSP"), "w").write("\n".join(names + [os.path.basename(l) for l in LIBS]) + "\n")
    rc, out = tool(os.path.join(BIN, "LINK.EXE"), ["/NOLOGO", "/SUBSYSTEM:WINDOWS", "/MAP:MK3W.MAP", "/OUT:MK3W.EXE",
                                                   "@L.RSP"], work, {"LIB": rel(os.path.join(VC, "LIB"), work)})
    exe = ci(os.path.join(work, "MK3W.EXE"))
    if rc or not os.path.exists(exe) or "INTERNAL ERROR" in out:
        raise SystemExit(f"link failed\n{out[-4000:]}")
    os.makedirs(OUT, exist_ok=True)
    shutil.copy(exe, os.path.join(OUT, "mk3w.exe"))
    shutil.copy(ci(os.path.join(work, "MK3W.MAP")), os.path.join(OUT, "mk3w.map"))
    stamp_build_time(os.path.join(OUT, "mk3w.exe"))


def stamp_build_time(exe):
    """LINK writes the current time into the PE header and CVTRES into every resource directory"""
    link_t, res_t = BUILD_TIME
    d = bytearray(open(exe, "rb").read())
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    struct.pack_into("<I", d, pe + 8, link_t)
    rsrc_rva = struct.unpack_from("<I", d, pe + 24 + 96 + 2 * 8)[0]
    for name, vs, va, rs, rp in pe_sections(d):
        if va <= rsrc_rva < va + max(vs, rs):
            base = rp + rsrc_rva - va

            def walk(off):
                struct.pack_into("<I", d, base + off + 4, res_t)
                named, ids = struct.unpack_from("<HH", d, base + off + 12)
                for k in range(named + ids):
                    entry = struct.unpack_from("<I", d, base + off + 16 + 8 * k + 4)[0]
                    if entry & 0x80000000:
                        walk(entry & 0x7fffffff)
            walk(0)
    open(exe, "wb").write(bytes(d))


# ---------------------------------------------------------------- compare
def compare(ours, retail):
    print(f"OUT/mk3w.exe: {len(ours)} bytes (retail {len(retail)}), sha1 {hashlib.sha1(ours).hexdigest()}")
    if ours == retail:
        print("identical to retail")
        return
    a = {s[0]: s for s in pe_sections(ours)}
    total = 0
    for name, vs, va, rs, rp in pe_sections(retail):
        if name not in a:
            print(f"  {name}: missing")
            continue
        _, vs2, va2, rs2, rp2 = a[name]
        x, y = retail[rp:rp + rs], ours[rp2:rp2 + rs2]
        diff = [i for i in range(min(len(x), len(y))) if x[i] != y[i]] + list(range(min(len(x), len(y)), max(len(x), len(y))))
        total += len(diff)
        if diff:
            print(f"  {name}: {len(diff)} bytes differ, {0x400000 + va + diff[0]:#x}-{0x400000 + va + diff[-1]:#x}")
    head = sum(1 for i in range(0x400) if ours[i] != retail[i])
    if head:
        print(f"  headers: {head} bytes differ")
    print(f"{total + head} bytes differ from retail")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    retail = open(sys.argv[1], "rb").read()
    if hashlib.sha1(retail).hexdigest() != RETAIL_SHA1:
        raise SystemExit(f"{sys.argv[1]} is not the retail mk3w.exe (sha1 {RETAIL_SHA1})")
    missing = [os.path.relpath(p, ROOT) for p in [JWASM] + [os.path.join(ROOT, l) for l in LIBS] +
               [os.path.join(VC, "BIN", f) for f in BIN_FILES] if not os.path.exists(ci(p))]
    if missing:
        raise SystemExit("missing (see BUILDING.md): " + ", ".join(missing))
    if WINE and not shutil.which(WINE):
        raise SystemExit(f"Wine not found ({WINE}); install it or set MK3_WINE (see BUILDING.md)")
    shutil.rmtree(TMP, ignore_errors=True)
    os.makedirs(TMP)
    global BIN
    BIN = bin_dir()
    fill_skeletons(retail, os.path.join(TMP, "obj", "data"))
    compile_all()
    assemble()
    resources(retail)
    link()
    compare(open(os.path.join(OUT, "mk3w.exe"), "rb").read(), retail)


if __name__ == "__main__":
    main()
