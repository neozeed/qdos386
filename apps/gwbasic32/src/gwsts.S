# GWSTS.ASM -> gwsts.S
# Common statement support: text screen (CLS, LOCATE, WIDTH, GETLIN, SCREEN function),
# parsing helpers (SCNINT, EOSCHK), VARPTR$, sound (SOUND, BEEP, SNDINI, SNDRST) and the
# event-trap code (ON/OFF/STOP for keys).
#
# Out of scope in this port (see NOTES.md): LCOPY (printer), COLOR and the SCREEN statement
# (display attributes and video modes), PLAY (uses MACLNG, out of scope), and graphics PUT/GET.
# Those entries report "Illegal function call".
#
# Widths: pointers (text pointer, variable pointers) are 4 bytes on the stack; values are 2 bytes.
# Device routines (CLRSCN, SCNLOC, CSRDSP, SWIDTH, SCRATR, SCRINP, DONOTE, POLKEY, ...) are the
# Stage 4 console layer and keep the original names.

	.intel_syntax noprefix
	.include "oem.inc"
	.include "msdosu.inc"
	.include "gwtok.inc"
	.include "tokens.inc"

	.section .text
	.globl CLS, LOCATE, GWWID, LCOPYS, COLOR, GETLIN, SCRENF, SCREEN
	.globl PUT, GET, SCNINT, EOSCHK, VARPT2, PLAYS, SNDINI, SNDRST, BEEPS, BEEP, SOUNDS

# -------------------------------------------------------------------------------------
# CLS: clear the screen. The optional parameter is parsed and checked.
# -------------------------------------------------------------------------------------
CLS:
	call SCNINT			# test for optional parameter
	cmc
	pushfd
	push ax
	call EOSCHK			# test for end of statement
	pop ax
	popfd
	push ebx
	call CLRSCN
	pop ebx
	ret

# -------------------------------------------------------------------------------------
# LOCATE [Y] [, [X] [, [CURSOR] [, [START] [, [STOP] ]]]]
# Stack: Y (2), X (2), then three (parameter, flag) values (2 each), then the text pointer.
# -------------------------------------------------------------------------------------
LOCATE:
	call SCNINT			# get optional Y parameter
	jnb YLCPRM			# parameter present
	call GTLINE			# get the current screen position
YLCPRM:
	mov dl, al
	or al, al			# test for LOCATE 0
	jz GOFCER
	sub dl, BYTE PTR KEYSW		# increment if PF-keys are displayed
	cmp dl, BYTE PTR LINCNT		# check for parameter range
	ja GOFCER
	push ax				# save new Y location
	call SCNINT			# get optional X parameter
	jnb XLCPRM			# parameter present
	call SCNPOS			# get the current screen position
	mov al, dh			# default to current cursor position
XLCPRM:
	mov dl, al
	dec dl				# disallow LOCATE ,0
	cmp dl, BYTE PTR LINLEN		# check for parameter range
	jae GOFCER
	push ax				# save new X
	call SCNINT			# cursor on/off - 0=off else on
	mov ah, 255			# ensure non-zero
	jnb LOCPR1			# parameter 1 found
	xor ah, ah			# flag as a default
LOCPR1:
	push ax				# push first parameter and flag
	call SCNINT			# get next parameter
	mov ah, 255			# ensure non-zero flag
	jnb LOCPR2			# parameter 2 found
	xor ah, ah			# flag as a default
LOCPR2:
	push ax				# push second parameter and flag
	call SCNINT			# get next parameter
	mov ah, 255			# ensure non-zero flag
	jnb LOCPR3			# parameter 3 found
	xor ah, ah			# flag as a default
LOCPR3:
	push ax				# push third parameter
	call EOSCHK			# check for end of statement
	mov edx, ebx			# save text pointer
	pop cx
	pop bx
	pop ax				# recover three parameters
	push edx			# save text pointer (4 bytes)
	call CSRATR			# set cursor attribute (OEM routine)
	jb GFCERR			# declare error from CSRATR
	pop ebx				# text pointer
SETLOC:
	mov eax, ebx			# save text pointer (4 bytes)
	pop cx				# X
	pop bx				# Y
	push eax			# save text pointer (4 bytes)
	mov bh, cl			# BX = (X << 8) | Y
	mov ax, bx			# AH = column, AL = line
	call SCNLOC			# position cursor at line [AL], col [AH]
	mov dx, ax			# load cursor position
	mov al, 3			# signal for user cursor
	call CSRDSP			# set cursor
	pop ebx				# restore text pointer
	ret

GOFCER:
	jmp FCERR

# -------------------------------------------------------------------------------------
# WIDTH [X] [, [Y]]: parse, then set the screen size if it changed.
# Exit: AL = X parameter, BX = text pointer.
# -------------------------------------------------------------------------------------
GWWID:
	call SCNINT			# get X dimension
	jnb XPRAM			# X param found
	mov al, BYTE PTR LINLEN		# use current as default
XPRAM:
	push ax				# save for RET to WIDTH
	push ax				# save for GWWID use
	call SCNINT			# get Y dimension
	jnb YPRAM			# Y param found
	mov al, BYTE PTR LINCNT		# use current as default
YPRAM:
	push ax
	call EOSCHK			# must be at end of statement
	pop ax
	cmp al, BYTE PTR LINCNT		# set CCs for Y dimension change
	lahf
	mov cx, ax
	pop ax
	cmp al, BYTE PTR LINLEN		# set CCs for X dimension change
	lahf
	and ah, ch			# set CCs for X OR Y change
	sahf
	jz GWWIDX			# no change - done
	push ebx			# save text pointer
	push ax				# save width
	push cx				# save height
	call SWIDTH			# machine dependent set logic
	pop cx
	pop ax
	jb GFCERR			# error detected within SWIDTH
	pop ebx				# restore text pointer
GWWIDX:
	pop ax				# return X dimension for WIDTH
	ret

GFCERR:
	jmp FCERR

# -------------------------------------------------------------------------------------
# LCOPY (printer copy of the screen): out of scope, reports an error.
# -------------------------------------------------------------------------------------
LCOPYS:
	jmp FCERR

# -------------------------------------------------------------------------------------
# COLOR and the SCREEN statement (display attributes and video modes): out of scope.
# -------------------------------------------------------------------------------------
COLOR:
	jmp FCERR
SCREEN:
	jmp FCERR

# -------------------------------------------------------------------------------------
# GETLIN: the current cursor line number as a function value. BX is preserved.
# -------------------------------------------------------------------------------------
GETLIN:
	call GTLINE
	push ebx
	xor ebx, ebx
	mov bl, al
	call MAKINT
	pop ebx
	call CHRGTR
	ret

# GTLINE: line number of the character position the next character would be written to.
# Exit: AL = line number. Uses AH.
GTLINE:
	mov al, BYTE PTR CSRY		# BX = cursor line number
	mov ah, BYTE PTR CSRX
	cmp ah, BYTE PTR LINLEN
	jbe GETLN0			# will not wrap before the next char is output
	mov ah, BYTE PTR LINCNT		# AL = last valid line number
	dec ah				# scroll always occurs on line [LINCNT]-1
	cmp al, ah
	jae GETLN0			# wrap will cause a scroll
	inc al				# else wrap will cause a line number increment
GETLN0:
	ret

# -------------------------------------------------------------------------------------
# SCREEN(row, col): the character at a text position.
# -------------------------------------------------------------------------------------
SCRENF:
	call CHRGTR			# eat the SCREEN token (dispatched to by EVAL)
	call SYNCHR
	.byte '('			# check for "("
	call SCNINT			# get Y parameter
	jb FCERGO			# parameter not present
	dec al
	cmp al, BYTE PTR LINCNT		# check range [1,LINCNT]
	jae FCERGO			# out of range
	inc al
	push ax				# Y param
	call SCNINT			# check for X param
	jb FCERGO			# error - no X param
	dec al
	cmp al, BYTE PTR LINLEN		# check range [1,LINLEN]
	jae FCERGO			# out of range
	inc al
	push ax				# X param
	cmp ah, ')'			# check terminator
	je SCRCHR			# end of params - go get char
	call SCNINT
	jb FCERGO			# error - no CPAREN and no Z param
	cmp ah, ')'			# must now have CPAREN
	jne FCERGO
	cmp al, 0			# is Z zero?
	je SCRCHR			# yes - go get screen character
	pop ax				# retrieve X param
	pop cx				# retrieve Y param
	push ebx			# save text pointer
	movzx ebx, cx			# call SCRATR with AX=X, BX=Y
	call SCRATR			# get screen attributes
	jmp SCRENX
FCERGO:
	jmp FCERR
SCRCHR:
	pop ax				# retrieve col number
	pop cx				# retrieve row number
	push ebx			# save the text pointer
	push dx
	mov dl, cl
	mov dh, al
	stc				# indicate call is from SCREEN function
	call SCRINP			# [AX]=read char at (DH,DL)
	mov bx, ax			# return result in BX
	pop dx
SCRENX:
	call MAKINT			# set FAC
	pop ebx				# retrieve text pointer
	dec ebx
	call CHRGTR
	ret

# -------------------------------------------------------------------------------------
# PUT / GET: distinguish graphics (a "(" or "@" follows) from disk (DPUTG). The graphics
# form is out of scope and reports an error.
# -------------------------------------------------------------------------------------
PUT:
	mov ecx, 1			# set PUT flag
	jmp PARSE
GET:
	xor ecx, ecx			# set GET flag
PARSE:
	push cx				# save indication of PUT or GET (2 bytes)
	push ebx			# save text pointer (4 bytes)
	cmp al, '('			# test for "("
	je GRPVER			# branch if graphics version
	cmp al, '@'			# test for relative GET/PUT
	jne DSKVER			# disk code may have no "(" or "@"
GRPVER:
	pop ebx				# restore text pointer
	pop ax				# get PUT/GET flag
	jmp FCERR			# graphics PUT/GET is out of scope
DSKVER:
	pop ebx				# restore text pointer
	pop ax				# get PUT/GET flag
	jmp DPUTG

# -------------------------------------------------------------------------------------
# SCNINT: test for an optional integer parameter. A comma or end of line means no parameter.
# Exit: AL = value; carry set = no parameter; AH = terminator.
# -------------------------------------------------------------------------------------
SCNINT:
	dec ebx
	call CHRGTR
	jz NOMORE			# EOL - param null
	cmp al, ','
	jz OMITD			# comma found - param null
	call GETBYT			# evaluate parameter
	push ax				# save parameter
	dec ebx				# prepare to test expression terminator
	call CHRGTR
	jz TRMOK			# EOL caused termination - OK
	cmp al, ','
	jz TRMCOM			# comma caused termination - OK
	cmp al, ')'			# CPAREN caused termination - OK
	jz TRMCOM
	pop ax				# retrieve param
	jmp FCERR			# all other terminators not OK
TRMCOM:
	inc ebx				# move over comma
TRMOK:
	mov cl, al			# save terminator
	pop ax				# retrieve parameter value
	mov ah, cl			# return with AH = terminator
	clc				# flag param found
	ret
OMITD:
	inc ebx
NOMORE:
	mov ah, al			# save terminator
	mov al, 0			# set param value to 0
	stc				# flag param not found
	ret

# -------------------------------------------------------------------------------------
# EOSCHK: detect garbage beyond the end of the statement. Exit: AL = 0, or FCERR/SNERR.
# -------------------------------------------------------------------------------------
EOSCHK:
	dec ebx				# back up text pointer
EOSCH1:
	call CHRGTR			# get next character (skipping blanks)
	jz EOSCKX			# end of statement
	jmp SNERR			# not EOS - error
EOSCKX:
	ret

# -------------------------------------------------------------------------------------
# VARPTR$(x): called after VARPTR sees "$". Returns a 5-byte string: byte 0 = type of x,
# bytes 1-4 = the 4-byte (flat) address of x. The original returned 3 bytes with 16-bit
# addresses, which cannot hold a flat 32-bit address.
# -------------------------------------------------------------------------------------
VARPT2:
	call CHRGTR			# get byte after "$"
	call SYNCHR
	.byte '('			# eat left paren
	call PTRGTN			# get address of variable
	call SYNCHR
	.byte ')'			# eat right paren
	or edx, edx			# make sure not undefined var
	jnz VARRT2
	jmp FCERR			# all over if undef
VARRT2:
	push ebx			# save text pntr
	push edx			# save var addr (4 bytes)
	mov al, BYTE PTR VALTYP
	push ax				# save type (2 bytes)
	mov al, 5
	call STRINI			# get a 5-byte string
	mov ebx, DWORD PTR DSCPTR	# descriptor in [BX]
	pop WORD PTR [ebx]		# store type in byte 0 (the word write also touches byte 1)
	pop DWORD PTR [ebx+1]		# store address in bytes 1-4
	jmp PUTNEW			# desc in FAC and return ([BX] on stack)

# -------------------------------------------------------------------------------------
# PLAY: out of scope (MACLNG music macro engine). Reports an error.
# -------------------------------------------------------------------------------------
PLAYS:
	jmp FCERR

# -------------------------------------------------------------------------------------
# SNDINI: initial music settings. Called at CLEARC and during initialization.
# SNDRST: turn off sound and reset the background queue (INIT and CTL-C).
# -------------------------------------------------------------------------------------
SNDINI:
	mov BYTE PTR BEATS, 120
	mov BYTE PTR MSCALE, 3
	mov BYTE PTR MMODE, 0
	mov BYTE PTR NOTELN, 4
	mov BYTE PTR NOTE1L, 4
	mov BYTE PTR OCTAVE, 4
	call SNDRST			# turn off sound
	ret

SNDRST:
	push eax
	push ebx
	push ecx
	push edx
	pushfd
	mov al, 255
	call DONOTE			# disable background music, init music queue
	popfd
	pop edx
	pop ecx
	pop ebx
	pop eax
	ret

# -------------------------------------------------------------------------------------
# BEEP: 800 Hz for 1/4 second. SOUND x,y: frequency x (at least 37 Hz), duration y (ticks).
# DONOTE is the Stage 4 sound layer.
# -------------------------------------------------------------------------------------
BEEP:
BEEPS:
	mov cx, 800			# 800 Hz
	mov dx, 100			# .. for 1/4 second
	xor al, al			# [AL]=music mode (0=foreground)
	jmp JDNOTE

DOSND:
	mov al, BYTE PTR MMODE		# [AL]=music mode (0=foreground, 1=background)
JDNOTE:
	call DONOTE			# start new sound
	jnb DNOTOK			# no errors detected by DONOTE
	jmp FCERR			# function call error detected
DNOTOK:
	jmp POLKEY			# allow CTL-C to interrupt and return

SOUNDS:
	call GETIN2			# get frequency
	cmp dx, 37
	jb SNDFCE			# must be at least 37 Hz
	push dx				# save frequency
	call SYNCHR
	.byte ','
	call FRMEVL			# get duration
	call EOSCHK			# syntax error if not end-of-statement
	push ebx			# text pointer
	call FRCSNG			# make single precision
	mov ebx, offset _FAC		# point at exponent
	cmp BYTE PTR [ebx], 0		# will turn sound off if 0
	jnz SOUNL4			# not 0: start new sound
	pop ebx				# text pointer
	pop dx				# frequency (not used)
	jmp SNDRST			# turn off sound, initialize the queue

SOUNL4:
	cmp BYTE PTR [ebx], 0221	# duration > 65535?
	jnb SNDFCE			# too big for *32
	push ebx			# save FAC address
	push WORD PTR _FACM1		# push FAC on the stack
	push WORD PTR _FACLO
	mov ebx, 22			# mult by 22
	call MAKINT
	call FRCSNG			# get s.p. 22
	pop dx				# low mantissa bits
	pop bx				# exp sign and high mantissa bits
	call _FMULS			# multiply
	pop ebx				# FAC address
	cmp BYTE PTR [ebx], 0221	# overflow?
	jb SOUNL5			# not
	mov WORD PTR [ebx-1], 0110177	# else force to 65535
	mov WORD PTR [ebx-3], 0177400
SOUNL5:
	call FRQINT			# convert back to integer
	mov dx, bx			# in [DX]
	pop ebx				# text pointer
	pop cx				# [CX]=frequency, [DX]=duration
	jmp DOSND			# play the note
SNDFCE:
	jmp FCERR

# -------------------------------------------------------------------------------------
# Event trapping (ON KEY, ON COM/PEN/STRIG, ...). Event ids: KEY events only, since COM,
# PEN and STRIG have 0 devices in this port (oem.inc). Trap entries are 5 bytes: a flag byte
# (T_ON, T_STOP, T_REQ) and a 4-byte GOSUB line pointer.
# -------------------------------------------------------------------------------------
	.set T_ON, 1			# trap on (BIMISC.ASM)
	.set T_STOP, 2			# trap stop
	.set T_REQ, 4			# trap request
	.set STKEYF, 1			# start number of string function keys
	.set PENOFF, 0			# offset for PEN event ids (no pens: unused)
	.set KEYOFF, PENOFF+NMPENT	# offset for KEY event ids
	.set COMOFF, KEYOFF+NMKEYT	# offset for COM event ids
	.set STROFF, COMOFF+NMCOMT	# offset for STRIG event ids
	.set CR, 13
	.set LF, 10

	.globl ONGOTP, SETGSB, CHKINT, EVTRP, EVCLR, TSTCEV, COMS, COMTRP, KEYTRP, KEYS
	.globl KEYDSP, TKEYOF, SKEYON, PENS, PENF, STRIGS, STICKF, DATES, DATEF, TIMES, TIMEF

# SET1EV: set one event. Entry: [DX] = address of event flag; [AL] = _ON, _OFF or _STOP.
SET1EV:
	call EVSET			# set event flags to ON, OFF or STOP
	ret

# SEVSTT: common parse for ON/OFF/STOP of an event id. Entry: CL = number of legal events
# for the device, CH = event-id offset for the device.
SEVSTT:
	push cx				# save maximum and offset for class
	call SYNCHR
	.byte '('
	call GETBYT			# [AL]=class-relative event id
	push ax				# save it
	call SYNCHR
	.byte ')'
	push ax				# save [AL] = $ON, $OFF or $STOP
	call EOSCH1			# skip past $ON, $OFF or $STOP; error if not end of statement
	pop dx				# restore $ON, $OFF or $STOP
	pop ax				# [AL]=class-relative event id
	pop cx				# restore maximum, offset for class
	cmp ch, STROFF			# special check for STRIG(x)
	jnz SEVST1
	ror al, 1			# divide by 2 so get 0,1,2 or 3
	jmp SEVST2
SEVST1:
	or al, al
	jz SEVALL			# branch if parm was 0
	dec al				# make parm 0-relative
SEVST2:
	push dx				# save $ON, $OFF or $STOP
	cmp al, cl			# compare with maximum
	jge FCERR1			# branch if not in range
	add al, ch			# add in offset to get event id
	call EVADR			# [DX]=adr of event flag
	pop ax				# [AL]=ON, OFF or STOP
	jmp SET1EV

FCERR1:
	jmp FCERR
SNERR1:
	jmp SNERR

# SEVALL: set status for all events of a class. Entry: [DL] = $ON/$OFF/$STOP token,
# [CH] = class event-id offset, [CL] = number of events in class.
SEVALL:
	push dx				# save $ON, $OFF or $STOP
	mov al, ch			# [AL]=class offset
	movzx ecx, cl			# [CX]=number of entries in class (CX = CL, as XOR CH,CH gave)
	call EVADR			# [DX] points to first event flag
	pop ax				# [AL]=ON, OFF or STOP
SEVAL1:
	call EVSET			# set event for key 9-[B]
	add edx, 5			# [DX] points to next entry (5-byte entries)
	loopne SEVAL1
	ret

# EVSET changes the status of one event. Entry: [AL] = $ON/$OFF/$STOP; [DX] = event flag.
# All registers preserved.
EVSET:
	push edx			# save caller's registers
	push ecx
	push ebx
	push eax
	mov ebx, edx			# [BX] = address of event flag
	cmp al, _ON
	jz EVON
	cmp al, _OFF
	jz EVOFF
	cmp al, _STOP
	jnz SNERR1
EVSTP:
	call STPTRP			# stop event pointed to by [BX]
	jmp EVONX
EVON:
	call ONTRP			# enable event pointed to by [BX]
	jmp EVONX
EVOFF:
	call OFFTRP			# disable event pointed to by [BX]
EVONX:
	pop eax				# restore all caller's registers
	pop ebx
	pop ecx
	pop edx
	ret

# EVADR: event id -> event flag address. Entry: [AL] = event id. Exit: [DX] = address of the
# flag, [AX] used, other registers preserved. Each entry is 5 bytes: 5 * event id + TRPTBL.
EVADR:
	movzx eax, al			# [AX] = event id
	mov edx, eax
	shl eax, 2
	add eax, edx			# [AX] = 5 * event id
	add eax, offset TRPTBL
	mov edx, eax
	ret

# -------------------------------------------------------------------------------------
# ONGOTP: called from the ON GOTO code. Syntax checks for event traps. Carry set means the
# ON statement is not a trap GOSUB (ON n GOSUB/GOTO follows).
# Event forms: ON KEY(x) GOSUB, ON COM(x) (no COM ports: error), ON PEN (no pen: error),
# ON STRIG(x) (no joysticks: error).
# -------------------------------------------------------------------------------------
ONGOTP:
	push ebx			# text pointer in case not us
	xor ch, ch			# assume one byte token
	cmp al, 0375			# check for 2-byte token
	jb NT2BTK			# branch if not FF, FE or FD
	mov ch, al			# [CH] = 1st byte
	call CHRGTR			# [AL] = 2nd byte of 2-byte token
	mov ah, ch			# [AH] = 1st byte
NT2BTK:
	cmp ax, _KEY2B
	mov ecx, 256*KEYOFF+NMKEYT	# (maximum for KEY(x))
	jz ONFUN			# brif ON KEY...
	cmp ax, _COM2B
	mov ecx, 256*COMOFF+NMCOMT
	jz ONFUN			# brif ON COM...
	cmp ax, _PEN2B
	jnz NTONPN			# brif not ON PEN...
	jmp FCERR			# light pen is not supported
NTONPN:
	cmp ax, _STR2B
	mov ecx, 256*STROFF+NMSTRT
	jz ONFUN
	stc				# set carry - indicates not event
	pop ebx				# restore entry text pointer
	jmp ONGOTX

FCERR2:
	jmp FCERR

ONFUN:
	call GETSUB			# get event no. in (x)
	dec al				# want base 0 (if not STRIG(x))
	cmp ch, STROFF			# special check for STRIG(x)
	jnz ONFUN1			# brif not
	inc al				# restore STRIG #
	ror al, 1			# divide by 2 so get 0,1,2 or 3
ONFUN1:
	cmp al, cl			# value [AL] > MAX [CL]?
	jnb FCERR2			# if so, illegal function call
	mov cl, al			# save event index in [CL]
ONSTM:
	add ch, cl			# [CH]=event-id
	mov cl, 1			# [CL]=maximum number of line numbers in gosub
	pop edx				# discard saved text pointer
	dec ebx				# backup for CHRGET
	or al, al			# clear carry - indicates it was an event
ONGOTX:
	mov al, BYTE PTR [ebx]		# restore token
	ret

GETSUB:
	push cx
	call CHRGTR			# skip current token
GETSU2:
	call SYNCHR
	.byte '('
	call GETBYT			# get event-class index
	push ax				# save it
	call SYNCHR
	.byte ')'
	pop ax
	pop cx
	ret

# SETGSB: set a GOSUB entry. Entry: [AL] = trap id, [DX] = line pointer (4 bytes).
SETGSB:
	push esi
	push edx			# save gosub line pointer
	call EVADR			# [DX] points to event flag
	mov esi, edx			# [SI] points to event flags
	pop edx				# restore GOSUB line pointer
	inc esi				# [SI] points to gosub line pointer entry
	mov DWORD PTR [esi], edx	# put trap address in table for this entry
	pop esi
	ret

# EVTRP: signal that an event occurred. Entry: [AL] = event id. NZ if not trapped (not ON).
EVTRP:
	push edx
	push ecx
	push ebx
	movzx ebx, WORD PTR CURLIN
	inc bx
	jz NOT_ON			# branch if in direct mode (no event trapping)
	call EVADR			# [DX] points to event flag
	mov ebx, edx			# [BX] points to event flag
	mov al, BYTE PTR [ebx]		# [AL] = event flag
	and al, T_ON			# NZ if trapping enabled for key
	jz NOT_ON			# branch if event is not on
	pushfd
	call REQTRP			# trap enabled: issue request
	popfd
NOT_ON:
	pop ebx
	pop ecx
	pop edx
	ret

# EVCLR: reset an event. Entry: [AL] = event id.
EVCLR:
	push edx
	push ecx
	push ebx
	call EVADR			# [DX] points to event flag
	mov ebx, edx			# [BX] points to event flag
	call FRETRP			# reset trap request
	jmp NOT_ON

# TSTCEV: test and clear an event. Entry: [AL] = 0-relative event index.
# Exit: [BX] = -1 if the event occurred, 0 if not. Bit T_REQ of the flag is cleared.
TSTCEV:
	call EVADR			# [DX] points to event flag
	mov ebx, edx			# [BX] points to event flag
	mov al, BYTE PTR [ebx]		# [AL] = current event state
	push ax
	call FRETRP			# reset trap request (clear T_REQ)
	pop ax
	xor ebx, ebx			# prepare to return a negative result
	and al, T_REQ
	je TSTEVX
	dec ebx				# [BX] = -1 (event has occurred)
TSTEVX:
	ret

# COM statement: no COM ports in this port, so the count is 0 and the parse reports an error.
COMS:
	mov ecx, 256*COMOFF+NMCOMT
	jmp SEVSTT

# COMTRP: signal a COM event. Entry: [AL] = 0-relative COM channel. Not used (no COM ports).
COMTRP:
	push ax
	add al, COMOFF
	call EVTRP			# signal the occurrence of a COM event
	pop ax
	ret

# KEYTRP: called when CHGET receives a soft key. Entry: [AL] = soft key id (0..NMKEYT-1).
# Exit: NZ if the key was trapped (not to be expanded). All other registers preserved.
KEYTRP:
	push ax
	add al, KEYOFF			# [AL] = 0-relative event id
	call EVTRP			# signal the occurrence of a KEY event
	pop ax
	ret

# -------------------------------------------------------------------------------------
# KEY statement: KEY(n) status, KEY ON/OFF/LIST, KEY n, "text".
# Soft key table STRTAB: 16 bytes per key (15 characters and a 0 terminator).
# -------------------------------------------------------------------------------------
KEYSTT:
	mov ecx, 256*KEYOFF+NMKEYT
	jmp SEVSTT			# branch to common code

KEYS:
	cmp al, '('
	jz KEYSTT			# set key status
	cmp al, _ON
	jz KEYON			# brif enable soft key display line
	cmp al, _OFF
	jz KEYOF			# brif disable soft key display line
	cmp al, _LIST
	jz KEYLSI			# brif LIST soft keys
	call GETBYT			# else key definition: get key number
	or al, al
	jz KEYFCE
	sub al, STKEYF			# get zero relative key number
	cmp al, NMKEYF
	jnb KEYFCE			# must be STKEYF - NMKEYF+STKEYF, else error
	mov dx, 16
	mul dl				# (16 * key number)
	movzx edx, ax
	add edx, offset STRTAB		# index into soft key table
	push edx			# save address (4 bytes)
	call SYNCHR
	.byte ','
	call FRMEVL
	push ebx			# save text pointer
	call FRESTR			# get string descriptor
	mov cl, BYTE PTR [ebx]
	cmp cl, 15
	jb KEY1
	mov cl, 15			# string may be 0 to 15 chars
KEY1:
	inc ebx
	mov esi, DWORD PTR [ebx]	# get addr of string
	pop ebx				# text pointer
	pop edi				# STRTAB addr (4 bytes)
	push ebx
	movzx ecx, cl			# [CX] = count (CH = 0)
	cld
	rep movsb			# move new soft key to soft key table
	mov BYTE PTR [edi], 0		# terminate entry with 0
	mov al, BYTE PTR KEYSW		# are the key definitions being displayed?
	or al, al
	jz NODSPK			# no, don't call DSPKEY since it would erase the bottom line
	call KEYDSP			# yes, update the display
NODSPK:
	pop ebx				# text pointer
	ret
KEYFCE:
	jmp FCERR
KEYLSI:
	jmp KEYLST
KEYON:
	call SKEYON
	jmp KEYOXX			# non-zero = ON
KEYOF:
	mov ah, 1			# prepare to inc scroll limit
	mov al, 0			# zero = OFF
	call KEYOX
KEYOXX:
	call CHRGTR			# over ON/OFF token
	ret

SKEYON:
	mov ah, -1			# prepare to dec scroll limit
	mov al, 255
KEYOX:					# AH = scroll limit diff., AL = new KEYSW
	cmp al, BYTE PTR KEYSW		# state change?
	mov BYTE PTR KEYSW, al
	jz KEYXX			# same: do nothing
	cmp BYTE PTR KEYSW, 255		# test if change to ON
	jnz KEYOX1			# change is to OFF: do not call FKYADV
	call FKYADV
KEYOX1:
	call KEYDSP			# on: display on the 25th line
KEYXX:
	ret

# LIST the function keys.
KEYLST:
	push ebx
	mov esi, offset STRTAB		# list 10 special function keys
	mov ecx, NMKEYF+(STKEYF*256)	# CL = count, CH = key number
KEYLS0:
	push esi
	mov al, 'F'
	call OUTDO
	push ecx
	movzx ebx, ch
	call LINPRT			# display the key number
	mov al, ' '
	call OUTDO
	pop ecx
	pop esi
	push esi
	push ecx
KEYLS1:
	cld
	lodsb
	or al, al
	jz KEYLS2			# brif end of string
	call KEYLSP
	jmp KEYLS1
KEYLS2:
	mov al, 13			# output carriage return
	call OUTDO
	mov al, 10			# output a line feed
	call OUTDO
	pop ecx
	pop esi
	add esi, 16			# next key address
	inc ch				# next key number
	dec cl
	jnz KEYLS0
	pop ebx
	jmp KEYOXX

KEYLSP:
	push esi
	cmp al, 13			# check for carriage return
	jnz KEYLSQ
	mov al, 27			# (escape, as the original's 33O)
KEYLSQ:
	call OUTDO
	pop esi
	ret

# -------------------------------------------------------------------------------------
# Soft key display on the last line (KEYDSP and helpers).
# -------------------------------------------------------------------------------------
TKEYOF:
	mov BYTE PTR KEYSW, 0		# turn function key display switch off

KEYDSP:
	push dx
	mov dh, 1			# set for col = 1
	mov dl, BYTE PTR LINCNT		# set for last line
	mov al, BYTE PTR KEYSW
	or al, al			# key on or off?
	jnz KEYDS0			# softkey display switch on
	call CLREOL			# clear from (DH,DL) to EOL
	pop dx
	ret

KEYDS0:
	mov BYTE PTR CSRTYP, 0		# set off mode cursor
	call SETCSR			# turn the cursor off
	push ebx
KEYDS1:
	call GETFMT			# get function key display format
KNXTST:
	push ax				# save key disp no.
	cmp ah, '0'			# single digit case?
	jz SINDIG			# print only one digit
	xchg ah, al
	call KEYDCH			# display first digit
	xchg ah, al
SINDIG:
	call KEYDCH			# display last digit
	push esi
	mov cl, BYTE PTR FKCNUM		# count of chars per fun. key (set by GETFMT)
	call XFGBG			# swap foreground and background colors
KNXTCH:					# write the next key character
	push ecx
	cld
	lodsb
	or al, al			# end of string?
	pushfd
	push esi
	jnz KEYDS4			# no, display char
	xor al, al			# else blank
KEYDS4:
	call KEYDCH			# display char, advance cursor
	pop esi
	popfd
	jnz KEYDS5			# brif not EOS
	dec esi
KEYDS5:
	pop ecx
	dec cl
	jnz KNXTCH			# loop for next character
	call XFGBG			# swap foreground and background colors
	call KEYDB			# follow with blank
	pop esi
	pop ax
	call KEYADV			# advance to next key
	dec ch
	jnz KNXTST			# loop for next key string
KEYDSX:
	pop ebx				# retrieve cursor position
	pop dx
	mov BYTE PTR CSRTYP, 3		# set user mode cursor
	call SETCSR			# turn on cursor
	ret

KEYDB:
	xor al, al			# for blank at end of key field
KEYDCH:
	push ax
	push ebx
	or al, al			# separating keys?
	jnz KEYDC1			# brif not
	mov al, ' '			# else write space
KEYDC1:
	cmp al, CR			# CR?
	jnz KEYNCR			# not CR
	mov al, 27			# FKEYCR (IBM CR key display graphic, FKEYCR=27D)
KEYNCR:
	cmp al, LF			# line feed?
	jnz KEYNLF			# not line feed
	mov al, 60			# substitute (74O in the original)
KEYNLF:
	push ecx
	mov ah, 0
	call SCROUT			# write the character at (DH,DL)
	inc dh
	pop ecx
	pop ebx
	pop ax				# restore key number
	ret

# GETFMT: get the function key display format from the OEM routine FKYFMT.
GETFMT:
	push ebx
	call FKYFMT			# OEM routine
	movzx ecx, WORD PTR [ebx]	# CH = key count, CL = characters per key
	push ecx
	mov BYTE PTR FKCNUM, cl
	mov esi, offset STRTAB		# SI = address of first fkey in table
	mov al, BYTE PTR [ebx+2]
	cbw
	push ax				# save number of first function key
	dec al				# set to zero relative
	mov cl, 4			# multiply by 16 (bytes per key)
	shl ax, cl			# AX = index of first display key
	movzx eax, ax
	add esi, eax
	pop ax
	call INTOCH			# get key number to character code
	cmp ah, '0'
	jz ONEDIG			# only one digit
	dec BYTE PTR FKCNUM		# adjust function key format for two digits
ONEDIG:
	call KADNRM			# normalize key address
	pop ecx
	pop ebx
	ret

# INTOCH: integer AL (0..100) to two characters in AX (radix 10).
INTOCH:
	push ecx
	xor ah, ah
	mov cl, 10			# load radix
	div cl
	add ax, 0x3030			# 3030H forms character codes
	xchg ah, al			# AH represents the significant digit
	pop ecx
	ret

# KEYADV: advance to the next key. Entry: AX = key number characters; SI = index into STRTAB.
KEYADV:
	add esi, 16			# move to next key table entry
	inc al
	cmp al, '9'
	jle KADNRM
	mov al, '0'
KADNRM:
	cmp esi, offset STRTAB+NMKEYF*16
	jb KADNMX
	mov esi, offset STRTAB		# wrap around to the first function key
					# print function key 10 number as 0 except when it is the first key
	cmp ah, '1'			# only true if 1st function key is key 10
	jnz KADNMX			# all other wraps are for 1 digit only
	mov ah, '0'
	mov al, '1'
	inc BYTE PTR FKCNUM		# re-adjust format
KADNMX:
	ret

# Swap foreground and background colors (toggle reverse video).
XFGBG:
	clc				# signal text attributes
	call GETFBC			# get foreground/background attributes
	xchg ax, bx
	call SETFBC			# set foreground/background attributes
	ret

# -------------------------------------------------------------------------------------
# PEN: no light pen in this port (NMPENT = 0). PEN ON/OFF/STOP and PEN(n) report an error.
# POLPEN is never called with a pen present, so it does nothing.
# -------------------------------------------------------------------------------------
PENS:
	jmp FCERR

PENF:
	call ONESUB			# parse "(n)"
	jmp FCERR

POLPEN:
	ret

# ONESUB: parse "(n)" and return n in [AL].
ONESUB:
	push dx
	call INTFR2			# [DX] = function argument
	jne FCERRI			# branch if [DX] is not [0..255]
	mov al, dl			# return argument in [AL]
	pop dx
	ret

FCERRI:
	jmp FCERR

# -------------------------------------------------------------------------------------
# STRIG (joystick triggers): NMSTRT = 0, so every trigger id is illegal, and
# STRIG(n) ON/OFF/STOP parse but do nothing.
# -------------------------------------------------------------------------------------
STRIGS:
	cmp al, '('
	jne STRIG1			# branch if STRIG ON or STRIG OFF
	mov ecx, 256*STROFF+NMSTRT
	jmp SEVSTT			# branch to common code
STRIG1:
	cmp al, _ON
	je STROK
	cmp al, _OFF
	je STROK
	jmp SNERR			# syntax error if not $ON or $OFF or (n)
STROK:
	jmp EOSCH1			# syntax error if not end-of-statement

	.globl STRIGF
STRIGF:
	call ONESUB			# parse "(n)", [AL] = n
	push ebx			# save text pointer
	mov ah, al			# map AL to AH
	inc ah
	and ah, 1			# [AH] = 1 for latched, 0 for current
	shr al, 1			# [AL] = 0-relative joystick trigger id
	cmp al, NMSTRT
	jb STRGOK
	jmp FCERR			# illegal trigger id
STRGOK:
	or ah, ah
	je NSTR0			# brif current (not latched) was requested
	push ax
	add al, STROFF			# see if STRIG(n) ON has been executed
	call EVADR
	mov ebx, edx
	test BYTE PTR [ebx], T_ON
	pop ax
	je NSTR0			# branch if event is not enabled
	add al, STROFF			# [AL] = 0-relative event index
	call TSTCEV			# test and clear event [AL]
	jmp STRRET
NSTR0:
	call RDTRIG			# [AL] = 0/1 for not-pressed/pressed
	cbw				# [AX] = 0/1
	neg ax				# [AX] = 0/-1
	movsx ebx, ax
STRRET:
	call MAKINT			# return [BX] as signed integer
	pop ebx				# restore text pointer
	ret

# STICK(n): joystick coordinates. No joysticks: RDSTIK reports an error.
STICKF:
	call ONESUB			# [AL] = stick id
	push ebx			# save text pointer
	call RDSTIK			# [BX] = stick coordinate
	jae STKOK
	jmp FCERR			# bad parameter
STKOK:
	call MAKINT			# return [BX] as signed integer
	pop ebx				# restore text pointer
	ret

# POLSTR: called by CHKINT at the start of each statement. Loops over NMSTRT (0) triggers.
POLSTR:
	xor ax, ax			# [AL] = joystick trigger #0
	inc ah				# [AH] = latched (not current) flag
STRILP:
	push ax				# save current trigger id
	add al, STROFF
	call EVADR			# [DX] points to event mask
	mov ebx, edx			# [BX] points to event mask
	test BYTE PTR [ebx], T_ON	# see if STRIG(n) ON has been done
	pop ax
	push ax
	je STRI1			# don't call RDTRIG if event not enabled
	call RDTRIG			# [AL] = 0/1 if trigger is not-pressed/pressed
	or al, al
	je STRI1			# brif this trigger has not interrupted
	pop ax				# restore [AL] = joystick trigger id
	push ax
	add al, STROFF			# [AL] = global event id
	call EVTRP			# signal the occurrence of a TRIGGER EVENT
STRI1:
	pop ax
	inc al
	cmp al, NMSTRT
	jb STRILP			# brif there are more triggers to poll
	ret

# CHKINT: called at the start of each statement (NEWSTT) to see if a trappable event occurred.
CHKINT:
	test BYTE PTR MSDCCF, 255	# test for Ctrl-C received by the OS
	jnz CHKIN1
	call POLLEV			# test for occurrence of trappable events
	jz CHKINX			# exit - no trappable event
CHKIN1:
	push ebx
	push ecx
	push edx
	call POLKEY			# trap function keys, CTL-C, CTL-S
	call POLCOM			# (no COM ports: returns at once)
	call POLPEN
	call POLSTR
	pop edx
	pop ecx
	pop ebx
CHKINX:
	ret

# -------------------------------------------------------------------------------------
# DATE$ and TIME$. The clock calls are the Stage 4 device layer (they replace the MS-DOS
# INT 21h date/time calls of the original):
#   DEVGDAT: CX = year, DH = month, DL = day
#   DEVSDAT: CX = year, DH = month, DL = day; AL = 0 if the date was accepted
#   DEVGTIM: CH = hour, CL = minute, DH = second, DL = hundredths
#   DEVSTIM: CH = hour, CL = minute, DH = second, DL = hundredths; AL = 0 if accepted
# -------------------------------------------------------------------------------------
	.globl DATES, DATEF, TIMES, TIMEF

# DATE$="[M]M/[D]D/[YY]YY" or "[M]M-[D]D-[YY]YY" (USA format)
DATES:
	call PRSDAT			# CX=year, DH=month, DL=day
	jmp SETDAT			# set system date

# X$=DATE$ returns "MM-DD-YYYY"
DATEF:
	call CHRGTR			# skip DATE$
	push ebx			# save text pointer
	mov al, 10
	call STRINI			# get space for 10 char string
	push edx			# save adr of string
	call GETDAT			# CX=year, DH=month, DL=day
	pop ebx				# [BX]=adr of string
	sub cx, 1900			# reduce year by two digits
	cmp cl, 100			# see if in 20th century
	mov ch, 19			# set up 20th century in case
	jb DATEF2			# brif so
	sub cl, 100			# subtract into next century
	inc ch				# 21st century
DATEF2:
	mov al, dh
	call PUTCHR			# store ascii month
	mov al, '-'
	call PUTCH2			# put separator
	mov al, dl
	call PUTCHR			# store ascii day
	mov al, '-'
	call PUTCH2			# put separator
	mov al, ch
	call PUTCHR			# store ascii century
	mov al, cl
	call PUTCHR			# store ascii year
	jmp PUTNEW			# put result and return (text pointer on stack)

PUTCHR:
	aam				# convert to unpacked BCD
	xchg al, ah
	or ax, 0x3030			# add "0" bias to both digits
	call PUTCH2
	mov al, ah
PUTCH2:
	mov BYTE PTR [ebx], al		# store char in string
	inc ebx
	ret

# PRSDAT: parse "MM/DD/YYYY" (or with "-"). Exit: CX = year, DH = month, DL = day, [BX] = text pointer.
PRSDAT:
	call SYNCHR
	.byte EQULTK			# must be DATE$ = string
	call FRMEVL
	push ebx			# save text pointer
	call FRESTR
	mov cl, BYTE PTR [ebx]		# save string len in [CL]
	cmp cl, 1			# string must not be null
	jb DATERR			# brif null str
	mov esi, DWORD PTR [ebx+1]	# [SI] has addr of string
	mov bl, cl			# working reg for string len
	call GNUM8			# [AX]=month
	mov dh, al			# [DH] = month
	call DATSEP			# skip / or -
	call GNUM8			# [AL]=day of month
	mov dl, al			# [DL] = day
	call DATSEP			# skip / or -
	call GNUM16			# [AX]=year (16 bits)
	cmp ax, 1978
	jnb DATE2			# branch if >= 1978
	cmp ax, 100
	jnb DATERR			# error if between 100 and 1977
	cmp ax, 78
	jnb DATE1			# add 1900 if >= 78
	add ax, 100			# add 2000 if <= 77
DATE1:
	add ax, 1900
DATE2:
	cmp ax, 2100
	jnb DATERR			# branch if year too large
	mov cx, ax			# CX = year
	pop ebx				# text pointer
	ret

# DATSEP: check for a date separator (- or /) and return if found.
DATSEP:
	or bl, bl
	jz DATERR			# error if string empty
	mov al, BYTE PTR [esi]
	cmp al, '/'
	jz DIGITX
	cmp al, '-'
	jz DIGITX			# branch if found
	jmp DATERR

GNUM8:
	call GNUM16			# [AX]=16-bit number
	or ah, ah
	jnz DATERR			# error if larger than 255
	ret

GNUM16:
	push cx				# save caller's [CX], [DX] (value pushes, 16-bit)
	push dx
	mov ax, 0			# initialize accumulator
GNUML:
	call DIGIT			# [CX]=0..9
	jb GNUMX			# branch if not legal digit
	mov dx, 10
	mul dx				# [AX]=[AX]*10
	jo DATERR			# branch if overflow
	add ax, cx			# add in new digit
	jmp GNUML
GNUMX:
	pop dx
	pop cx
RET2:
	ret

DIGIT:
	cmp bl, 1			# end-of-string?
	jb RET2				# brif end-of-string
	mov cl, BYTE PTR [esi]
	sub cl, '0'
	jb RET2				# branch if illegal digit
	cmp cl, 10
	cmc
	jb RET2				# branch if illegal digit
	movzx ecx, cl			# [CX]=digit
DIGITX:
	dec bl				# length -1
	inc esi				# [SI] points to next byte in string
	ret

DATERR:
	jmp FCERR

# SETDAT: set the system date from DATE$ (CX=year, DH=month, DL=day). Exit: [BX] preserved.
SETDAT:
	push ebx			# save text pointer
	call DEVSDAT			# give date to the device layer
	or al, al			# date OK?
	jnz DATERR			# brif not
	pop ebx
	ret

# GETDAT: CX = year, DH = month, DL = day of month.
GETDAT:
	call DEVGDAT
	ret

# -------------------------------------------------------------------------------------
# TIME$
# -------------------------------------------------------------------------------------
TIMES:
	call PRSTIM			# CH=hour, CL=min, DH=sec, DL=.01sec
	jmp SETTIM			# set system time

# X$=TIME$ returns "HH:MM:SS"
TIMEF:
	call CHRGTR			# skip TIME$ (it was CPI'ed in FRMEVL)
	push ebx			# save text pointer
	mov al, 8
	call STRINI			# get space for 8 char string
	push edx			# save addr of string
	call GETTIM			# CH=hour, CL=min, DH=sec
	pop ebx				# restore addr of string
	mov al, ch
	call PUTCHR			# store ascii hours
	mov al, ':'
	call PUTCH2
	mov al, cl
	call PUTCHR			# store ascii minutes
	mov al, ':'
	call PUTCH2
	mov al, dh
	call PUTCHR			# store ascii seconds
	jmp PUTNEW			# put result and return (text pointer on stack)

# PRSTIM: parse "HH[:MM[:SS[.TT]]]". Exit: CH=hours, CL=minutes, DH=seconds, DL=.01 secs, [BX]=text pointer.
PRSTIM:
	call SYNCHR
	.byte EQULTK			# must be TIME$ = string
	call FRMEVL
	push ebx			# save text pointer
	call FRESTR
	mov cl, BYTE PTR [ebx]		# save string len in [CL]
	cmp cl, 1			# string must not be null
	jb TIMERR			# brif null str
	mov esi, DWORD PTR [ebx+1]	# [SI] has addr of string
	mov bl, cl			# working reg for string len
	call GNUM8			# [AX]=hours
	cmp al, 24
	jnb TIMERR
	mov ch, al			# [CH] = hours
	call TIMSEP
	call GNUM8
	cmp al, 60
	jnb TIMERR
	mov cl, al			# [CL] = minutes
	call TIMSEP
	call GNUM8
	cmp al, 60
	jnb TIMERR
	mov dh, al			# [DH] = seconds
	call TIMSEP
	call GNUM8
	cmp al, 100
	jnb TIMERR
	mov dl, ah			# [DL] = 100ths
	pop ebx				# text pointer
	ret

TIMSEP:
	or bl, bl
	jz TIMSXX
	dec bl
	cld				# set to increment
	lodsb
	cmp al, ':'
	jz TIMSXX
	cmp al, '.'
	jnz TIMERR
TIMSXX:
	ret

TIMERR:
	jmp FCERR

# SETTIM: set the system time from TIME$ (CH=hour, CL=min, DH=sec, DL=.01sec).
SETTIM:
	push ebx
	call DEVSTIM			# give time to the device layer
	or al, al			# time OK?
	jnz TIMERR			# brif not
	pop ebx
	ret

# GETTIM: CH=hour, CL=min, DH=sec.
GETTIM:
	call DEVGTIM
	ret

# -------------------------------------------------------------------------------------
# Statements and functions not supported in this port report "Advanced feature". Their
# dispatch names all fall through to ADVERR, as in the original.
# PALETTE reports a syntax error.
# -------------------------------------------------------------------------------------
	.globl TIMER, ERDEV, IOCTL, CHDIR, MKDIR, RMDIR, SHELL, ENVIRON, VIEW, WINDOW, PMAP, ADVERR, PALETE
PALETE:
	jmp SNERR
TIMER:
ERDEV:
IOCTL:
CHDIR:
MKDIR:
RMDIR:
SHELL:
ENVIRON:
VIEW:
WINDOW:
PMAP:
ADVERR:
	mov dl, ERRADV
	jmp ERROR
