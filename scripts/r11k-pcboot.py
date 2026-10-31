#!/usr/bin/env python3
# R11K Tiny386 boot probe (SCRIPTED, no person).  Boots a PC disk image under the Tiny386 emulator
# with -term, collects the debug-port console output, and reports whether the shell prompt appeared.
#   usage: r11k-pcboot.py TINY386 FIRMWARE_DIR HDA.img HDB.img [timeout_s] [extra shell lines...]
# Output (stdout): "RESULT PROMPT" or "RESULT NOPROMPT" or "RESULT EXIT", then the console text.
# A refused image never reaches the prompt; the loader's one-line diagnostic (QBOOT:<code>) is the text.
import os, re, select, shutil, subprocess, sys, tempfile, time
tiny, fw, hda, hdb = [os.path.abspath(a) for a in sys.argv[1:5]]
timeout = float(sys.argv[5]) if len(sys.argv) > 5 else 40.0
lines = sys.argv[6:]
wd = tempfile.mkdtemp(prefix="r11k-pcboot-")
for f in ("bios.bin", "vgabios.bin"):
    shutil.copy(os.path.join(fw, f), wd)
shutil.copy(hda, os.path.join(wd, "hda.img"))
if hdb != os.path.abspath("/dev/null"):
    shutil.copy(hdb, os.path.join(wd, "hdb.img"))
ini = "[pc]\nbios = bios.bin\nvga_bios = vgabios.bin\nmem_size = 8M\nhda = hda.img\n"
if os.path.exists(os.path.join(wd, "hdb.img")):
    ini += "hdb = hdb.img\n"
ini += "\n[cpu]\nfpu = 1\n"
open(os.path.join(wd, "run.ini"), "w").write(ini)
p = subprocess.Popen([tiny, "-term", "run.ini"], cwd=wd, stdin=subprocess.PIPE,
                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
os.set_blocking(p.stdout.fileno(), False)
buf = b""
t0 = time.time()
state = "NOPROMPT"
sent = 0
PROMPT = re.compile(rb"(^|\n)> $")
def keys(s):
    # session.py's key path would be better; the probe only needs the prompt.  Extra lines are
    # typed as raw bytes, which -term turns into PS/2 keys.
    return s.encode() + b"\n"
while time.time() - t0 < timeout:
    r, _, _ = select.select([p.stdout], [], [], 0.2)
    if r:
        try:
            d = p.stdout.read(65536)
        except Exception:
            d = b""
        if d:
            buf += d
    if p.poll() is not None:
        state = "EXIT"
        break
    clean = re.sub(rb"\x1b\[[0-9;?]*[A-Za-z]|\x1b[78]|\r", b"", buf)
    if re.search(rb"Q+B+O+T+:+[A-Z]+\n", clean):    # the loader refused the image and halted
        time.sleep(0.5)
        break
    if PROMPT.search(clean):
        if sent < len(lines):
            p.stdin.write(keys(lines[sent])); p.stdin.flush(); sent += 1
            time.sleep(0.5)
            buf_mark = len(buf)
            continue
        state = "PROMPT"
        break
time.sleep(0.3)
try:
    d = p.stdout.read(65536)
    if d:
        buf += d
except Exception:
    pass
p.kill()
shutil.rmtree(wd, ignore_errors=True)
txt = re.sub(rb"\x1b\[[0-9;?]*[A-Za-z]|\x1b[78]|\r", b"", buf).decode("latin-1")
print("RESULT", state)
# The loader prints "QBOOT:<c>" on the debug port and the VGA text; -term shows both, so letters repeat.
m = re.search(r"Q+B+O+T+:+([A-Z])", txt)
if m:
    print("DIAG", m.group(1))
sys.stdout.write(txt)
