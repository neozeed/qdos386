# OEM.H -> oem.inc
# Effective values of the OEM switches for this build. MASM redefines a symbol on
# each `=`, and conditionals use the value current at that point. Every switch in
# OEM.H is set before any code that tests it, so the LAST assignment in OEM.H is the
# one that counts. The values below are those last assignments.
# All values are decimal (OEM.H is .RADIX 10 until the "Useful macros" section;
# values in that later section, if any, are noted).
#
# Names that collide with GAS intel-syntax keywords are renamed with a trailing "_":
#   SHORT -> SHORT_, LENGTH -> LENGTH_, STRING -> STRING_.
	.set OEMRAM, 0
	.set NMLINE, 25		# Max screen display text lines
	.set NBANKS, 1		# Number of graphic planes
	.set PALSIZ, 0
	.set DBLCHR, 0		# Editor support for KANJI chars
	.set KANABS, 0
	.set KANJFN, 0
	.set JISKTN, 0
	.set SUPRSH, 1		# super shift key feature
	.set NMKEYF, 10		# number of function keys
	.set NMKEYT, 14		# number of function key traps
	.set FKEYON, 1
	.set SCROLT, 1
	.set PLAYSW, 1
	.set BEEPSW, 1
	.set NMLPT, 3		# number of line printers
	.set CONSSW, 1
	.set NMCOMT, 0		# number of communications ports
	.set NMPENT, 0
	.set NMSTRT, 0		# number of joysticks
	.set GWCASS, 0
	.set IBMLIK, 1
	.set IBMTOK, 1
	.set DYNCOM, 1
	.set NUMTRP, 14		# NMKEYT+NMCOMT+NMPENT+NMSTRT = 14+0+0+0 (COM, PEN, STRIG, STICK are out of scope)
	.set GW, 1
	.set ESCCTL, 0
	.set GWDBUG, 0
	.set SIXDIG, 0
	.set CASSW, 0
	.set LABKEY, 1
	.set HGHBIT, 1
	.set QUEUE, 1
	.set GIO86, 1
	.set SCRNIO, 1
	.set SCRN86, 1
	.set I8086, 1
	.set MULSEG, 1
	.set SHORT_, 0		# was SHORT
	.set TRUROM, 1
	.set SBC86, 0
	.set SHORTJ, 1
	.set LNGVAR, 1
	.set SHTERR, 0
	.set SHTINS, 0
	.set SHTLHM, 0
	.set SHTCHN, 0
	.set SHTPRO, 0
	.set SHTLIN, 0
	.set SHTREN, 0
	.set INKEYF, 1
	.set NOIOS, 0
	.set NOPEKS, 0
	.set NOUSR, 0
	.set NOCTLA, 0
	.set NORUBT, 0
	.set NOROMP, 0
	.set NOATDL, 0
	.set NODOT, 0
	.set NOEDIT, 0
	.set CRONLY, 0
	.set NONULL, 0
	.set NOVARP, 0
	.set FIVEO, 1		# generate version with release 5.0 features
	.set FIVMEM, 1
	.set ANSI, 1
	.set FIVDSK, 1
	.set FIVLPT, 1
	.set HEXOCT, 1
	.set BINCSW, 0
	.set BLODSW, 1
	.set MEMIMG, 1
	.set GENBIN, 1
	.set TRUANS, 0
	.set COMMNT, 0
	.set INTEL, 0
	.set INTFSW, 0
	.set INTFHW, 0
	.set RMX, 0
	.set EDIT80, 0
	.set RSHACK, 0
	.set INTLEC, 0
	.set LENGTH_, 2		# was LENGTH (1 = 8K, 2 = 12K)
	.set BASDEB, 0
	.set RSTLES, 1
	.set REALIO, 1
	.set GENIO, 1
	.set CHSEAT, 1
	.set GENDSK, 0
	.set LONGI, 0
	.set INSTSW, 0
	.set SWAPSW, 1
	.set MDS, 0
	.set SBC, 0
	.set PURE, 1
	.set KILLER, 0
	.set INTROM, 0
	.set LPTSW, 1
	.set NCRLPT, 1
	.set NCRCAS, 0
	.set ISC, 0
	.set LPT3, 0
	.set TSLPT, 0
	.set DOS, 0
	.set DISK, 1
	.set TIMSHR, 0
	.set TSDISK, 0
	.set SPCDSK, 0
	.set SPCNDS, 0
	.set FAST, 1
	.set KEYSTA, 0
	.set ISIS, 0
	.set MDSISS, 0
	.set CPM, 1
	.set CPM2, 1
	.set CPM86, 0
	.set SCP, 1
	.set CPMCON, 0
	.set CPMSER, 0
	.set CPMADD, 0
	.set CPMA75, 0
	.set CPMRSH, 0
	.set CPM110, 0
	.set CPMSBC, 0
	.set CPMPER, 0
	.set CPMLIF, 0
	.set CPMMDS, 0
	.set Z80, 0
	.set DRVPOS, 0		# = CPMADD
	.set TRNCHR, 0		# = CPMADD
	.set INLADD, 0		# = CPMADD
	.set LISTEN, 1
	.set LEN2, 1
	.set LEN3, 1		# = LEN2 OR CASSW
	.set I8251, 0
	.set CONTRO, 1
	.set STRING_, 1		# was STRING
	.set CMPUTN, 0
	.set DUPONT, 0
	.set EXIDY, 0
	.set GESBC, 0
	.set HEATH, 0
	.set LOKHED, 0
	.set MOSTEK, 0
	.set TEK, 0
	.set TRSER2, 0
	.set NASCOM, 0
	.set NATROM, 0
	.set NCRBAS, 0
	.set NCRDEB, 0
	.set NCRELS, 0
	.set ONTEL, 0
	.set PCS, 0
	.set SGSATS, 0
	.set ALTAIR, 0
	.set DMC, 0
	.set PANBAS, 0
	.set XTNSYS, 0
	.set NECBAS, 0
	.set CONTO, 15		# CHARACTER TO SUPPRESS OUTPUT (usually CTRL-O)
	.set DBLTRN, 0
	.set CLMWID, 14		# comma column width
	.set DATPSC, 128	# data bytes in disk sector
	.set LINLN, 80		# terminal line length
	.set LPTLEN, 132
	.set BUFLEN, 255	# long lines
	.set NAMLEN, 40		# maximum name length
	.set NUMLEV, 110	# stack levels reserved by GETSTK
	.set STRSIZ, 5		# string descriptor: length(1) + address(4) (was 3)
	.set NUMTMP, 10		# number of string temporaries
