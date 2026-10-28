#!/usr/bin/env python3
"""R11M Stage 5 demonstration judge (independent of the programs' own banners where the transcript allows).

usage: r11m-demo-judge.py EMU A.txt B.txt      EMU = vm | pc | qemu
Reads the transcripts of the two phases of the demonstration (tests/r11m/seq-demo-*-a.txt / seq-demo-b.txt) and prints
"ok ..." / "FAIL ..." lines; exit status 1 if any check failed.  QEMU transcripts are VGA text-buffer snapshots cut into
segments by "=== CMD " markers; Tiny386 and i386VM transcripts are terminal streams.
"""
import re
import sys

emu, pa, pb = sys.argv[1:4]
A = open(pa, errors="replace").read()
B = open(pb, errors="replace").read()
bad = 0


def ok(c, msg):
    global bad
    print(("ok   " if c else "FAIL ") + msg)
    if not c:
        bad += 1


def segments(text):
    """qemu: {command: [last snapshot text, ...]} in order of appearance."""
    out = []
    for seg in text.split("=== CMD ")[1:]:
        head, _, body = seg.partition("\n")
        snaps = body.split("=== SNAP ")
        last = snaps[-1].split("\n", 1)[1] if len(snaps) > 1 else ""
        out.append((head.strip(), last, body))
    return out


def maxnum(text, pat):
    v = [int(m.group(1)) for m in re.finditer(pat, text)]
    return max(v) if v else -1


# ---- phase A: file system part -------------------------------------------------------------------------------
ok(re.search(r"SUPERB~1\.C\s+205\s+SUPERBASIC_INTERPRETER\.C", A) is not None, "A: DIR shows the alias and the long name of the copied source")
ok(re.search(r"RELEAS~1\.TXT\s+4011\s+Release Notes\.txt", A) is not None, "A: DIR shows the alias and the long name of the quoted copy")
ok("205 bytes copied" in A and "4011 bytes copied" in A, "A: COPY reports the byte counts")
ok(re.search(r"/SRC> ?PWD\s*\n/SRC", A) is not None or "/SRC\n" in A, "A: PWD shows /SRC after CD SRC")
ok(A.count("line 60: the quick brown fox") >= 1, "A: TYPE of the long-named file shows its last line")

# ---- phase A: consoles and jobs ------------------------------------------------------------------------------
ok(re.search(r"COUNTER\.AOU\s+\(paged\)", A) is not None and re.search(r"TICKER\.AOU\s+\(paged\)", A) is not None, "A: PS shows both programs as paged jobs (same linked origin)")
ok(re.search(r"65539\s+3\s+\w+\s+\d\s+1\s+\d+\s+COUNTER", A) is not None and re.search(r"65540\s+4\s+\w+\s+\d\s+2\s+\d+\s+TICKER", A) is not None,
   "A: COUNTER runs on console 1 and TICKER on console 2")
if emu == "qemu":
    segs = segments(A)
    f2 = [s for s in segs if s[0] == "%key alt-f2"]
    f3 = [s for s in segs if s[0] == "%key alt-f3"]
    ok(len(f2) >= 2 and len(f3) >= 2, "A: the focus was switched to console 1 twice and console 2 twice")
    if len(f2) >= 2 and len(f3) >= 2:
        c1, c2 = maxnum(f2[0][1], r"COUNTER 1 (\d+)"), maxnum(f2[1][1], r"COUNTER 1 (\d+)")
        t1, t2 = maxnum(f3[0][1], r"tick (\d+) of"), maxnum(f3[1][1], r"tick (\d+) of")
        ok(0 < c1 < c2, "A: console 1 progress continued while another console had the focus (COUNTER %d -> %d)" % (c1, c2))
        ok(0 < t1 < t2, "A: console 2 progress continued while another console had the focus (tick %d -> %d)" % (t1, t2))
        ok(all("tick" not in s[1] and "/>" not in s[1] for s in f2), "A: console 1 shows only its own output (no TICKER text, no shell prompt)")
        ok(all("COUNTER" not in s[1] and "/>" not in s[1] for s in f3), "A: console 2 shows only its own output (no COUNTER text, no shell prompt)")
        back = [s for s in segs if s[0] == "%key alt-f1"]
        ok(bool(back) and back[-1][1].rstrip().endswith(">"), "A: focus returned to the shell console, prompt shown")
else:
    toks = re.split(r"(?m)^(?:/[^>\n]*)?> ?", A)          # one token per typed command (command text first)
    d1 = [t for t in toks if t.startswith("CONDUMP 1")]
    d2 = [t for t in toks if t.startswith("CONDUMP 2")]
    ok(len(d1) >= 2 and len(d2) >= 2, "A: consoles 1 and 2 were dumped twice each")
    if len(d1) >= 2 and len(d2) >= 2:
        c1, c2 = maxnum(d1[0], r"COUNTER 1 (\d+)"), maxnum(d1[-1], r"COUNTER 1 (\d+)")
        ok(0 < c1 < c2, "A: console 1 progress continued between the two dumps (COUNTER %d -> %d)" % (c1, c2))
        t1, t2 = maxnum(d2[0], r"tick (\d+) of"), maxnum(d2[-1], r"tick (\d+) of")
        ok(0 < t1 < t2, "A: console 2 progress continued between the two dumps (tick %d -> %d)" % (t1, t2))
        ok(all("tick" not in t and "TICKER" not in t for t in d1), "A: console 1 holds only its own output")
        ok(all("COUNTER" not in t for t in d2), "A: console 2 holds only its own output")
    if emu == "pc":
        ok(A.count("TICKER on console 2") >= 1, "A: TICKER header present on console 2")
ok("job 65539 killed" in A and "job 65540 killed" in A, "A: KILL ended both background jobs")
ks = A.find("job 65540 killed")
ok(ks >= 0 and "2 job(s) in the kernel table" in A[ks:], "A: after KILL only the shell and the idle job remain in the table")
ok(A.count("2 job(s) in the kernel table") >= 2, "A: the table is back to two jobs before and after (clean reclamation)")
ok("QDOS/386 R11B JOBT PASS" in A, "A: JOBT (job table, kill, reclamation baselines) passed after the demonstration")
ok("QDOS/386 R9 FILECAT PASS" in A, "A: an SDK utility (CATFIL) ran from the shell")
ok(re.search(r"RUN PHOON\.AOU[^\n]*\n(?:.*\n){3,}?.*exit status 0", A) is not None or A.count("exit status 0") >= 3, "A: Phoon (x87) ran and exited with status 0")

# ---- phase B: after reboot -----------------------------------------------------------------------------------
ok(re.search(r"SUPERB~1\.C\s+205\s+SUPERBASIC_INTERPRETER\.C", B) is not None, "B (after reboot): DIR SRC still shows the long-named source")
ok(re.search(r"RELEAS~1\.TXT\s+4011\s+Release Notes\.txt", B) is not None, "B (after reboot): DIR SRC still shows the quoted-name copy")
if emu == "qemu":
    ty = [s for s in segments(B) if s[0].startswith("TYPE")]
    ok(len(ty) == 3 and all("line 60: the quick brown fox" in s[1] for s in ty),
       "B (after reboot): the file is read by long name, by alias and by lower-case name inside the directory")
else:
    ok(B.count("line 60: the quick brown fox") >= 3, "B (after reboot): the file is read by long name, by alias and by lower-case name inside the directory")
ok(re.search(r"/SRC\n", B) is not None, "B (after reboot): CD SRC / PWD")
ok("2 job(s) in the kernel table" in B, "B (after reboot): PS shows a clean table")
print("DEMO %s %s" % (emu, "PASS" if bad == 0 else "FAIL (%d)" % bad))
sys.exit(1 if bad else 0)
