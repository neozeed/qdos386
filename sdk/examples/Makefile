# SDK examples.  Build with the historical tree:
#   QDOS386_TOOLS=/path/to/tree make -C sdk/examples
# Each example is built with bin/qdos386-gcc only (no hand-written trap code).
ifndef QDOS386_TOOLS
$(error set QDOS386_TOOLS to the GCC 1.40 / GAS / i386-ld tree)
endif
WRAP = ../bin/qdos386-gcc
# Every example depends on the libraries it links (and the wrapper's startup
# object), so rebuilding libc.a or libqdos.a relinks the examples.
LIBS = ../lib/libc.a ../lib/libgcc-qd.a ../lib/libqdos.a ../lib/crt0_libc.s ../lib/crt0.s ../bin/qdos386-gcc
export QDOS386_TOOLS

all: HELPER.aout FPARSE.aout FMATH.aout FPRINT.aout FPMULT.aout FPCOMB.aout SDKHEL.aout SDKARG.aout SDKSTM.aout SDKSTS.aout LIBHEL.aout CATFIL.aout MEMSTR.aout BIGAPP.aout HEAPEX.aout CTYPE.aout STRTST.aout FTEST.aout STREAM.aout FORMAT.aout CONSOL.aout LIBQDO.aout REPEAT.aout EXITFL.aout

# R11A: C89 library expansion tests.  Output names are 8.3 and at most 10
# characters with the dot, the limit of the WIN1_ channel name (see README).
CTYPE.aout: r11a/ctype/ctype.c $(LIBS)
	$(WRAP) -o $@ r11a/ctype/ctype.c

STRTST.aout: r11a/strings/strings.c $(LIBS)
	$(WRAP) -o $@ r11a/strings/strings.c

FTEST.aout: r11a/filetest/filetest.c $(LIBS)
	$(WRAP) -o $@ r11a/filetest/filetest.c

# R9: the C library proofs (libc.a, see sdk/libc and docs/R9-LIBC-ABI.md).
LIBHEL.aout: libhello/libhello.c $(LIBS)
	$(WRAP) -o $@ libhello/libhello.c

CATFIL.aout: catfil/catfil.c $(LIBS)
	$(WRAP) -o $@ catfil/catfil.c

MEMSTR.aout: memstr/memstr.c $(LIBS)
	$(WRAP) -o $@ memstr/memstr.c

# BIGAPP is larger than 224 KiB (the R8 limit): its table is generated.
bigapp/table.c: bigapp/gen_table.py
	python3 bigapp/gen_table.py > $@

BIGAPP.aout: bigapp/bigapp.c bigapp/table.c $(LIBS)
	$(WRAP) -o $@ bigapp/bigapp.c bigapp/table.c

HEAPEX.aout: heapex/heapex.c $(LIBS)
	$(WRAP) -o $@ heapex/heapex.c

SDKHEL.aout: hello/hello.c $(LIBS)
	$(WRAP) -o $@ hello/hello.c

SDKARG.aout: args/args.c $(LIBS)
	$(WRAP) -o $@ args/args.c

SDKSTM.aout: stream/stream.c $(LIBS)
	$(WRAP) -o $@ stream/stream.c

SDKSTS.aout: stress/stress.c $(LIBS)
	$(WRAP) -o $@ stress/stress.c

# R11B: buffered stdio, the libqdos wrappers, and the repeated-run proof.
# Names are 8.3 and at most 10 characters with the dot (see README).
STREAM.aout: r11e/stream/stream.c r11e/check.h $(LIBS)
	$(WRAP) -o $@ r11e/stream/stream.c

FORMAT.aout: r11b/format/format.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11b/format/format.c

CONSOL.aout: r11b/console/consol.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11b/console/consol.c

LIBQDO.aout: r11b/libqdo/libqdo.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11b/libqdo/libqdo.c

REPEAT.aout: r11e/repeat/repeat.c r11e/check.h $(LIBS)
	$(WRAP) -o $@ r11e/repeat/repeat.c

EXITFL.aout: r11b/exitfl/exitfl.c $(LIBS)
	$(WRAP) -o $@ r11b/exitfl/exitfl.c

# R11M: path layer test (directories, relative paths, legacy names).
PATHT.aout: r11m/pathtest/pathtest.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11m/pathtest/pathtest.c

DIRFUL.aout: r11m/dirfull/dirfull.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11m/dirfull/dirfull.c

# R11M Stage 4: VFAT long file names.
VFATT.aout: r11m/vfattest/vfattest.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11m/vfattest/vfattest.c

# R11M: virtual console tests and demo jobs.
CONT1.aout: r11m/contest/contest.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11m/contest/contest.c

COUNTER.aout: r11m/counter/counter.c $(LIBS)
	$(WRAP) -o $@ r11m/counter/counter.c

TICKER.aout: r11m/counter/ticker.c r11m/counter/counter.c $(LIBS)
	$(WRAP) -o $@ r11m/counter/ticker.c

CONINP.aout: r11m/coninp/coninp.c $(LIBS)
	$(WRAP) -o $@ r11m/coninp/coninp.c

JOBT.aout: r11m/jobtest/jobtest.c r11b/check.h $(LIBS)
	$(WRAP) -o $@ r11m/jobtest/jobtest.c

WRITER.aout: r11m/jobtest/writer.c $(LIBS)
	$(WRAP) -o $@ r11m/jobtest/writer.c

# R11M-FIX1: guest helper that asks the emulator for a clean shutdown (port 0x8900).
SHUTDOWN.aout: r11m/shutdown/shutdown.c $(LIBS)
	$(WRAP) -o $@ r11m/shutdown/shutdown.c

clean:
	rm -f *.aout bigapp/table.c

.PHONY: all clean

# R11C: compiler support (libgcc-qd.a) conformance, run before any FP test.
HELPER.aout: r11c/helpers/helpers.c r11c/check.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11c/helpers/helpers.c

FPARSE.aout: r11c/fparse/fparse.c r11c/check.h r11c/tables/parse.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11c/fparse/fparse.c

FMATH.aout: r11c/fmath/fmath.c r11c/check.h r11c/tables/libm.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11c/fmath/fmath.c

FPRINT.aout: r11c/fprint/fprint.c r11c/check.h r11c/tables/printf.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11c/fprint/fprint.c

# FPMULT: two jobs with live x87 state (the FPJOB protocol, R11C banner).
FPMULT.aout: r11c/fpmult/fpmult.c $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11c/fpmult/fpmult.c

FPCOMB.aout: r11c/fpcomb/fpcomb.c r11c/check.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11c/fpcomb/fpcomb.c

# The compiler support archive, built from the historical gnulib sources.
../lib/libgcc-qd.a:
	$(MAKE) -C ../compiler

# R11D tests: the C89 core, native setjmp/longjmp, scanf, the libm extension.
C89COR.aout: r11d/c89cor/c89cor.c r11d/check.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11d/c89cor/c89cor.c

JMPTST.aout: r11d/jmptst/jmptst.c r11d/check.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11d/jmptst/jmptst.c

SCANTS.aout: r11d/scants/scants.c r11d/check.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11d/scants/scants.c

MATHX.aout: r11d/mathx/mathx.c r11d/tables/mathx.h r11d/check.h $(LIBS)
	../bin/qdos386-gcc -O -o $@ r11d/mathx/mathx.c

# ---- R11E: libqdos API tests and their child programs (wrapper: qdos386-gcc) ----
R11E_APPS = QDOSAPI PS C89EDGE CHILD1 CHILD2 CONWAIT EXITX EXITR
r11e-apps: $(addsuffix .aout,$(R11E_APPS))

QDOSAPI.aout: r11e/qdosapi/qdosapi.c r11e/check.h $(LIBS)
	$(WRAP) -o $@ r11e/qdosapi/qdosapi.c
PS.aout: r11e/jobs/jobs.c $(LIBS)
	$(WRAP) -o $@ r11e/jobs/jobs.c
C89EDGE.aout: r11e/c89edge/c89edge.c r11e/check.h $(LIBS)
	$(WRAP) -o $@ r11e/c89edge/c89edge.c
CHILD1.aout: r11e/children/child1.c $(LIBS)
	$(WRAP) -o $@ r11e/children/child1.c
CHILD2.aout: r11e/children/child2.c $(LIBS)
	$(WRAP) -o $@ r11e/children/child2.c
CONWAIT.aout: r11e/children/conwait.c $(LIBS)
	$(WRAP) -o $@ r11e/children/conwait.c
EXITX.aout: r11e/exits/exitx.c $(LIBS)
	$(WRAP) -o $@ r11e/exits/exitx.c
EXITR.aout: r11e/exits/exitr.c $(LIBS)
	$(WRAP) -o $@ r11e/exits/exitr.c

# R12: console API version 2 self-check and interactive demo.
CONAPI.aout: r12/conapi/conapi.c $(LIBS)
	$(WRAP) -o $@ r12/conapi/conapi.c

CONDEMO.aout: r12/condemo/condemo.c $(LIBS)
	$(WRAP) -o $@ r12/condemo/condemo.c

# R12: the full-screen text editor.
EDIT.aout: r12/edit/edit.c $(LIBS)
	$(WRAP) -o $@ r12/edit/edit.c

# R12: resource probe (heap, channel slots, address spaces).
LEAK.aout: r12/leak/leak.c $(LIBS)
	$(WRAP) -o $@ r12/leak/leak.c

# R12 Stage 4: SuperBASIC/C89 (SB4 core, unchanged but for one #ifndef guard) with the QDOS channel adapter.
# BASIC.aout is the interactive interpreter, BASTEST.aout the in-guest corpus runner (derived from tests/sbtest.c).
SBDIR = ../../apps/superbasic
SBCORE = $(SBDIR)/src/sb_mem.c $(SBDIR)/src/sb_chan.c $(SBDIR)/src/sb_error.c $(SBDIR)/src/sb_format.c $(SBDIR)/src/sb_value.c \
         $(SBDIR)/src/sb_lex.c $(SBDIR)/src/sb_store.c $(SBDIR)/src/sb_expr.c $(SBDIR)/src/sb_check.c $(SBDIR)/src/sb_interp.c
SBQD = $(SBDIR)/qdos/qb_io.c $(SBDIR)/qdos/qb_stack.c
SBDEPS = $(wildcard $(SBDIR)/src/*.h $(SBDIR)/src/*.c $(SBDIR)/qdos/*.h $(SBDIR)/qdos/*.c)
SBFLAGS = -O -I $(SBDIR)/src -I $(SBDIR)/qdos -D SB_CALL_DEPTH_VAR=qb_depth_limit

BASIC.aout: $(SBDIR)/qdos/basic.c $(SBCORE) $(SBQD) $(SBDIR)/qdos/qb_ext.c $(SBDEPS) $(LIBS)
	$(WRAP) $(SBFLAGS) -D QB_R12B -o $@ $(SBDIR)/qdos/basic.c $(SBCORE) $(SBQD) $(SBDIR)/qdos/qb_ext.c

BASTEST.aout: $(SBDIR)/qdos/bastest.c $(SBCORE) $(SBQD) $(SBDEPS) $(LIBS)
	$(WRAP) $(SBFLAGS) -o $@ $(SBDIR)/qdos/bastest.c $(SBCORE) $(SBQD)

# R12A: dynamic-process tests (docs/R12A-RUNTIME-REPORT.md).
MAPTEST.aout: r12a/maptest/maptest.c $(LIBS)
	$(WRAP) -o $@ r12a/maptest/maptest.c
BIGBSS.aout: r12a/bigbss/bigbss.c $(LIBS)
	$(WRAP) -o $@ r12a/bigbss/bigbss.c
HEAPT.aout: r12a/heapt/heapt.c $(LIBS)
	$(WRAP) -o $@ r12a/heapt/heapt.c
STKT.aout: r12a/stkt/stkt.c $(LIBS)
	$(WRAP) -o $@ r12a/stkt/stkt.c
FAULTT.aout: r12a/faultt/faultt.c $(LIBS)
	$(WRAP) -o $@ r12a/faultt/faultt.c
SPAWNT.aout: r12a/spawnt/spawnt.c $(LIBS)
	$(WRAP) -o $@ r12a/spawnt/spawnt.c
CHILDT.aout: r12a/childt/childt.c $(LIBS)
	$(WRAP) -o $@ r12a/childt/childt.c
PINFO.aout: r12a/pinfo/pinfo.c $(LIBS)
	$(WRAP) -o $@ r12a/pinfo/pinfo.c

# R12B: the process environment (docs/R12B-RUNTIME-REPORT.md).
ENVT.aout: r12b/envt/envt.c $(LIBS)
	$(WRAP) -o $@ r12b/envt/envt.c
NEGDYN.aout: r12b/negdyn/negdyn.c $(LIBS)
	$(WRAP) -o $@ r12b/negdyn/negdyn.c

# R12C: program titles (docs/R12C-PROGRAM-TITLE-ABI.md).
TITLET.aout: r12c/titlet/titlet.c $(LIBS)
	$(WRAP) -o $@ r12c/titlet/titlet.c
SYNCT.aout: r12c/synct/synct.c $(LIBS)
	$(WRAP) -o $@ r12c/synct/synct.c

# R13: display surfaces (docs/R13-DISPLAY-ABI.md).
SURFT.aout: r13/surft/surft.c $(LIBS)
	$(WRAP) -o $@ r13/surft/surft.c
COLOUR.aout: r13/colour/colour.c r13/gfxmodel.h $(LIBS)
	$(WRAP) -o $@ r13/colour/colour.c
BOUNCE.aout: r13/bounce/bounce.c r13/gfxmodel.h $(LIBS)
	$(WRAP) -o $@ r13/bounce/bounce.c

# R13A: fonts, rendered consoles, logical surfaces, key events (docs/R13A-GRAPHICS-ABI.md).
GFXA.aout: r13a/gfxa/gfxa.c $(LIBS)
	$(WRAP) -o $@ r13a/gfxa/gfxa.c

# R13B: argument / lookup probe (docs/R13B-EXEC-RESOLUTION-ABI.md); WHICHA/WHICHB are the same source with another tag.
ARGV.aout: r13b/argv/argv.c $(LIBS)
	$(WRAP) -o $@ r13b/argv/argv.c
WHICHA.aout: r13b/argv/argv.c $(LIBS)
	$(WRAP) -DTAG=\"WHICH-A\" -o $@ r13b/argv/argv.c
WHICHB.aout: r13b/argv/argv.c $(LIBS)
	$(WRAP) -DTAG=\"WHICH-B\" -o $@ r13b/argv/argv.c
WHICHC.aout: r13b/argv/argv.c $(LIBS)
	$(WRAP) -DTAG=\"WHICH-CWD\" -o $@ r13b/argv/argv.c
TIMEQ.aout: r13b/timeq/timeq.c $(LIBS)
	$(WRAP) -o $@ r13b/timeq/timeq.c

# R14: serial terminal and the QXF1 transfer test probe (not XMODEM) - docs/R14-SERIAL-ABI.md
TERM.aout: r14/term/term.c $(LIBS)
	$(WRAP) -O -o $@ r14/term/term.c
XFER.aout: r14/xfer/xfer.c $(LIBS)
	$(WRAP) -O -o $@ r14/xfer/xfer.c
