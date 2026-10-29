#!/usr/bin/env python3
# R12 (derived from scripts/r11m-qemu-capture.py, which stays frozen) VGA text capture under QEMU (SCRIPTED keys through the monitor, not a person).  Derived from the R11L script:
# the shell prompt is "/> " or "/DIR> " (it carries the current directory), and a sequence line can be a control
# line (a "%" prefix) instead of typed text:
#   %key NAME[,NAME..]   send monitor keys (e.g. alt-f2) without waiting for anything
#   %sleep SECONDS       wait while the guest runs (samples keep being taken)
#   %expect TEXT [S]     wait up to S seconds (default 60) until TEXT is on the screen; the segment FAILS (marker
#                        "=== EXPECT-TIMEOUT TEXT") if it never appears
#   %prompt              wait for a shell prompt on the screen (after a key that returns to the shell)
# R12 additions:
#   %type TEXT           type TEXT without Enter (the line editor tests); %enter presses Enter and does not wait
#   %screen LABEL        record the whole current screen as "=== SCREEN LABEL" + 25 rows "NN|text" (exact positions, blank rows kept)
#   %wait TEXT [S]       like %expect but a missing TEXT is only recorded (=== WAIT-TIMEOUT) and the run goes on
#   %gone TEXT [S]       wait until TEXT is no longer on the screen
# R12C addition:
#   %cursor LABEL        record the VGA hardware cursor as "=== CURSOR LABEL row col start" (CRTC registers 0x0E/0x0F,
#                        0x0A: start bit 5 set = cursor hidden), read with the guest stopped; the CRTC index is restored
#   %kill                SIGKILL the emulator at once (no monitor quit, no guest shutdown) and end the run: the volume
#                        holds exactly what QEMU had written to the image file at that moment
# R12_HDB_PREFIX (environment) is put in front of the hdb file name, e.g. "blkdebug:CONF:" for error injection.
# The original R11L notes follow.
# Generalises scripts/r11h-qemu-capture.py: the commands are arguments, uppercase letters are
# sent as shift-<key>, and the 80x25 text buffer at 0xB8000 is sampled from boot to the end,
# so lines that scroll away are kept.  Each command waits until "exit status" appears after it
# (or its timeout), so the run length follows the guest, not a fixed delay.
#   usage: python3 scripts/r11k-qemu-capture.py OUTDIR WORKDIR REPODIR ("cmd1" ["cmd2" ...] | --seq FILE)
#   --seq reads one command per line, typed exactly as written (the "!"/"@" prefixes are removed).
#   WORKDIR holds hda.img (VGA kernel disk) and hdb.img (FAT16 volume, a copy).
import re, socket, subprocess, sys, time

O, W, R = sys.argv[1], sys.argv[2], sys.argv[3]
CMDS = sys.argv[4:]
if len(CMDS) == 2 and CMDS[0] == "--seq":
    CMDS = [l.rstrip("\n") for l in open(CMDS[1]) if l.strip()]
PORT = int(__import__("os").environ.get("QEMU_MON_PORT", "4472"))
p = subprocess.Popen(["qemu-system-i386", "-machine", "pc", "-cpu", "pentium", "-m", __import__("os").environ.get("QEMU_MEM", "16"),
    "-bios", "/usr/share/seabios/bios.bin", "-vga", "std", "-display", "none",
    "-monitor", "tcp:127.0.0.1:%d,server,nowait" % PORT,
    "-drive", "file=%s/hda.img,format=raw,if=ide,index=0,media=disk" % W,
    "-drive", "file=%s%s/hdb.img,format=raw,if=ide,index=1,media=disk%s" % (__import__("os").environ.get("R12_HDB_PREFIX", ""), W, __import__("os").environ.get("R12_HDB_OPTS", "")),
    "-boot", "order=c", "-no-reboot"] + __import__("os").environ.get("QEMU_EXTRA", "").split(), cwd=R,
    stdout=open(O + "/stderr.txt", "w"), stderr=subprocess.STDOUT)
time.sleep(2)
s = socket.create_connection(("127.0.0.1", PORT))
s.settimeout(5)

def cmd(c, wait=0.0):
    s.sendall((c + "\n").encode())
    time.sleep(wait)
    buf = b""
    try:
        while True:
            d = s.recv(65536)
            if not d:
                break
            buf += d
            if buf.endswith(b"(qemu) "):
                break
    except Exception:
        pass
    return buf.decode(errors="replace")

KEY = {' ': 'spc', '.': 'dot', '-': 'minus', '\n': 'ret', '=': 'equal', '_': 'shift-minus', '/': 'slash', ',': 'comma', ';': 'semicolon', ':': 'shift-semicolon', '\\': 'backslash', '<': 'shift-comma', '>': 'shift-dot', '"': 'shift-apostrophe', "'": 'apostrophe', '(': 'shift-9', ')': 'shift-0', '*': 'shift-8', '+': 'shift-equal', '#': 'shift-3', '%': 'shift-5', '$': 'shift-4', '!': 'shift-1', '@': 'shift-2',
       '{': 'shift-bracket_left', '}': 'shift-bracket_right', '[': 'bracket_left', ']': 'bracket_right', '|': 'shift-backslash',
       '&': 'shift-7', '^': 'shift-6', '~': 'shift-grave_accent', '?': 'shift-slash', '`': 'grave_accent', '\t': 'tab'}

def key(ch):
    if ch.isupper():
        return "shift-" + ch.lower()
    return KEY.get(ch, ch)

def screen():
    out = cmd("xp /4000xb 0xb8000")
    vals = []
    for line in out.splitlines():
        m = re.match(r"^\s*0*b8[0-9a-f]{3}:\s*(.*)$", line)
        if m:
            vals += [int(x, 16) for x in re.findall(r"0x([0-9a-f]{2})", m.group(1))]
    chars = vals[0::2]
    return [bytes(chars[r * 80:(r + 1) * 80]).decode("cp437", errors="replace").rstrip()
            for r in range(25)]

seen, prev, samples = [], None, 0

def sample():
    global prev, samples
    rows = screen()
    samples += 1
    if rows != prev:
        seen.append("=== SNAP %d" % samples)
        seen.extend(r for r in rows if r)
        prev = rows
    return rows

def run_until(pred, seconds):
    t_end = time.time() + seconds
    while time.time() < t_end:
        rows = sample()
        if pred(rows):
            return True
        time.sleep(0.02)
    return False

PROMPT_RE = re.compile(r"^(/[^>]*)?>$")   # the prompt shows the cwd; VFAT directory names may contain spaces and punctuation

def prompt_last(rows):
    ne = [r for r in rows if r.strip()]
    return bool(ne) and PROMPT_RE.match(ne[-1].strip()) is not None

# R12B: QEMU_BOOT_KEY (e.g. "esc") is sent once QEMU_BOOT_KEY_AT (default "press ESC") is on the screen (the startup
# bypass); QEMU_BOOT_WAIT replaces "Type HELP" as the text that means the boot is over (a startup script may show
# another console first).  Unset, the behaviour is the R12 one.
_E = __import__("os").environ
if _E.get("QEMU_BOOT_KEY"):
    run_until(lambda rows: any(_E.get("QEMU_BOOT_KEY_AT", "press ESC") in r for r in rows), 90)
    seen.append("=== BOOT-KEY " + _E["QEMU_BOOT_KEY"])
    cmd("sendkey " + _E["QEMU_BOOT_KEY"])
run_until(lambda rows: any(_E.get("QEMU_BOOT_WAIT", "Type HELP") in r for r in rows), 90)
for c in CMDS:
    open(O + "/lines.txt", "w").write("\n".join(seen) + "\n")      # partial transcript survives a killed run
    seen.append("=== CMD " + c)          # marker: lets the judge cut the capture into one segment per typed command
    if c.startswith("%"):
        parts = c[1:].split(" ", 1)
        op, rest = parts[0], (parts[1] if len(parts) > 1 else "")
        if op == "key":
            for k in rest.split(","):
                cmd("sendkey " + k.strip(), 0.15)
                sample()
            run_until(lambda rows: False, 0.5)
        elif op == "sleep":
            run_until(lambda rows: False, float(rest))
        elif op == "expect":
            txt, _, secs = rest.rpartition(" ")
            if not secs.isdigit():
                txt, secs = rest, "60"
            if not run_until(lambda rows: any(txt in r for r in rows), float(secs)):
                seen.append("=== EXPECT-TIMEOUT " + txt)
        elif op == "type":
            for ch in rest:
                cmd("sendkey " + key(ch), 0.10)
            sample()
        elif op == "enter":
            cmd("sendkey ret", 0.15)
            sample()
        elif op == "screen":
            rows = screen()
            seen.append("=== SCREEN " + rest)
            for i, r in enumerate(rows):
                seen.append("%02d|%s" % (i, r))
            seen.append("=== SCREEN-END")
        elif op == "wait":
            txt, _, secs = rest.rpartition(" ")
            if not secs.isdigit():
                txt, secs = rest, "30"
            if not run_until(lambda rows: any(txt in r for r in rows), float(secs)):
                seen.append("=== WAIT-TIMEOUT " + txt)
        elif op == "gone":
            txt, _, secs = rest.rpartition(" ")
            if not secs.isdigit():
                txt, secs = rest, "30"
            if not run_until(lambda rows: not any(txt in r for r in rows), float(secs)):
                seen.append("=== GONE-TIMEOUT " + txt)
        elif op == "prompt":
            if not run_until(prompt_last, 120):
                seen.append("=== PROMPT-TIMEOUT")
        elif op == "kill":
            seen.append("=== KILL emulator (SIGKILL)")
            open(O + "/lines.txt", "w").write("\n".join(seen) + "\n")
            p.kill()
            p.wait()
            open(O + "/samples.txt", "w").write(str(samples) + "\n")
            print("killed", len(seen), samples)
            sys.exit(0)
        elif op == "cursor":
            def inb(port):
                m = re.search(r"=\s*(0x[0-9a-fA-F]+)", cmd("i /b 0x%x" % port))
                return int(m.group(1), 16) if m else -1
            cmd("stop", 0.1)
            idx = inb(0x3d4)
            regs = {}
            for r in (0x0e, 0x0f, 0x0a):
                cmd("o /b 0x3d4 0x%x" % r)
                regs[r] = inb(0x3d5)
            if idx >= 0:
                cmd("o /b 0x3d4 0x%x" % idx)
            cmd("cont", 0.1)
            pos = (regs[0x0e] << 8 | regs[0x0f]) if regs[0x0e] >= 0 and regs[0x0f] >= 0 else -1
            seen.append("=== CURSOR %s %d %d %d" % (rest.strip(), pos // 80 if pos >= 0 else -1, pos % 80 if pos >= 0 else -1, regs[0x0a]))
        elif op == "cells":                 # R13: the whole text buffer, characters AND attributes (25 rows of 80 hex pairs)
            out = cmd("xp /4000xb 0xb8000")
            vals = []
            for line in out.splitlines():
                m = re.match(r"^\s*0*b8[0-9a-f]{3}:\s*(.*)$", line)
                if m:
                    vals += [int(x, 16) for x in re.findall(r"0x([0-9a-f]{2})", m.group(1))]
            seen.append("=== CELLS %s %d" % (rest.strip(), len(vals)))
            for r in range(25):
                seen.append("C%02d|%s" % (r, "".join("%02x%02x" % (vals[2 * (r * 80 + c)], vals[2 * (r * 80 + c) + 1])
                                                    for c in range(80) if 2 * (r * 80 + c) + 1 < len(vals))))
            seen.append("=== CELLS-END")
        elif op == "screendump":            # R12B: the VGA picture through the monitor (PPM in OUTDIR, NAME.ppm)
            cmd("screendump %s/%s.ppm" % (__import__("os").path.abspath(O), rest.strip()), 1.0)
            seen.append("=== SCREENDUMP " + rest.strip())
        continue
    nowait = c.startswith("!")           # "!text": console input for the running program, do not wait for the prompt
    if c[:1] in "!@":
        c = c[1:]
    for ch in c + "\n":
        cmd("sendkey " + key(ch), 0.12)
        sample()
    if nowait:
        run_until(lambda rows: False, 1.5)
        continue
    if c.strip().upper() == "EXIT":      # the shell ends: no prompt follows
        if not run_until(lambda rows: any("exited with status" in r for r in rows), 60):
            seen.append("=== EXIT-TIMEOUT")
        continue
    # done when a bare prompt is the last non-empty row again (right after typing, the last row is the echoed command)
    if not run_until(prompt_last, float(__import__("os").environ.get("QEMU_CMD_TIMEOUT", "300"))):
        seen.append("=== PROMPT-TIMEOUT")
    run_until(lambda rows: False, 0.4)
open(O + "/lines.txt", "w").write("\n".join(seen) + "\n")
open(O + "/samples.txt", "w").write(str(samples) + "\n")
cmd("quit")
time.sleep(1)
p.kill()
print("done", len(seen), samples)
