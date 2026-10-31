#!/usr/bin/env python3
# R11K QEMU boot probe (SCRIPTED, no person).  Boots a PC disk image with qemu-system-i386 (SeaBIOS,
# -vga std, display none), samples the 80x25 VGA text buffer through the monitor, and reports
# whether the shell banner ("Type HELP") appeared, or what the screen shows at the timeout.
#   usage: r11k-qemuboot.py HDA.img HDB.img|- [timeout_s] [port]
# Output: "RESULT BOOTED" or "RESULT NOBOOT", then the final screen rows (non-empty).
import os, re, shutil, socket, subprocess, sys, tempfile, time
hda = os.path.abspath(sys.argv[1]); hdb = sys.argv[2]
timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 40.0
port = int(sys.argv[4]) if len(sys.argv) > 4 else 4471 + (os.getpid() % 500)
wd = tempfile.mkdtemp(prefix="r11k-qemuboot-")
shutil.copy(hda, wd + "/hda.img")
cmd = ["qemu-system-i386", "-machine", "pc", "-cpu", "pentium", "-m", "16",
       "-bios", "/usr/share/seabios/bios.bin", "-vga", "std", "-display", "none",
       "-monitor", "tcp:127.0.0.1:%d,server,nowait" % port,
       "-drive", "file=%s/hda.img,format=raw,if=ide,index=0,media=disk" % wd]
if hdb != "-":
    shutil.copy(hdb, wd + "/hdb.img")
    cmd += ["-drive", "file=%s/hdb.img,format=raw,if=ide,index=1,media=disk" % wd]
cmd += ["-boot", "order=c", "-no-reboot"]
p = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
s = None
for _ in range(20):
    try:
        s = socket.create_connection(("127.0.0.1", port)); break
    except OSError:
        time.sleep(0.3)
s.settimeout(5)
def mon(c):
    s.sendall((c + "\n").encode()); buf = b""
    try:
        while True:
            d = s.recv(65536)
            if not d: break
            buf += d
            if buf.endswith(b"(qemu) "): break
    except Exception:
        pass
    return buf.decode(errors="replace")
def screen():
    out = mon("xp /4000xb 0xb8000"); vals = []
    for line in out.splitlines():
        m = re.match(r"^\s*0*b8[0-9a-f]{3}:\s*(.*)$", line)
        if m: vals += [int(x, 16) for x in re.findall(r"0x([0-9a-f]{2})", m.group(1))]
    ch = vals[0::2]
    return [bytes(ch[r*80:(r+1)*80]).decode("cp437", "replace").rstrip() for r in range(25)]
mon("")
t0 = time.time(); rows = []; state = "NOBOOT"
while time.time() - t0 < timeout:
    rows = screen()
    if any("Type HELP" in r for r in rows):
        state = "BOOTED"; break
    time.sleep(0.4)
mon("quit"); time.sleep(0.3); p.kill(); shutil.rmtree(wd, ignore_errors=True)
print("RESULT", state)
print("\n".join(r for r in rows if r))
