#!/usr/bin/env python3
"""R11E cross-reference of the historical C68/QDOS qdos/ entry points.

Prototypes are extracted from the C68 sources (comment headers and C
declarations in qdos/trap1, trap2, trap3, vect).  Each symbol is then classified
against the explicit MAP table below, which records the QDOS/386 service that
backs it and the semantic status:
  semantic     same name-level behaviour for the cases implemented
  approximate  works for a documented subset; the convention or scope differs
  native-only  a QDOS/386 function exists with a different convention
  unsupported  no QDOS/386 service; the C68 call is not provided

usage: python3 scripts/r11e-c68-xref.py <C68 libc-4.24.5 dir> docs/R11E-C68-XREF
writes DIR.csv
"""
import csv, os, re, sys

SRC, OUT = sys.argv[1], sys.argv[2]
QD = os.path.join(SRC, "qdos")
PROTO = re.compile(r"^[;\t ]*((?:int|long|void|char|short|jobid_t|chanid_t|unsigned|struct|timeout_t|\*)[^(]*?)\b((?:mt|mm|io|ioa|iow|fs|sd|ut|sms|cn|c|io_q)_?[a-z_]*)\s*\(([^)]*)\)", re.M)

# symbol -> (status, QDOS/386 equivalent or "", note)
MAP = {
 "mt_activ":  ("approximate", "qd_job_activate", "activation without timeout; returns status, not job id"),
 "mt_alloc":  ("native-only", "qd_alloc", "single heap; C68 pointer-pair convention not provided"),
 "mt_alres":  ("unsupported", "", "no resident-procedure area"),
 "mt_reljb":  ("approximate", "qd_job_reactivate", "releases a job suspended by QT_SUSPEND"),
 "mt_prior":  ("approximate", "qd_job_chprio", "returns 0, not the old priority; read it with qd_job_info first"),
 "mt_inf":    ("unsupported", "", "system variable block not exposed"),
 "mt_jinf":   ("unsupported", "", "job pointer and location not exposed"),
 "mt_cjob":   ("unsupported", "", "kernel job creation takes a raw code address; deferred (see R11E-QDOS-API-MAP)"),
 "mt_rjob":   ("unsupported", "", "no job removal trap"),
 "mt_frjob":  ("unsupported", "", "no job removal trap"),
 "mt_shrink": ("unsupported", "", "no heap shrink trap"),
 "mt_lnkfr":  ("unsupported", "", "no heap link-back trap"),
 "mm_lnkfr":  ("unsupported", "", "no heap link-back trap"),
 "mt_rechp":  ("approximate", "qd_free", "one heap; release by block address"),
 "mm_rechp":  ("approximate", "qd_free", "one heap; release by block address"),
 "mm_alloc":  ("native-only", "qd_alloc", "one heap; no common/user split"),
 "mt_susjb":  ("approximate", "qd_job_suspend", "suspends the calling job only; hold pointer not used"),
 "mt_trans":  ("unsupported", "", "no translation table"),
 "mt_trapv":  ("unsupported", "", "no job vector table"),
 "mt_ipcom":  ("unsupported", "", "no second processor"),
 "mt_baud":   ("unsupported", "", "no serial port service"),
 "mt_dmode":  ("unsupported", "", "no display-mode service"),
 "sms_dmod":  ("unsupported", "", "no display-mode service"),
 "mt_free":   ("unsupported", "", "free space query of the C68 kind; use qd_free_bytes (native-only)"),
 "io_open":   ("approximate", "qd_open", "read only; write and append modes do not exist on WIN1_"),
 "io_close":  ("semantic", "qd_close", ""),
 "io_fbyte":  ("approximate", "qd_read", "one byte through qd_read; no timeout argument"),
 "io_delete": ("unsupported", "", "FAT16 volume is read only"),
 "io_rename": ("unsupported", "", "FAT16 volume is read only"),
 "io_format": ("unsupported", "", "no medium formatting"),
 "fs_mkdir":  ("unsupported", "", "FAT16 volume is read only"),
 "fs_pos":    ("approximate", "qd_seek", "absolute positions only"),
 "fs_posab":  ("approximate", "qd_seek", "absolute positions only"),
 "fs_posre":  ("unsupported", "", "relative positioning not provided"),
 "fs_load":   ("unsupported", "", "no binary load primitive; use qd_read"),
 "fs_save":   ("unsupported", "", "FAT16 volume is read only"),
 "fs_date":   ("unsupported", "", "no date service"),
 "fs_mdinf":  ("unsupported", "", "medium information not exposed"),
 "fs_vers":   ("unsupported", "", "no file version key"),
 "io_edlin":  ("unsupported", "", "console line editor not exposed"),
 "io_serq":   ("unsupported", "", "serial queue primitive"),
 "io_qin":    ("unsupported", "", "queue primitive of the C68 kernel"),
 "io_qout":   ("unsupported", "", "queue primitive of the C68 kernel"),
 "io_qeof":   ("unsupported", "", "queue primitive of the C68 kernel"),
 "io_qset":   ("unsupported", "", "queue primitive of the C68 kernel"),
 "io_qtest":  ("unsupported", "", "queue primitive of the C68 kernel"),
 "ut_con":    ("unsupported", "", "window and console definition"),
 "ut_scr":    ("unsupported", "", "screen definition"),
 "ut_windowf":("unsupported", "", "window formatting"),
 "ut_mint":   ("unsupported", "", "text output of integers to a window"),
 "ut_mtext":  ("unsupported", "", "text output to a window; use qd_puts or qd_write on CON"),
 "ut_err":    ("unsupported", "", "error text; C89 perror/strerror are provided instead"),
 "ut_cstr":   ("unsupported", "", "string comparison of the C68 kind; use strcmp"),
 "ut_link":   ("unsupported", "", "utility link list of the C68 kernel"),
 "ut_unlnk":  ("unsupported", "", "utility link list of the C68 kernel"),
 "cn_date":   ("unsupported", "", "no date service"),
 "cn_day":    ("unsupported", "", "no date service"),
 "c_extop":   ("unsupported", "", "external operation trap"),
 "iow_xtop":  ("unsupported", "", "external operation trap"),
 "mt_ldd":    ("unsupported", "", "kernel link list"),
 "mt_liod":   ("unsupported", "", "kernel link list"),
 "mt_lpoll":  ("unsupported", "", "kernel link list"),
 "mt_lschd":  ("unsupported", "", "kernel link list"),
 "mt_lxint":  ("unsupported", "", "kernel link list"),
 "mt_rdd":    ("unsupported", "", "kernel link list"),
 "mt_riod":   ("unsupported", "", "kernel link list"),
 "mt_rpoll":  ("unsupported", "", "kernel link list"),
 "mt_rschd":  ("unsupported", "", "kernel link list"),
 "mt_rxint":  ("unsupported", "", "kernel link list"),
 "fs_headr":  ("unsupported", "", "file header read; no header records on WIN1_"),
 "fs_heads":  ("unsupported", "", "file header write; FAT16 volume is read only"),
 "io_fline":  ("unsupported", "", "line read of the C68 kind; use fgets"),
 "io_fstrg":  ("unsupported", "", "string read of the C68 kind; use fread"),
 "io_sstrg":  ("unsupported", "", "string write; use qd_write on CON"),
 "ioa_sown":  ("unsupported", "", "channel owner change"),
 "mt_alchp":  ("approximate", "qd_alloc", "one heap; the size-got result is not returned"),
 "mm_alchp":  ("approximate", "qd_alloc", "one heap; the size-got result is not returned"),
 "sms_achp":  ("approximate", "qd_alloc", "one heap; the size-got result is not returned"),
 "sms_achp_acsi": ("unsupported", "", "ACSI-specific common heap"),
 "sms_schp":  ("unsupported", "", "SMS common heap shrink"),
 "sms_cach":  ("unsupported", "", "SMS cache control"),
 "sms_iopr":  ("unsupported", "", "SMS I/O priority"),
 "sms_lenq":  ("unsupported", "", "SMS queue"),
 "sms_lldm":  ("unsupported", "", "SMS device loading"),
 "sms_lset":  ("unsupported", "", "SMS setting"),
 "sms_mptr":  ("unsupported", "", "SMS pointer"),
 "sms_pset":  ("unsupported", "", "SMS pointer setting"),
 "sms_sevt":  ("unsupported", "", "SMS event"),
 "sms_wevt":  ("unsupported", "", "SMS event wait"),
 "sd_arc":    ("unsupported", "", "screen graphics"),
 "sd_extop":  ("unsupported", "", "screen graphics"),
 "sd_fount":  ("unsupported", "", "screen font"),
 "sd_gcur":   ("unsupported", "", "screen graphics cursor"),
 "sd_iarc":   ("unsupported", "", "screen graphics"),
 "sd_ielipse":("unsupported", "", "screen graphics"),
 "sd_igcur":  ("unsupported", "", "screen graphics cursor"),
 "sd_iline":  ("unsupported", "", "screen graphics"),
 "sd_ipoint": ("unsupported", "", "screen graphics"),
 "sd_iscale": ("unsupported", "", "screen graphics"),
 "sd_line":   ("unsupported", "", "screen graphics"),
 "sd_point":  ("unsupported", "", "screen graphics"),
 "sd_routine":("unsupported", "", "screen graphics"),
}

def group(sym):
    if sym.startswith(("mt_", "mm_", "sms_")): return "multitasking/heap"
    if sym.startswith(("io_", "ioa_", "iow_")): return "channel I/O"
    if sym.startswith("fs_"): return "file system"
    if sym.startswith("sd_"): return "screen/graphics"
    if sym.startswith("ut_"): return "window/utility"
    if sym.startswith("cn_"): return "date"
    return "other"

seen = {}
for sub in ("trap1", "trap2", "trap3", "vect", "misc"):
    d = os.path.join(QD, sub)
    if not os.path.isdir(d):
        continue
    for fn in sorted(os.listdir(d)):
        if not fn.endswith((".s", ".c", ".x")):
            continue
        text = open(os.path.join(d, fn), errors="replace").read()
        for m in PROTO.finditer(text):
            sym = m.group(2)
            if not sym.startswith(("mt_", "mm_", "io_", "ioa_", "iow_", "fs_", "sd_", "ut_", "sms_", "cn_", "c_", "io_q")):
                continue
            proto = (m.group(1) + sym + "(" + m.group(3) + ")").strip()
            proto = re.sub(r"\s+", " ", proto)
            key = sym
            if key not in seen:
                seen[key] = (sub + "/" + fn, proto)

rows = []
for sym in sorted(seen):
    where, proto = seen[sym]
    status, native, note = MAP.get(sym, ("unsupported", "", "not classified"))
    rows.append([sym, where, proto, group(sym), status, native, note])

os.makedirs(os.path.dirname(OUT) or ".", exist_ok=True)
with open(OUT + ".csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["c68_symbol", "c68_source", "c68_prototype", "group", "r11e_status", "qdos386_equivalent", "notes"])
    w.writerows(rows)
counts = {}
for r in rows:
    counts[r[4]] = counts.get(r[4], 0) + 1
print("symbols", len(rows), counts)
unmapped = [r[0] for r in rows if r[6] == "not classified"]
print("unclassified", unmapped)
