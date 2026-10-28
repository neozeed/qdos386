"""R11K helper: a scripted QEMU session (monitor over TCP; no person).  Used by the boot matrix and
the acceptance drivers.  Keys are sent with 'sendkey'; the 80x25 VGA text buffer is read with xp;
guest physical memory is dumped with pmemsave."""
import os, re, shutil, socket, subprocess, tempfile, time

KEYMAP = {' ': 'spc', '.': 'dot', '-': 'minus', '\n': 'ret', '_': 'shift-minus', '/': 'slash',
          ',': 'comma', '=': 'equal', ':': 'shift-semicolon', ';': 'semicolon', '"': 'shift-apostrophe',
          "'": 'apostrophe', '!': 'shift-1', '?': 'shift-slash', '*': 'shift-8', '+': 'shift-equal'}

class Qemu:
    def __init__(self, hda, hdb=None, port=None, mem=16, serial_file=None):
        self.wd = tempfile.mkdtemp(prefix="r11kq-")
        shutil.copy(hda, self.wd + "/hda.img")
        self.port = port or 4600 + (os.getpid() * 7 + int(time.time() * 10)) % 3000
        cmd = ["qemu-system-i386", "-machine", "pc", "-cpu", "pentium", "-m", str(mem),
               "-bios", "/usr/share/seabios/bios.bin", "-vga", "std", "-display", "none",
               "-monitor", "tcp:127.0.0.1:%d,server,nowait" % self.port,
               "-drive", "file=%s/hda.img,format=raw,if=ide,index=0,media=disk" % self.wd]
        if hdb:
            shutil.copy(hdb, self.wd + "/hdb.img")
            cmd += ["-drive", "file=%s/hdb.img,format=raw,if=ide,index=1,media=disk" % self.wd]
        cmd += ["-boot", "order=c", "-no-reboot"]
        self.p = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.s = None
        for _ in range(40):
            try:
                self.s = socket.create_connection(("127.0.0.1", self.port)); break
            except OSError:
                time.sleep(0.25)
        if self.s is None:
            raise RuntimeError("QEMU monitor did not come up")
        self.s.settimeout(5)
        self.mon("")

    def mon(self, c, wait=0.0):
        self.s.sendall((c + "\n").encode()); time.sleep(wait); buf = b""
        try:
            while True:
                d = self.s.recv(65536)
                if not d: break
                buf += d
                if buf.endswith(b"(qemu) "): break
        except Exception:
            pass
        return buf.decode(errors="replace")

    def screen(self):
        out = self.mon("xp /4000xb 0xb8000"); vals = []
        for line in out.splitlines():
            m = re.match(r"^\s*0*b8[0-9a-f]{3}:\s*(.*)$", line)
            if m: vals += [int(x, 16) for x in re.findall(r"0x([0-9a-f]{2})", m.group(1))]
        ch = vals[0::2]
        return [bytes(ch[r * 80:(r + 1) * 80]).decode("cp437", "replace").rstrip() for r in range(25)]

    def wait_for(self, pred, timeout, step=0.3):
        t0 = time.time(); rows = []
        while time.time() - t0 < timeout:
            rows = self.screen()
            if pred(rows): return True, rows
            time.sleep(step)
        return False, rows

    def key(self, ch):
        if ch.isupper(): return "shift-" + ch.lower()
        return KEYMAP.get(ch, ch)

    def type_line(self, text):
        for ch in text + "\n":
            self.mon("sendkey " + self.key(ch), 0.12)

    def pmemsave(self, addr, length):
        path = os.path.join(self.wd, "dump-%x-%x.bin" % (addr, length))
        self.mon("pmemsave 0x%x %d \"%s\"" % (addr, length, path))
        for _ in range(50):
            if os.path.exists(path) and os.path.getsize(path) == length:
                break
            time.sleep(0.1)
        return open(path, "rb").read()

    def close(self):
        try: self.mon("quit")
        except Exception: pass
        time.sleep(0.3); self.p.kill()
        shutil.rmtree(self.wd, ignore_errors=True)
