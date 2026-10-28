#!/usr/bin/env python3
# R11L (copy of the R11K script, with more keys, plus --seq FILE) VGA text capture under QEMU (SCRIPTED keys through the monitor, not a person).
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
PORT = 4471
p = subprocess.Popen(["qemu-system-i386", "-machine", "pc", "-cpu", "pentium", "-m", "16",
    "-bios", "/usr/share/seabios/bios.bin", "-vga", "std", "-display", "none",
    "-monitor", "tcp:127.0.0.1:%d,server,nowait" % PORT,
    "-drive", "file=%s/hda.img,format=raw,if=ide,index=0,media=disk" % W,
    "-drive", "file=%s/hdb.img,format=raw,if=ide,index=1,media=disk" % W,
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

run_until(lambda rows: any("Type HELP" in r for r in rows), 90)
for c in CMDS:
    seen.append("=== CMD " + c)          # marker: lets the judge cut the capture into one segment per typed command
    nowait = c.startswith("!")           # "!text": console input for the running program, do not wait for the prompt
    if c[:1] in "!@":
        c = c[1:]
    for ch in c + "\n":
        cmd("sendkey " + key(ch), 0.12)
        sample()
    # Done when the shell's bare prompt ">" is the last row again.  Right after typing, the last
    # row is the echoed command; a program's output never consists of a bare ">".
    # R11K: judged on the last NON-EMPTY row (the R11J test used rows[-1], which is only the prompt once the
    # screen has scrolled; a short transcript never reaches the bottom row and waited out the 900 s limit).
    if nowait:
        run_until(lambda rows: False, 1.5)
        continue
    run_until(lambda rows: bool([r for r in rows if r.strip()]) and [r for r in rows if r.strip()][-1].strip() == ">", 900)
    run_until(lambda rows: False, 3)
open(O + "/lines.txt", "w").write("\n".join(seen) + "\n")
open(O + "/samples.txt", "w").write(str(samples) + "\n")
cmd("quit")
time.sleep(1)
p.kill()
print("done", len(seen), samples)
