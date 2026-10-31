#!/usr/bin/env python3
"""R11K bootloader boundary matrix (SCRIPTED; no person).  Boots the images made by
scripts/r11k-boot-images.sh on Tiny386 (plain kernels) and QEMU (VGA kernels) and judges each case
independently of the loader:
  NORMAL     new loader (plain/vga = default kernel, pgplain/pgvga = -DQDOS_PAGING kernel), kernel + transfer from 119 to 896 sectors: must reach the shell and the kernel's
             own BIGK check must verify exactly the pad length recorded at build time
  LEGACY     original loader controls: 119/120 boot; forced 128 boots; forced 129 hangs silently
  REFUSE     new loader, deliberately bad images: counts (N), truncated (R), corrupt (S); each must stop
             with exactly that diagnostic, never reach the shell, never print BIGK
  INTEGRITY  QEMU only: guest memory after boot equals the disk bytes (kernel text at 0x100000, transfer
             area at 0x340000); after a refusal nothing was copied (high memory still zero apart from the
             4-byte A20 probe at 0x100500)
usage: TINY386=.. FW=.. python3 scripts/r11k-boot-matrix.py OUTDIR [rounds] [volume]
"""
import json, os, re, subprocess, struct, sys, time, pathlib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from r11kqemu import Qemu

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = pathlib.Path(sys.argv[1]); OUT.mkdir(parents=True, exist_ok=True)
ROUNDS = int(sys.argv[2]) if len(sys.argv) > 2 else 1
VOL = sys.argv[3] if len(sys.argv) > 3 else str(ROOT / "build/fat16/win1-r11j.img")
TINY = os.environ["TINY386"]; FW = os.environ["FW"]
IMG = ROOT / "build/r11k/bootimg"
WORK = OUT / "img"; WORK.mkdir(exist_ok=True)
manifest = {}
for line in open(IMG / "manifest.txt"):
    v, t, pad, text, raw = line.split(); manifest[(v, int(t))] = dict(pad=int(pad), text=int(text), raw=int(raw))

results = []
def rec(rnd, case, plat, expect, got, ok, note=""):
    results.append(dict(round=rnd, case=case, platform=plat, expect=expect, got=got, verdict="PASS" if ok else "FAIL", note=note))
    print("%-4s r%d %-28s %-6s expect=%-10s got=%s %s" % ("PASS" if ok else "FAIL", rnd, case, plat, expect, got, note), flush=True)

def tiny(img, timeout=40):
    r = subprocess.run([sys.executable, str(ROOT / "scripts/r11k-pcboot.py"), TINY, FW, str(img), VOL, str(timeout)],
                       capture_output=True, text=True, errors="replace").stdout
    state = re.search(r"RESULT (\w+)", r).group(1)
    diag = re.search(r"^DIAG (\w)", r, re.M)
    big = re.search(r"BIGK: (\d+) bytes verified", r)
    return state, (diag.group(1) if diag else None), (int(big.group(1)) if big else None), ("MISMATCH" in r), r

def qemu_boot(img, timeout=40):
    q = Qemu(str(img), VOL)
    try:
        ok, rows = q.wait_for(lambda rows: any("Type HELP" in r for r in rows) or any("QBOOT:" in r for r in rows), timeout)
        time.sleep(0.5); rows = q.screen()
        txt = "\n".join(rows)
        diag = re.search(r"QBOOT:([A-Z])", txt)
        big = re.search(r"BIGK: (\d+) bytes verified", txt)
        booted = "Type HELP" in txt
        return q, ("PROMPT" if booted else "NOPROMPT"), (diag.group(1) if diag else None), (int(big.group(1)) if big else None), ("MISMATCH" in txt), txt
    except Exception:
        q.close(); raise

def check_normal(rnd, plat, v, t, run):
    m = manifest[(v, t)]
    st, diag, big, mism, _ = run
    rec(rnd, "normal-%s-%d" % (v, t), plat, "boot+BIGK=%d" % m["pad"], "%s big=%s%s" % (st, big, " MISMATCH" if mism else ""),
        st == "PROMPT" and big == m["pad"] and not mism and diag is None)

def patched(img_bytes, **kw):
    b = bytearray(img_bytes)
    if "ksec" in kw: b[0x1C2:0x1C4] = kw["ksec"].to_bytes(2, "little")
    if "xsec" in kw: b[0x1C4:0x1C6] = kw["xsec"].to_bytes(2, "little")
    if "csum" in kw: b[0x1C6:0x1CA] = kw["csum"].to_bytes(4, "little")
    return bytes(b)

def refusal_cases(g):
    ks = int.from_bytes(g[0x1C2:0x1C4], "little"); xs = int.from_bytes(g[0x1C4:0x1C6], "little")
    cs = int.from_bytes(g[0x1C6:0x1CA], "little")
    def flip(off): return g[:off] + bytes([g[off] ^ 0x01]) + g[off + 1:]
    return [
        ("count-kernel-zero", "N", patched(g, ksec=0)), ("count-xfer-zero", "N", patched(g, xsec=0)),
        ("count-xfer-513", "N", patched(g, xsec=513)), ("count-sum-897", "N", patched(g, ksec=897 - xs)),
        ("count-kernel-ffff", "N", patched(g, ksec=0xFFFF)), ("count-wrap-16bit", "N", patched(g, ksec=0xFFFF, xsec=2)),
        ("truncated-mid-kernel", "RS", g[:512 + (ks // 2) * 512]), ("truncated-last-sector", "RS", g[:len(g) - 512]),
        ("truncated-header-larger", "RS", patched(g, ksec=ks + 300, csum=cs)),
        ("corrupt-kernel-byte", "S", flip(512 + 40)), ("corrupt-kernel-last-sector", "S", flip(512 + ks * 512 - 7)),
        ("corrupt-transfer-area", "S", flip(512 + ks * 512 + 9)), ("corrupt-checksum-field", "S", patched(g, csum=cs ^ 0x10)),
    ]

for rnd in range(1, ROUNDS + 1):
    # --- NORMAL ---
    for (v, t) in sorted(manifest, key=lambda k: (k[0], k[1])):
        if v in ("plain", "pgplain"):
            check_normal(rnd, "tiny386", v, t, tiny(IMG / ("%s-new-%d.img" % (v, t))))
        else:
            q, st, diag, big, mism, txt = qemu_boot(IMG / ("%s-new-%d.img" % (v, t))); q.close()
            check_normal(rnd, "qemu", v, t, (st, diag, big, mism, txt))
    # --- LEGACY controls ---
    for t in (119, 120):
        m = manifest[("plain", t)]
        st, diag, big, mism, _ = tiny(IMG / ("plain-old-%d.img" % t))
        rec(rnd, "legacy-loader-%d" % t, "tiny386", "boot+BIGK=%d" % m["pad"], "%s big=%s" % (st, big), st == "PROMPT" and big == m["pad"])
        m = manifest[("vga", t)]
        q, st, diag, big, mism, txt = qemu_boot(IMG / ("vga-old-%d.img" % t)); q.close()
        rec(rnd, "legacy-loader-%d" % t, "qemu", "boot+BIGK=%d" % m["pad"], "%s big=%s" % (st, big), st == "PROMPT" and big == m["pad"])
    for t, want in ((128, "PROMPT"), (129, "NOPROMPT")):
        for plat, v, boot_img in (("tiny386", "plain", None), ("qemu", "vga", None)):
            raw = IMG / ("%s-kernel-119.raw" % v)
            img = WORK / ("forced-legacy-%s-%d.img" % (v, t))
            subprocess.run([sys.executable, str(ROOT / "tools/r11k-oldlimit-image.py"), str(ROOT / "build/pc-r9/boot.bin"),
                            str(raw), str(ROOT / "build/xfer.bin"), str(t), str(img)], check=True, capture_output=True)
            if plat == "tiny386":
                st, diag, *_ = tiny(img, 25)
            else:
                q, st, diag, *_ = qemu_boot(img, 25); q.close()
            rec(rnd, "legacy-forced-%d-sectors" % t, plat, want, "%s diag=%s" % (st, diag),
                st == want and diag is None, "(the original loader has no diagnostic: a failed read is a silent hang)" if want == "NOPROMPT" else "")
    # --- REFUSE ---
    for plat, v in (("tiny386", "plain"), ("qemu", "vga")):
        g = (IMG / ("%s-new-200.img" % v)).read_bytes()
        for name, want, data in refusal_cases(g):
            p = WORK / ("refuse-%s-%s.img" % (v, name)); p.write_bytes(data)
            if plat == "tiny386":
                st, diag, big, mism, _ = tiny(p, 20)
            else:
                q, st, diag, big, mism, _ = qemu_boot(p, 15); q.close()
            # A truncated image is caught either by the BIOS read (R) or, where the emulator's disk
            # returns zeros past the end of the file, by the checksum (S): both are refusals.
            rec(rnd, "refuse-" + name, plat, "diag " + "/".join(want), "%s diag=%s big=%s" % (st, diag, big),
                st == "NOPROMPT" and diag is not None and diag in want and big is None)
    # --- INTEGRITY (QEMU) ---
    for t in (200, 896):
        m = manifest[("vga", t)]
        q, st, diag, big, mism, txt = qemu_boot(IMG / ("vga-new-%d.img" % t))
        try:
            raw = (IMG / ("vga-kernel-%d.raw" % t)).read_bytes()
            xfer = (ROOT / "build/xfer.bin").read_bytes()
            text = q.pmemsave(0x100000, m["text"])
            xs = q.pmemsave(0x340000, len(xfer))
            rec(rnd, "memory-kernel-text-%d" % t, "qemu", "equals disk", "equal" if text == raw[:m["text"]] else "DIFFERENT", text == raw[:m["text"]])
            rec(rnd, "memory-transfer-area-%d" % t, "qemu", "equals disk", "equal" if xs == xfer else "DIFFERENT", xs == xfer)
        finally:
            q.close()
    g = (IMG / "vga-new-200.img").read_bytes()
    for name, want, data in refusal_cases(g):
        if name not in ("count-kernel-zero", "truncated-mid-kernel", "corrupt-kernel-byte", "corrupt-transfer-area"):
            continue
        p = WORK / ("refuse-vga-%s.img" % name)
        q, st, diag, *_ = qemu_boot(p, 15)
        try:
            hi = q.pmemsave(0x100000, 0x2000); hx = q.pmemsave(0x340000, 0x1000)
            allowed = bytearray(len(hi))   # the A20 probe writes the complement of the dword at 0x500 to 0x100500 (S/A paths only)
            nz = [i for i, b in enumerate(hi) if b]
            ok = (not hx.strip(b"\0")) and (nz == [] or ("S" in want and nz and min(nz) >= 0x500 and max(nz) < 0x504))
            rec(rnd, "no-copy-after-refusal-" + name, "qemu", "high memory untouched", "nonzero offsets %s" % (nz[:6],) if nz else "all zero", ok)
        finally:
            q.close()
    json.dump(results, open(OUT / "boot-matrix.json", "w"), indent=1)

npass = sum(r["verdict"] == "PASS" for r in results); nfail = len(results) - npass
print("BOOT MATRIX: %d PASS, %d FAIL (%d rounds)" % (npass, nfail, ROUNDS))
with open(OUT / "boot-matrix.txt", "w") as f:
    for r in results:
        f.write("%s r%d %s %s expect=%s got=%s %s\n" % (r["verdict"], r["round"], r["platform"], r["case"], r["expect"], r["got"], r["note"]))
    f.write("BOOT MATRIX: %d PASS, %d FAIL (%d rounds)\n" % (npass, nfail, ROUNDS))
sys.exit(1 if nfail else 0)
