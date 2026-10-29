/*
 * bastest.c - BASTEST.AOU: the SuperBASIC/C89 regression runner, run inside QDOS/386.
 *
 * Derived from the supplied tests/sbtest.c: the unit checks and the corpus
 * runner (run_source / run_file) are the same code; only the driver around
 * them changed (a manifest instead of a command line of files, results
 * written to a file, the run made on the private stack).
 *
 *   BASTEST [/K] [/O:results] [/U] @manifest | file.bas ...
 *
 * Each FILE.bas is executed line by line in a fresh interpreter, output (channels
 * #0..#3) is captured and compared byte for byte with FILE.out; error reports are
 * not compared; every run also checks that all core heap blocks were released.
 * @manifest names a text file with one path per line, the path of FILE.bas
 * (the extension may be omitted).  /K marks the files as documented
 * discrepancies (a mismatch is KNOWN, not FAIL).  /U skips the unit checks.
 * Result lines go to the results file (default none) and the console shows
 * every line that is not PASS plus the totals.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sb_chan.h"
#include "sb_error.h"
#include "sb_format.h"
#include "sb_interp.h"
#include "sb_lex.h"
#include "sb_mem.h"
#include "sb_state.h"
#include "qdos386.h"
#include "qb.h"

/* Result lines: always to the results file (if any); to the console unless
 * quiet_pass says this is a PASS line. */
static FILE *resfile;
static int quiet_pass;

static void report(const char *fmt, ...)
{
    char buf[400];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    if (resfile != NULL) {
        fputs(buf, resfile);
    }
    if (!(quiet_pass && strncmp(buf, "PASS ", 5) == 0)
        && !(quiet_pass && strncmp(buf, "KNOWN-OK ", 9) == 0)) {
        fputs(buf, stdout);
    }
}

/* ---- capture buffer -------------------------------------------------- */

typedef struct Capture {
    char *data;
    size_t len;
    size_t cap;
} Capture;

static void capture_put(Capture *c, const char *s, size_t n)
{
    size_t need = c->len + n + 1;
    if (need > c->cap) {
        size_t ncap = (c->cap == 0) ? 256 : c->cap;
        char *nd;
        while (ncap < need) {
            ncap *= 2;
        }
        nd = (char *)realloc(c->data, ncap);
        if (nd == NULL) {
            return;
        }
        c->data = nd;
        c->cap = ncap;
    }
    if (n > 0) {
        memcpy(c->data + c->len, s, n);
    }
    c->len += n;
    c->data[c->len] = '\0';
}

/* One corpus run: the captured output, the source text, and the next unread
 * byte of the source.  Program lines and INPUT both read from pos, so an
 * INPUT takes the lines that follow it, as the ROM takes typed lines. */
typedef struct Run {
    Capture *cap;
    const char *src;
    size_t srclen;
    size_t pos;
} Run;

static int feed_in(void *ctx)
{
    Run *r = (Run *)ctx;
    if (r->pos >= r->srclen) {
        return -1;
    }
    return (unsigned char)r->src[r->pos++];
}

/* Channel adapters for a corpus run.  Output is captured; error reports are
 * discarded (they are kept out of the compared output). */
static int cap_write(void *obj, const char *s, size_t n, size_t *done)
{
    capture_put(((Run *)obj)->cap, s, n);
    *done = n;
    return SB_OK;
}

static int feed_read(void *obj, char *buf, size_t n, size_t *got)
{
    int c;

    *got = 0;
    if (n == 0) {
        return SB_OK;
    }
    c = feed_in(obj);
    if (c < 0) {
        return SB_OK;                  /* end of input */
    }
    buf[0] = (char)c;
    *got = 1;
    return SB_OK;
}

static int discard_write(void *obj, const char *s, size_t n, size_t *done)
{
    (void)obj;
    (void)s;
    *done = n;
    return SB_OK;
}

static const SbChanOps run_io_ops = { cap_write, feed_read, NULL, NULL };
static const SbChanOps run_out_ops = { cap_write, NULL, NULL, NULL };
static const SbChanOps discard_ops = { discard_write, NULL, NULL, NULL };

/* ---- unit checks ----------------------------------------------------- */

static int failures = 0;

static void check_text(const char *what, const char *got, const char *want)
{
    if (strcmp(got, want) != 0) {
        report("FAIL unit %s: got \"%s\", want \"%s\"\n", what, got, want);
        failures++;
    }
}

static void unit_format(void)
{
    static const struct { double v; const char *want; } cases[] = {
        { 0.0, "0" },
        { -0.0, "0" },
        { 1.0, "1" },
        { -5.0, "-5" },
        { 1.0 / 3.0, ".3333333" },
        { 1.414213562, "1.414214" },
        { 1234567.0, "1.234567E6" },
        { 1234567.5, "1.234568E6" },
        { 999999.5, "999999.5" },
        { 999999.95, "1E6" },
        { 1000000.0, "1E6" },
        { 100000.0, "100000" },
        { 9999999.5, "1E7" },
        { 0.1, ".1" },
        { 0.09999999, "9.999999E-2" },
        { 0.01, "1E-2" },
        { 0.0123, "1.23E-2" },
        { 0.001, "1E-3" },
        { 0.0001234, "1.234E-4" },
        { 0.5, ".5" },
        { -0.25, "-.25" },
        { 1.5e10, "1.5E10" },
        { 2147483648.0, "2.147484E9" },
        { 9007199254740992.0, "9.007199E15" },
        { 12345.6789, "12345.68" },
        { 1e-100, "1E-100" },
        { 1e300, "1E300" },
        { 3.0, "3" }
    };
    size_t i;
    char buf[64];

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        sb_format_number(sb_ql_quantise(cases[i].v), buf, sizeof(buf));
        check_text("format", buf, cases[i].want);
    }
}

static void unit_errors(void)
{
    check_text("err -17", sb_error_text(-17), "error in expression");
    check_text("err -18", sb_error_text(-18), "overflow");
    check_text("err -21", sb_error_text(-21), "bad line");
    check_text("err 5", sb_error_text(5), "unknown error");
    check_text("err -99", sb_error_text(-99), "unknown error");
}

static void unit_canonical(void)
{
    static const struct { const char *src; const char *want; } cases[] = {
        { "print x", "PRINT x" },
        { "  for i=1 to 3 : next i", "  FOR i=1 TO 3 : NEXT i" },
        { "GOTO 10", "GO TO 10" },
        { "go   to 10", "GO TO 10" },
        { "rem hello there", "REMark hello there" },
        { "Print \"Hi GOTO\"", "PRINT \"Hi GOTO\"" },
        { "x=1 : y$=\"a\"", "x=1 : y$=\"a\"" }
    };
    size_t i;
    char *out;
    size_t outlen;
    SbAlloc a;
    char got[256];

    sb_alloc_default_init(&a, NULL);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        if (sb_lex_canonical(&a, cases[i].src, strlen(cases[i].src), &out, &outlen)
            != SB_OK) {
            report("FAIL unit canonical: %s\n", cases[i].src);
            failures++;
            continue;
        }
        memcpy(got, out, outlen);
        got[outlen] = '\0';
        check_text("canonical", got, cases[i].want);
        sb_mem_free(&a, out);
    }
}

/* ---- hosted safeguards ----------------------------------------------- */

/* A program that jumps to itself must stop at the step limit rather than
 * run forever.  The limit is a hosted safeguard; the ROM has none. */
static void unit_step_limit(void)
{
    static const char *lines[] = { "10 GOTO 10", "RUN" };
    SbAlloc a;
    SbState st;
    SbChan *c_out;
    SbChan *c_diag;
    long live = 0;
    int rc = 0;
    size_t i;

    sb_alloc_default_init(&a, &live);
    sb_init(&st, &a);
    c_out = sb_chan_new(&a, &discard_ops, NULL, SB_CH_WRITE);
    c_diag = sb_chan_new(&a, &discard_ops, NULL, SB_CH_WRITE);
    sb_set_channel(&st, 1, c_out);
    sb_set_diag(&st, c_diag);
    st.step_limit = 1000;
    for (i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        rc = sb_execute(&st, lines[i], strlen(lines[i]));
    }
    if (rc != SB_E_OUT_OF_RANGE || st.err != SB_E_OUT_OF_RANGE) {
        report("FAIL unit step limit: got %d\n", rc);
        failures++;
    }
    sb_destroy(&st);
    sb_chan_free(c_out);
    sb_chan_free(c_diag);
    if (live != 0) {
        report("FAIL unit step limit: %ld blocks leaked\n", live);
        failures++;
    }
}

/* ---- channel handle checks -------------------------------------------- */

/* A scripted adapter.  Each mode is independent: max_write limits bytes per
 * call (partial writes), zero makes writes make no progress, lie reports
 * more bytes than given, fail_code makes writes fail, and input feeds
 * reads. */
typedef struct Fake {
    char buf[512];
    size_t len;
    int writes;
    size_t max_write;
    int zero;
    size_t lie;
    int fail_code;
    const char *input;
    size_t in_len;
    size_t in_pos;
    int closed;
} Fake;

static int fake_write(void *obj, const char *s, size_t n, size_t *done)
{
    Fake *f = (Fake *)obj;
    size_t k = n;

    f->writes++;
    if (f->fail_code != 0) {
        *done = 0;
        return f->fail_code;
    }
    if (f->zero) {
        *done = 0;
        return SB_OK;
    }
    if (f->lie != 0) {
        *done = f->lie;
        return SB_OK;
    }
    if (f->max_write != 0 && k > f->max_write) {
        k = f->max_write;
    }
    if (f->len + k > sizeof(f->buf)) {
        k = sizeof(f->buf) - f->len;
    }
    memcpy(f->buf + f->len, s, k);
    f->len += k;
    *done = k;
    return SB_OK;
}

static int fake_read(void *obj, char *buf, size_t n, size_t *got)
{
    Fake *f = (Fake *)obj;
    size_t k = f->in_len - f->in_pos;

    if (k > n) {
        k = n;
    }
    memcpy(buf, f->input + f->in_pos, k);
    f->in_pos += k;
    *got = k;
    return SB_OK;
}

static void fake_close(void *obj)
{
    ((Fake *)obj)->closed++;
}

static const SbChanOps fake_ops = { fake_write, fake_read, NULL, fake_close };

static void check_int(const char *what, int got, int want)
{
    if (got != want) {
        report("FAIL unit %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

static void unit_channels(void)
{
    SbAlloc a;
    long live = 0;
    Fake f;
    SbChan *ch;
    SbChan *none = NULL;
    SbChan *w = NULL;
    SbChan *r = NULL;
    SbState st;
    size_t got = 99;
    char one[4];
    int rc;

    sb_alloc_default_init(&a, &live);

    /* partial writes: the core loops until every byte is taken */
    memset(&f, 0, sizeof(f));
    f.max_write = 1;
    ch = sb_chan_new(&a, &fake_ops, &f, SB_CH_WRITE);
    rc = sb_chan_write(ch, "hello", 5);
    check_int("partial write status", rc, SB_OK);
    check_int("partial write count", (int)f.writes, 5);
    if (f.len != 5 || memcmp(f.buf, "hello", 5) != 0) {
        report("FAIL unit partial write content\n");
        failures++;
    }
    /* end of the handle: close runs exactly once */
    sb_chan_free(ch);
    check_int("close once", f.closed, 1);

    /* zero progress and an adapter that reports too many bytes */
    memset(&f, 0, sizeof(f));
    f.zero = 1;
    ch = sb_chan_new(&a, &fake_ops, &f, SB_CH_WRITE);
    check_int("zero progress", sb_chan_write(ch, "x", 1), SB_E_IO);
    sb_chan_free(ch);
    memset(&f, 0, sizeof(f));
    f.lie = 10;
    ch = sb_chan_new(&a, &fake_ops, &f, SB_CH_WRITE);
    check_int("over-report", sb_chan_write(ch, "abc", 3), SB_E_IO);
    sb_chan_free(ch);

    /* failure injection: the adapter's own code comes back unchanged */
    memset(&f, 0, sizeof(f));
    f.fail_code = SB_E_OUT_OF_RANGE;
    ch = sb_chan_new(&a, &fake_ops, &f, SB_CH_WRITE);
    check_int("injected failure", sb_chan_write(ch, "x", 1), SB_E_OUT_OF_RANGE);
    sb_chan_free(ch);

    /* direction: a write-only handle cannot be read and vice versa */
    memset(&f, 0, sizeof(f));
    f.input = "ab";
    f.in_len = 2;
    w = sb_chan_new(&a, &fake_ops, &f, SB_CH_WRITE);
    check_int("read from write-only", sb_chan_read(w, one, 1, &got), SB_E_NOT_IMPLEMENTED);
    r = sb_chan_new(&a, &fake_ops, &f, SB_CH_READ);
    check_int("write to read-only", sb_chan_write(r, "x", 1), SB_E_NOT_IMPLEMENTED);

    /* end of input: reads give the bytes, then SB_OK with a zero count */
    check_int("read 1", sb_chan_read(r, one, 1, &got), SB_OK);
    if (got != 1 || one[0] != 'a') {
        report("FAIL unit read first byte\n");
        failures++;
    }
    check_int("read 2", sb_chan_read(r, one, 1, &got), SB_OK);
    if (got != 1 || one[0] != 'b') {
        report("FAIL unit read second byte\n");
        failures++;
    }
    check_int("end of input status", sb_chan_read(r, one, 1, &got), SB_OK);
    check_int("end of input count", (int)got, 0);

    /* a NULL handle is not open; flush without an operation succeeds */
    check_int("write to NULL", sb_chan_write(none, "x", 1), SB_E_NOT_OPEN);
    check_int("read from NULL", sb_chan_read(none, one, 1, &got), SB_E_NOT_OPEN);
    check_int("flush without operation", sb_chan_flush(w), SB_OK);
    sb_chan_free(w);
    sb_chan_free(r);
    sb_chan_free(none);

    /* interpreter: a failing handle's status reaches the caller, and each
     * channel number writes to its own handle */
    {
        Fake out1;
        Fake out3;
        SbChan *c1;
        SbChan *c3;

        memset(&out1, 0, sizeof(out1));
        memset(&out3, 0, sizeof(out3));
        sb_init(&st, &a);
        c1 = sb_chan_new(&a, &fake_ops, &out1, SB_CH_READ | SB_CH_WRITE);
        c3 = sb_chan_new(&a, &fake_ops, &out3, SB_CH_WRITE);
        sb_set_channel(&st, 1, c1);
        sb_set_channel(&st, 3, c3);
        sb_execute(&st, "PRINT \"to one\"", 14);
        sb_execute(&st, "PRINT #3,\"to three\"", 19);
        if (out1.len != 7 || memcmp(out1.buf, "to one\n", 7) != 0) {
            report("FAIL unit routing to #1\n");
            failures++;
        }
        if (out3.len != 9 || memcmp(out3.buf, "to three\n", 9) != 0) {
            report("FAIL unit routing to #3\n");
            failures++;
        }
        /* a PRINT to an installed-but-failing handle reports its status */
        out1.fail_code = SB_E_IO;
        rc = sb_execute(&st, "PRINT \"boom\"", 12);
        check_int("failing handle status", rc, SB_E_IO);
        check_int("failing handle ERNUM", st.err, SB_E_IO);
        /* channel #2 is not installed here: not open */
        check_int("uninstalled channel", sb_execute(&st, "PRINT #2,\"x\"", 12), SB_E_NOT_OPEN);
        sb_destroy(&st);
        sb_chan_free(c1);
        sb_chan_free(c3);
        check_int("interpreter close #1", out1.closed, 1);
        check_int("interpreter close #3", out3.closed, 1);
    }
    if (live != 0) {
        report("FAIL unit channels: %ld blocks leaked\n", live);
        failures++;
    }
}

/* ---- corpus runs ----------------------------------------------------- */

static char *read_file(const char *path, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    char *data;
    size_t n;
    size_t got;

    if (fp == NULL) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    n = (size_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (char *)malloc(n + 1);
    if (data == NULL) {
        fclose(fp);
        return NULL;
    }
    got = fread(data, 1, n, fp);
    fclose(fp);
    data[got] = '\0';
    *len = got;
    return data;
}

/* Run the lines of one source file; the captured output is left in cap.  The
 * channels #0 to #2 read the source (INPUT takes the lines that follow) and
 * write the capture; #3 writes the capture; error reports are discarded. */
static void run_source(const char *src, size_t srclen, Capture *cap,
                       long *leaked)
{
    SbAlloc a;
    SbState st;
    Run run;
    SbChan *chans[SB_NCHAN];
    SbChan *diag;
    long live = 0;
    size_t i;

    run.cap = cap;
    run.src = src;
    run.srclen = srclen;
    run.pos = 0;

    sb_alloc_default_init(&a, &live);
    sb_init(&st, &a);
    for (i = 0; i < SB_NCHAN; i++) {
        chans[i] = (i < 3)
            ? sb_chan_new(&a, &run_io_ops, &run, SB_CH_READ | SB_CH_WRITE)
            : sb_chan_new(&a, &run_out_ops, &run, SB_CH_WRITE);
        sb_set_channel(&st, (int)i, chans[i]);
    }
    diag = sb_chan_new(&a, &discard_ops, NULL, SB_CH_WRITE);
    sb_set_diag(&st, diag);

    while (run.pos < srclen) {
        size_t start = run.pos;
        size_t end = start;
        while (end < srclen && src[end] != '\n') {
            end++;
        }
        /* the next unread byte is after this line's newline; an INPUT in
         * this line moves it further on */
        run.pos = (end < srclen) ? end + 1 : end;
        sb_execute(&st, src + start, end - start);
    }
    sb_destroy(&st);
    for (i = 0; i < SB_NCHAN; i++) {
        sb_chan_free(chans[i]);
    }
    sb_chan_free(diag);
    *leaked = live;
}

static int run_file(const char *path, int known, int *counted_fail)
{
    size_t srclen;
    char *src;
    char *expect_path;
    char *expect;
    size_t explen;
    Capture cap;
    long leaked;
    int ok;
    size_t plen;

    src = read_file(path, &srclen);
    if (src == NULL) {
        report("FAIL %s: cannot read\n", path);
        (*counted_fail)++;
        return 0;
    }

    plen = strlen(path);
    expect_path = (char *)malloc(plen + 2);
    memcpy(expect_path, path, plen);
    strcpy(expect_path + plen, "");
    if (plen >= 4 && strcmp(path + plen - 4, ".bas") == 0) {
        memcpy(expect_path + plen - 4, ".out", 5);
    }

    cap.data = NULL;
    cap.len = 0;
    cap.cap = 0;
    run_source(src, srclen, &cap, &leaked);

    expect = read_file(expect_path, &explen);
    ok = (expect != NULL && explen == cap.len
          && memcmp(expect, cap.data == NULL ? "" : cap.data, explen) == 0);

    if (ok && leaked == 0) {
        report("%s %s\n", known ? "KNOWN-OK" : "PASS", path);
    } else if (leaked != 0) {
        report("FAIL %s: %ld heap blocks not released\n", path, leaked);
        if (!known) {
            (*counted_fail)++;
        }
    } else {
        report("%s %s (expected %lu bytes, got %lu)\n",
               known ? "KNOWN" : "FAIL", path,
               (unsigned long)explen, (unsigned long)cap.len);
        if (!known) {
            (*counted_fail)++;
        }
    }

    free(expect);
    free(expect_path);
    free(src);
    free(cap.data);
    return ok && leaked == 0;
}

struct drv {
    int argc;
    char **argv;
    int known;
    int units;
    int fails;
    int files;
    int passed;
    unsigned int kib;
};

static void run_one(struct drv *d, const char *path)
{
    char name[160];
    size_t n = strlen(path);

    if (n + 5 > sizeof(name)) {
        report("FAIL %s: path too long\n", path);
        d->fails++;
        return;
    }
    strcpy(name, path);
    if (n < 4 || strcmp(name + n - 4, ".bas") != 0) {
        strcat(name, ".bas");
    }
    d->files++;
    if (run_file(name, d->known, &d->fails)) {
        d->passed++;
    }
}

static int run_manifest(struct drv *d, const char *mf)
{
    FILE *fp = fopen(mf, "r");
    char ln[200];

    if (fp == NULL) {
        report("FAIL cannot open manifest %s\n", mf);
        d->fails++;
        return 0;
    }
    while (fgets(ln, (int)sizeof(ln), fp) != NULL) {
        size_t k = strlen(ln);

        while (k > 0 && (ln[k - 1] == '\n' || ln[k - 1] == '\r' || ln[k - 1] == ' ')) {
            ln[--k] = '\0';
        }
        if (k == 0 || ln[0] == '#') {
            continue;
        }
        run_one(d, ln);
    }
    fclose(fp);
    return 1;
}

static int drive(void *p)
{
    struct drv *d = (struct drv *)p;
    int i;

    if (d->units) {
        unit_format();
        unit_errors();
        unit_canonical();
        unit_step_limit();
        unit_channels();
        if (failures > 0) {
            report("%d unit check(s) failed\n", failures);
            d->fails += failures;
        } else {
            report("PASS unit checks (format, errors, canonical form, step limit, channels)\n");
        }
    }
    for (i = 1; i < d->argc; i++) {
        const char *a = d->argv[i];

        if (a[0] == '/') {
            continue;
        }
        if (a[0] == '@') {
            run_manifest(d, a + 1);
        } else {
            run_one(d, a);
        }
    }
    return d->fails;
}

int main(int argc, char **argv)
{
    struct drv d;
    int i;
    char t[160];

    memset(&d, 0, sizeof(d));
    d.argc = argc;
    d.argv = argv;
    d.units = 1;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (strcmp(a, "/K") == 0 || strcmp(a, "/k") == 0) {
            d.known = 1;
        } else if (strcmp(a, "/U") == 0 || strcmp(a, "/u") == 0) {
            d.units = 0;
        } else if (strcmp(a, "/Q") == 0 || strcmp(a, "/q") == 0) {
            quiet_pass = 1;
        } else if (a[0] == '/' && (a[1] == 'O' || a[1] == 'o') && a[2] == ':') {
            resfile = fopen(a + 3, "w");
            if (resfile == NULL) {
                printf("BASTEST: cannot create %s\n", a + 3);
                return 2;
            }
        } else if (a[0] == '/' && (a[1] == 'M' || a[1] == 'm') && a[2] == ':') {
            d.kib = (unsigned int)atol(a + 3);
        } else if (a[0] == '/') {
            printf("usage: BASTEST [/K] [/U] [/Q] [/O:results] @manifest | file.bas ...\n");
            return 2;
        }
    }
    if (qb_io_init() < 0) {
        printf("BASTEST: needs a console\n");
        return 2;
    }
    if (d.kib == 0) {
        d.kib = QB_STACK_DEFAULT_KIB;
    }
    if (qb_stack_call(d.kib, drive, &d) < 0 && qb_stack_size() == 0) {
        printf("BASTEST: out of memory for the interpreter stack\n");
        qb_io_done();
        return 2;
    }
    sprintf(t, "BASTEST %s: %d file(s), %d passed, %d failing, stack used=%u of %u, call depth limit=%d%s\n",
            d.known ? "known" : "corpus", d.files, d.passed, d.fails,
            qb_stack_used(), qb_stack_size(), qb_depth_limit, qb_stack_intact() ? "" : " STACK-OVERFLOW");
    report("%s", t);
    if (resfile != NULL) {
        fclose(resfile);
    }
    qb_stack_free();
    qb_io_done();
    printf("BASTEST DONE\n");
    return d.fails == 0 ? 0 : 1;
}
