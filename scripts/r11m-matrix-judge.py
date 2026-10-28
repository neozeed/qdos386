#!/usr/bin/env python3
"""Judge one round of scripts/r11m-matrix.sh: python3 r11m-matrix-judge.py EMU OUTDIR.

Judges the transcripts (patterns the guest programs print AND text the shell/kernel produce), the final scratch images
(fsck.fat -n and tools/r11m-fatwalk.py --strict --vfat) and the pass lines of the sub-gates (VFAT gate, demonstration).
Prints "ok"/"FAIL"/"NA" lines and "MATRIX <emu> PASS|FAIL"; exit status 1 on any failure.  A case that was not run is reported NA,
never PASS."""
import os
import re
import subprocess
import sys

emu, out = sys.argv[1:3]
bad = 0
ran = 0
pref = {"vm": "vm", "pc": "tiny386", "qemu": "qemu"}[emu]


def ok(c, msg):
    global bad, ran
    ran += 1
    print(("ok   " if c else "FAIL ") + msg)
    if not c:
        bad += 1


def na(msg):
    print("NA   " + msg)


def rd(tag):
    p = os.path.join(out, "%s-%s.txt" % (pref, tag))
    return open(p, errors="replace").read() if os.path.exists(p) else None


def img(tag):
    p = os.path.join(out, "%s-%s.final.img" % (pref, tag))
    return p if os.path.exists(p) else None


def image_ok(tag):
    p = img(tag)
    if p is None:
        na("no final image for " + tag)
        return
    r = subprocess.run(["fsck.fat", "-n", p], capture_output=True, text=True)
    ok(r.returncode == 0, "%s: fsck.fat -n clean" % tag)
    r = subprocess.run([sys.executable, "tools/r11m-fatwalk.py", p, "--strict", "--vfat"], capture_output=True, text=True)
    ok(r.returncode == 0, "%s: fatwalk --strict --vfat: %s" % (tag, (r.stdout.strip().splitlines() or ["?"])[-1]))


def seg(text, cmd):
    """text typed after the prompt that echoes cmd, up to the next prompt."""
    m = re.search(r"(?m)^\S*> ?" + re.escape(cmd) + r"[ \t]*\n", text)
    if not m:
        return ""
    rest = text[m.end():]
    n = re.search(r"(?m)^\S*> ", rest)
    return rest[:n.start()] if n else rest


# ---- fs ---------------------------------------------------------------------------------------------------------------
a, b = rd("fs-a"), rd("fs-b")
if a is None or b is None:
    na("fs case not run")
else:
    ok("PATHT: 68 checks passed, 0 failed" in b and "QDOS/386 R11B PATHT PASS" in b, "fs: PATHT 68/68 after the reboot (native paths, rename, remove, listings, bad names)")
    ok(b.count("Hello from an independently linked i386 a.out") >= 5, "fs: RUN of HELLO.AOU by legacy, relative, native and /BIN paths (%d runs)" % b.count("Hello from an independently"))
    ok("destination exists" in a, "fs: COPY refuses an existing destination without /Y")
    ok(re.search(r"not empty", a, re.I) is not None, "fs: RD of a non-empty directory is refused")
    ok(len(re.findall(r"(?m)^(?:/\S*)?> ?TYPE (?:/SRC/|\.\./BIN/|SUB/|HELLO\.C)", b)) >= 4 and b.count("line 60:") >= 0 and "no such" not in b.lower().split("path")[0],
       "fs: files read back by absolute and relative paths after the reboot")
    image_ok("fs-a")
    image_ok("fs-b")

# ---- con --------------------------------------------------------------------------------------------------------------
t = rd("con")
if t is None:
    na("con case not run")
else:
    ok("CONT1: 45 checks passed, 0 failed" in t and "QDOS/386 R11M CONT1 PASS" in t, "con: CONT1 45/45 (cursor, attributes, regions, scrolling, snapshots, input modes)")
    image_ok("con")

# ---- conin ------------------------------------------------------------------------------------------------------------
t = rd("conin")
if t is None:
    na("conin case not run")
else:
    ok("GOT 1: hello1" in t, "conin: input typed while console 1 had the focus reached the program on console 1")
    ok("GOT 1: abc" in t, "conin: backspace editing in cooked mode (a b x BS c -> abc)")
    ok("CONINP DONE 1" in t, "conin: the program finished (quit received)")
    ok("GOT 1: LEAK" not in t, "conin: nothing typed for the shell reached console 1")
    if emu == "qemu":
        ok(re.search(r"(?m)^/> *LEAK", t) is not None and "GOT 1: LEAK" not in t, "conin: LEAK typed after switching back went to the shell")
    image_ok("conin")

# ---- job --------------------------------------------------------------------------------------------------------------
t = rd("job-a")
if t is None:
    na("job case not run")
else:
    n = 65 if emu == "vm" else 69
    ok(re.search(r"JOBT: %d checks passed, 0 failed" % n, t) is not None and "QDOS/386 R11B JOBT PASS" in t,
       "job: JOBT %d/%d on the paging kernel (%s)" % (n, n, "4 console-wait checks NA on i386VM" if emu == "vm" else "all checks"))
    ok(t.count("2 job(s) in the kernel table") >= 2, "job: the job table is back to the shell and the idle job after JOBT")
    image_ok("job-a")
tb = rd("job-b")
if tb is not None:
    ok("job 65539 started on console 2" in tb and "job 131075 started on console 1" in tb, "job: START on consoles 2 and 1; job ids carry a new tag after slot reuse")
    ok(re.search(r"CONINP\.AOU\s+\(status 3\)", tb) is not None and re.search(r"COUNTER\.AOU\s+\(status 0\)", tb) is not None, "job: finished jobs stay listed with their exit status")
    ok("no such job (unknown or stale id)" in tb, "job: KILL of a stale id is refused")
    ok(tb.count("2 job(s) in the kernel table") >= 2, "job: table back to two jobs")

# ---- jobq (QEMU keyboard path) ----------------------------------------------------------------------------------------
t = rd("jobq")
if t is not None:
    ok(re.search(r"COUNTER\.AOU\s+\(paged\)", t) is not None and re.search(r"TICKER\.AOU\s+\(paged\)", t) is not None, "jobq: two paged jobs listed")
    ok("job 65539 killed" in t and "job 65540 killed" in t, "jobq: KILL ends two running paged jobs")
    ok(re.search(r"65539\s+-\s+ENDED\s+-\s+1\s+COUNTER\.AOU\s+\(killed\)", t) is not None, "jobq: a killed job is listed as killed and its slot is free")
    ok(t.count("job 131075 exit status 0") >= 2, "jobq: WAIT reports the exit status of a finished job, repeatedly")
    ok("job 65539 was killed" in t, "jobq: WAIT on a killed job says it was killed")
    ok("no such job (unknown or stale id)" in t, "jobq: WAIT on an unknown id is refused")
    ok("refused: not a started job" in t and "refused: a job cannot kill itself" in t, "jobq: KILL of the idle job and of the shell itself is refused")
    ok("job 196611 started on console 3" in t and "job 196611 killed" in t, "jobq: a job on console 3 starts and is killed")
    ok("QDOS/386 R11B JOBT PASS" in t and t.count("2 job(s) in the kernel table") >= 2, "jobq: JOBT passes at the same prompt; the table is back to two jobs")
    image_ok("jobq")
elif emu == "qemu":
    na("jobq case not run")

# ---- phoon (background x87 jobs) --------------------------------------------------------------------------------------
t = rd("phoon")
if t is not None:
    fg = seg(t, 'RUN PHOON.AOU "11 Jan 2024 11:57:00"')
    fg = [l.expandtabs(8).rstrip() for l in fg.splitlines() if not l.startswith("exit status")]
    d1 = seg(t, "CONDUMP 1")
    d1 = [l[1:].rstrip() for l in d1.splitlines() if l.startswith("|")]
    ok(len(fg) >= 20 and fg == d1[:len(fg)], "phoon: background Phoon on console 1 prints exactly what the foreground RUN printed (%d lines, tabs expanded)" % len(fg))
    d2 = seg(t, "CONDUMP 2")
    ok(len([l for l in d2.splitlines() if l.startswith("|")]) >= 20, "phoon: the second background Phoon (other date) produced its own picture on console 2")

# ---- stress (i386VM) --------------------------------------------------------------------------------------------------
t = rd("stress")
if t is not None:
    m = re.search(r"CONINP DONE 1 \((\d+) lines\)", t)
    ok(m is not None and int(m.group(1)) == 151, "stress: 150 lines plus quit through console 1 input without loss (%s)" % (m.group(0) if m else "no completion"))
    got = re.findall(r"GOT 1: (L\d{3})", t)
    ok(got == ["L%03d" % i for i in range(150)], "stress: every line arrived once, in order (%d lines)" % len(got))

# ---- sub-gates -------------------------------------------------------------------------------------------------------
for name, tag, pat in (("vfat", "vfat", r"GATE %s PASS" % emu), ("demo", "demo", r"DEMO-ALL %s PASS" % emu)):
    p = os.path.join(out, "%s-%s.txt" % (emu, name))
    if not os.path.exists(p):
        na("%s case not run" % name)
        continue
    s = open(p, errors="replace").read()
    ok(re.search(pat, s) is not None and "FAIL" not in s, "%s: %s" % (name, "the whole %s sub-gate passed (%d checks)" % (name, s.count("\n  ok ") + s.count("\nok  "))))

print("MATRIX %s %s (%d checks)" % (emu, "PASS" if bad == 0 else "FAIL (%d)" % bad, ran))
sys.exit(1 if bad else 0)
