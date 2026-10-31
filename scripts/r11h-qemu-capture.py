#!/usr/bin/env python3
# R11H VGA text capture: drives QEMU through its monitor (sendkey), reads the VGA text buffer at
# 0xB8000 with "xp" in a tight loop, and records every changed screen, so output that scrolls off the
# 80x25 console is kept.  Usage: python3 scripts/r11h-qemu-capture.py OUTDIR WORKDIR REPODIR
# WORKDIR holds hda.img (VGA kernel) and hdb.img (volume).  Test-only; keystrokes are scripted.
import socket, time, sys, subprocess, re
O, W, R = sys.argv[1], sys.argv[2], sys.argv[3]
p = subprocess.Popen(["qemu-system-i386","-machine","pc","-cpu","pentium","-m","16",
  "-bios","/usr/share/seabios/bios.bin","-vga","std","-display","none",
  "-monitor","tcp:127.0.0.1:4459,server,nowait",
  "-drive",f"file={W}/hda.img,format=raw,if=ide,index=0,media=disk",
  "-drive",f"file={W}/hdb.img,format=raw,if=ide,index=1,media=disk",
  "-boot","order=c","-no-reboot"], cwd=R, stdout=open(O+"/stderr.txt","w"), stderr=subprocess.STDOUT)
time.sleep(2)
s = socket.create_connection(("127.0.0.1",4459)); s.settimeout(5)
def cmd(c, wait=0.05):
    s.sendall((c+"\n").encode()); time.sleep(wait)
    buf=b""
    try:
        while True:
            d=s.recv(65536)
            if not d: break
            buf+=d
            if buf.endswith(b"(qemu) "): break
    except Exception: pass
    return buf.decode(errors="replace")
KEY = {' ':'spc','.':'dot','-':'minus','\n':'ret'}
def typ(text, pause=0.12):
    for ch in text:
        cmd("sendkey "+KEY.get(ch,ch), pause)
def screen():
    out = cmd("xp /4000xb 0xb8000", 0.0)
    vals=[]
    for line in out.splitlines():
        m = re.match(r"^\s*0*b8[0-9a-f]{3}:\s*(.*)$", line)
        if m: vals += [int(x,16) for x in re.findall(r"0x([0-9a-f]{2})", m.group(1))]
    chars = vals[0::2]
    return [bytes(chars[r*80:(r+1)*80]).decode("cp437",errors="replace").rstrip() for r in range(25)]
time.sleep(30)
typ("run multi.aou\n"); time.sleep(45)
typ("run neg.aou\n")
seen=[]; t_end=time.time()+120; samples=0; prev=None
while time.time()<t_end:
    rows=screen(); samples+=1
    if rows!=prev:
        seen.append("=== SNAP %d" % samples)
        seen += [r for r in rows if r]
        prev=rows
    time.sleep(0.02)
open(O+"/lines.txt","w").write("\n".join(seen)+"\n")
open(O+"/samples.txt","w").write(str(samples)+"\n")
cmd("quit"); time.sleep(1); p.kill()
print("done", len(seen), samples)
