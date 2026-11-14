# GIOKYB.ASM -> giokyb.S. Keyboard device driver (machine independent part): KYBINI, KYBCLR,
# KYBEOF, KYBLOC, KYBLOF, KYBCLS, KYBOPN, KYBSIN, INCHRI, CHGET, KEYIN, POLKEY (and its break,
# trap and queue helpers), CHKKYB, CNTCCN, CHSNS, CONIN, the soft-key and super-shift entries,
# INKEY/INKGET, KYBSNS, FKYSNS, SFTOFF, STCTYP and the keyboard dispatch table KYBDSP.
#
# Port notes (see NOTES.md and platform_win32.S):
#  - Terminal input goes only through the platform boundary: PLAT_CHAVAIL (non-blocking) and
#    PLAT_CHIN (blocking; CF=1 at end of input). KEYINP is the OEM keyboard hook that POLKEY calls.
#    It is not in the original source tree (IBM supplies it), so it is written here.
#  - Ctrl-C arrives as character 3. KEYINP turns it into CTLBRK (FF03), the Ctl-Break code that
#    POLKEY tests, so the break path (CNTCCN, STOP) is the original one.
#  - The Win32 console delivers no function-key codes. Soft keys (F1..F10), super shift, and the
#    ON KEY cursor traps are reached only from 2-byte codes (AH=80 or AH=FF), which KEYINP never
#    returns. They are marked UNSUPPORTED.
#  - Pointers held in SI, DI, BP and in BX as an address are 32-bit. The counts in CX are the
#    16-bit values of the original (ECX is not used as a count in this file).
#  - Original interpreter calls keep their names: INITQ, GETQ, PUTQ, NUMQ, LFTQ, INCHSI, BCHRSI,
#    INIFDB, FINPRT, PDCBAX, MAKINT (GIO86), SCNPOS, SCNBRK, SCNSWD..SCNGCW (SCNDRV),
#    SETCSR (scrout.S), KEYTRP, SNDRST (GWSTS), STOP (BIMISC), STRINI, STRIN1, SETSTR, PUTNEW
#    (BISTRS), CHRGTR (GWMAIN), EDTBRK and CLRFLG (scedit.S).

	.intel_syntax noprefix
	.include "oem.inc"
	.include "gio86u.inc"
	.include "msdosu.inc"

	.set CTLBRK, 65283		# &HFF03 is Ctl-Break
	.set CTLPAS, 65299		# &HFF13 is Ctl-Pause
	.set KYBQSZ, 32			# size of keyboard queue (gwram.S)

	.section .text

# ---------------------------------------------------------------- KYBINI
# KYBINI puts the keyboard device server in an initial state. Called at initialization
# time and after CTL-C. On exit, all registers are preserved.
	.globl KYBINI
KYBINI:
	push ax
	push ebx			# BX is an address here (4 bytes; PDCBAX pops it with pop ebx)
	push cx
	push dx
	pushfd
	push esi
	xor ax, ax
	mov WORD PTR CMDOFF, ax		# [CMDOFF]=0 (no soft key)
	mov esi, offset KYBQDS		# SI = keyboard queue descriptor
	mov ebx, offset KYBQUE		# BX points to 1st byte of queue buff
	mov ax, KYBQSZ			# AX = size of keyboard queue
	call INITQ			# Initialize keyboard queue
	call FINPRT			# reset PTRFIL to Keyboard/CRT
	pop esi
	popfd
	jmp PDCBAX			# POP DX, CX, BX, AX and return

# KYBCLR - Clear the keyboard buffer (documented to OEMs; used by the IBM poke).
# Entry - none. Exit - none. Segment registers and stack preserved.
	.globl KYBCLR
KYBCLR:
	call KYBINI			# Initialize the keyboard buffer
	ret

# KYBEOF - test for End-Of-File on device.
# Entry - SI points to File-Data-Block. Exit - [BX] = -1 if EOF, 0 if not EOF
	.globl KYBTRM
KYBEOF:
	xor ebx, ebx			# 0 means not at eof
	or esi, esi
	jz KBEOFX			# branch if not pseudo keyboard FDB
	call INCHSI			# [AL]=next byte from keyboard
	jb YKYEOF			# branch if next key = CTL-Z
	call BCHRSI			# put this back in queue
KBEOFX:	ret
YKYEOF:	dec ebx				# BX=-1, end-of-file is true
KYBTRM:	ret

# KYBLOC - Number of Bytes in input buffer for KEYBOARD device.
# Entry - SI points to File-Data-Block. Exit - [BX] = result.
KYBLOC:
	push esi			# save FDB pointer
	mov esi, offset KYBQDS		# SI points to KYB queue descriptor
	call NUMQ			# [AX]=number of bytes queued in KYB Q
	pop esi
	movzx ebx, ax			# return result in BX (a 16-bit count)
	test BYTE PTR [esi+F_FLGS], FL_BKC
	jz KYLOCX			# branch if char not backed up
	inc bx
KYLOCX:	ret

# KYBLOF - number of bytes free in KEYBOARD input buffer.
# Entry - SI points to File-Data-Block. Exit - [Floating-Point-Accumulator] = result.
KYBLOF:
	mov esi, offset KYBQDS		# SI points to KYB queue descriptor
	call LFTQ			# [AX]=number of bytes free in KYB Q
	movzx ebx, ax
	jmp MAKINT			# return result in FAC

# KYBCLS - perform any device dependent close functions (none for the keyboard).
KYBCLS:
RET11:	ret

# KYBOPN - perform any device dependent open functions.
# Exit - [SI] points to new FDB (linked by INIFDB). Only input is allowed on the keyboard.
KYBOPN:
	mov ah, MD_SQI			# allow input only
	cmp BYTE PTR FILMOD, MD_RND
	jnz KYBOPX			# Leave the mode as it is
	mov BYTE PTR FILMOD, ah		# Force the mode to INPUT
KYBOPX:
	jmp INIFDB

# KYBSIN - Sequential Input.
# Entry - SI points to File-Data-Block (or 0 for the keyboard itself).
# Exit  - [AL] = next byte from Keyboard File, carry set if EOF (CTL-Z read) and SI is an FDB.
#         SI must be preserved for KYBEOF to work. All other registers preserved.
	.globl KYBSIN, INCHRI, CHGET, KEYIN
INCHRI:
	mov al, BYTE PTR SAVKEY
	cmp BYTE PTR SAVKYF, 0
	mov BYTE PTR SAVKYF, 0
	jnz KBSINX			# Send second bytes through unfiltered
KEYGET:
	stc
	call STCTYP			# Set new cursor type to user mode (PSW.C set)
	call KEYSIN			# Read a key from the OS (could be a null fkey)
	call INFMAP			# OEM fixed length input map routine
	jz KEYGET			# OEM filtered out the key - get the next
	jmp KYBSI2			# Process as other keys
CHGET:
KYBSIN:	mov al, BYTE PTR SAVKEY
	cmp BYTE PTR SAVKYF, 0
	mov BYTE PTR SAVKYF, 0		# clear 2nd-byte of 2-byte sequence flag
	jnz KBSINX			# branch if 2nd-key of 2-byte sequence
	call STCTYP			# Set to insert or overwrite cursor (PSW.C reset)
	call KEYIN			# [AX]=next character from keyboard
KYBSI2:
	jb KBSIN2			# branch if got 2-byte sequence (f-key/KANJI)
	pushfd
	or esi, esi
	jz KB1X				# branch if no FDB
	cmp al, ASCCTZ			# CTL-Z=eof for keyboard
	jne KB1X			# branch if not EOF
	popfd
	stc				# return EOF indication
	jmp KBSINX
KB1X:	popfd
	jmp KBSINX
KBSIN2:
	mov BYTE PTR SAVKEY, al		# save 2nd byte of 2-byte sequence
	mov BYTE PTR SAVKYF, 255	# Set saved key flag
	xchg ah, al			# return 1st byte to user
	clc				# clear carry (not EOF)
KBSINX:	ret

# KEYIN turns cursor on if no key is available. It then waits for a key from the keyboard
# (if one wasn't already there).
# Exit - if Z is true, no key was ready, else [AX]=key. If C is true, returns 16 bit character.
#        Soft keys are expanded if not being trapped, and not null.
#        All other registers are preserved.
KEYIN:
IGNNFK:	call KEYSIN			# Get a key (possibly a null function key)
	pushfd
	jae NOTNFK			# branch if not 2-byte key code
	cmp ah, 128
	jne NOTNFK			# branch if definitely not a null Function key
	cmp al, 32
	jb NOTNFK			# branch if definitely not a null Function key
	cmp al, 65
	jae NOTNFK			# branch if definitely not a null Function key
	popfd
	jmp IGNNFK			# ignore null Function key

NOTNFK:	popfd
	ret

# KEYSIN - Get a key. This routine returns null function keys.
# Exit - PSW.C set indicates a two byte key code.
KEYSIN:
	call CHSNS			# try to get next key
	jne KEYINX			# Return with key if there is one.
	push dx
	call SCNPOS			# [DH]=1 relative column (cursor position), [DL]=1 relative line
	call SETCSR			# Set the cursor
	pop dx
CHWAIT:	call CHSNS			# Has a key been typed?
	jne KEYINX			# yes
	cmp BYTE PTR KEYEOF, 0		# input has ended: no key will ever come, so end cleanly
	jnz SYSTME
	jmp CHWAIT			# No, wait
KEYINX:	pushfd
	push ax
	mov BYTE PTR CSRTYP, 3		# Indicate user cursor
	call SETCSR			# Set the cursor
	pop ax
	popfd
	ret

# ---------------------------------------------------------------- OEM hooks
# KEYINP: the OEM keyboard hook that POLKEY calls. IBM supplies it in the OEM source, which is
# not in this tree, so it is written here on top of the platform boundary.
#   Exit: ZF=1 no key ready (AX = 0).
#         ZF=0, CF=0: one-byte key in AX (AH = 0).
#         ZF=0, CF=1: AX = CTLBRK (FF03) for CTL-C (character 3), the Ctl-Break code.
#   End of input (PLAT_CHIN returns CF=1) exits the process through SYSTME.
# The "cmp esp, 0" sets ZF=0 and CF=0 without changing any register.
KEYINP:
	call PLAT_CHAVAIL		# ZF=0 when PLAT_CHIN will not wait
	jz KEYINP_NONE
	call PLAT_CHIN			# AL = character with CF=0; CF=1 at end of input
	jc KEYINP_EOF
	cmp al, ASCCTC			# CTL-C (3): Ctl-Break
	je KEYINP_BRK
	movzx eax, al			# AH = 0: one-byte key
	cmp esp, 0			# ZF=0, CF=0
	ret
KEYINP_BRK:
	mov eax, CTLBRK
	cmp esp, 0			# ZF=0
	stc				# CF=1: two-byte key code (FF03)
	ret
KEYINP_NONE:
	xor eax, eax			# ZF=1: no key
	ret
KEYINP_EOF:
	mov BYTE PTR KEYEOF, 1		# end of input: remember it. A poll (POLKEY) just sees no key;
	xor eax, eax			# the blocking waits (CHWAIT, GOTCTS) end the process.
	ret

	.lcomm KEYEOF, 1		# set when the input has ended

# INFMAP: OEM fixed length input map. The IBM filter is not in this tree. This port passes
# every key through: flags are left as KEYSIN returned them (ZF=0 when a key was read).
INFMAP:
	ret

# INKMAP: OEM map for INKEY$. Not in this tree; this port passes every key through with the
# flags from CHSNS unchanged (ZF=0 when a key was read, CF=1 for a 2-byte code).
INKMAP:
	ret

# ---------------------------------------------------------------- POLKEY
# POLKEY is called from several places in BASIC to "poll" the keyboard.
# Exit - DI is used. All other registers are preserved.
#        If CTL C was typed, control does not return to the caller.
#
# Function:
#   get key from operating system (KEYINP)
#   while keyboard data is ready to be read begin
#     get key
#     if key is CTL-C, reset soft-key pointer, key-queue, AUTFLG, SEMFLG; call SNDRST;
#        exit to CNTCCN (resets stack and jumps to STOP)
#     if key is CTL-S, pause until non-CTL-S key is pressed
#     if key is 1st byte of a function key, continue getting keys until it is or is not one
#     if it was a function key: trap it if trapping is enabled, else queue it for CHSNS
#     else queue key for CHSNS
#   end {while keyboard data ready}
	.globl POLKEY, CHKKYB
POLKEY:
	push edi
	pushfd
	push ax
	push ebx
	push cx
	push dx
GETKLP:
	test BYTE PTR MSDCCF, 255
	jnz ITSCTC			# Branch if Ctl-C interrupt detected
	call KEYINP			# [AX]=next key from keyboard if one exists
	je POLKXI			# branch if no key present
	jae NOTTWO			# branch if not 2-byte key sequence
	cmp ah, 255			# Test for control function range
	jnz NTCTLFN			# Not a control function
	call TRPCHK			# Check for trapping in this range (UNSUPPORTED: cursor traps)
	jnz GETKLP			# Trap set - get next key
NTCTLFN:
	cmp ah, 128
	jne NOTFUN			# branch if not function key
	# UNSUPPORTED: function keys (AH=128). The Win32 console delivers no function-key codes;
	# the trap and queue code below is kept as the original.
	push ax				# save key-code
	sub al, 32			# [AL]=0 for 1st function key
	cmp al, NMKEYT			# see if its a trapable function key
	jae NOFUN1			# branch if not
	call KEYTRP			# Trap key if event is ON
	pop ax				# restore key code
	jnz GETKLP			# branch if Key was trapped (don't queue)
	jmp NOTFUN			# Not trapped function key (queue key)
POLKXI:	jmp POLKYX

NOFUN1:	pop ax				# Restore key code
NOTFUN:
	cmp ax, CTLBRK			# check for Ctl-Break
	je ITSCTC			# branch if CTL C
	cmp ax, CTLPAS
	je ITSINT			# branch if CTL S
NOTRAP:
	push ax
	call LFTQ			# [AX]=number of bytes free in KYB Q
	cmp ax, 3			# Test for space in keyboard queue
	pop ax				# Restore key code
	jb GETKLP			# Loop for next key (interrupt?)
	xchg ah, al
	call PKQUE			# append 1st byte of sequence to key queue
	xchg ah, al
	jmp QUEKEY			# append 2nd byte of sequence to key queue
NOTTWO:
	cmp al, 254			# Test for three byte char (IBM "scan codes")
	jnz QONEBT			# Queue a one byte character
	push ax
	call LFTQ			# [AX]=number of bytes free in KYB Q
	cmp ax, 4			# Test for space in keyboard queue
	pop ax				# Restore key code
	jb GETKLP			# Loop for next key (interrupt?)
	call PKQUE			# Queue the first byte
	mov ax, dx			# Get second and third bytes
	jmp NOTRAP			# Go queue the second and third bytes
QONEBT:
	jmp QUEKEY			# else queue the key for CHSNS
ITSCTC:
	call KYBINI			# clear keyboard queue, reset PTRFIL
	mov BYTE PTR MSDCCF, 0		# Reset Ctl-Break interrupt flag
	call SNDRST			# reset background music
	mov esp, DWORD PTR SAVSTK	# [SP]=SP of interrupted statement
	mov ebx, DWORD PTR SAVTXT	# [BX]=text pointer of interrupted stmt
	mov ax, CTLBRK			# [AX]=Key Code for BREAK (CTL-C)
ITSINT:	call CNTCCN			# process CTL C or S
	jmp GETKLP			# dont queue CTL-S or CTL-C

QUEKEY:	call PKQUE			# queue key [AL] for CHSNS
	jmp POLKYX			# one key per poll: CHSNS calls POLKEY again for the next (draining
					# the whole input here ran to end of input before the editor saw any key)

# At this point, Keyboard input queue from OS is flushed.
POLKYX:	pop dx				# restore caller's registers
	pop cx
	pop ebx
POLKXX:	pop ax
	popfd
	pop edi
RET8:	ret

# TRPCHK - Check for key trapping (UNSUPPORTED: ON KEY cursor traps. These codes come from
# AH=FF control functions, which the Win32 console does not deliver. Kept as the original.)
# Entry - AX = key code. Exit - PSW.Z set indicates key was not trapped.
TRPCHK:
	push ax				# Save key code
	mov ebx, offset TRPKTB - 1	# BX points before 1st byte of key trap table
CHKTRP:	inc ebx
	mov ah, BYTE PTR [ebx]
	inc ebx				# BX points to Trap Key ID for key [AH]
	or ah, ah			# test for end-of-table
	je NTRAP			# branch if not a trappable key
	cmp ah, al
	jne CHKTRP			# branch if they don't match (continue search)
	mov al, BYTE PTR [ebx]		# [AL]=key-trap id
	call KEYTRP			# Trap key if event is ON
NTRAP:	pop ax				# Restore key code
	ret

# CHKKYB - OEM version of POLKEY, documented to the OEM as the way to check for keyboard
# interrupts. Exit - all registers and PSW preserved.
CHKKYB:
	push esi
	push ebp			# For safety
	call POLKEY
	pop ebp
	pop esi
	ret

# CNTCCN performs special action upon receipt of CTL-S or CTL-C.
CNTCCN:
	cmp ax, CTLPAS			# check for PAUSE
	jnz NOTCTS			# branch if not
GOTCTS:	call KEYINP
	jnz GOTCTS_K			# got a key
	cmp BYTE PTR KEYEOF, 0		# input has ended: the pause can never end
	jnz SYSTME
	jmp GOTCTS			# wait for resume key (anything but CTL S)
GOTCTS_K:
	cmp ax, CTLPAS			# check for PAUSE
	je GOTCTS			# branch if got another CTL-S
NOTCTS:
	cmp ax, CTLBRK			# check for Ctl-Break
	jne RET8			# return if not CTL-C
	call EDTBRK			# Clear editor flags and position cursor
	call SCNBRK			# Clear flags associated with screen driver
	push ax
	call FINPRT			# Reset I/O
	pop ax
	mov ax, WORD PTR CURLIN		# Print "BREAK" message in program mode only
	and al, ah			# AL=^D255 if direct mode
	xor ah, ah			# Set PSW.Z so STOP won't give Syntax Error
	jmp STOP

# PKQUE appends [AL] to the keyboard circular queue.
PKQUE:
	push esi
	mov esi, offset KYBQDS		# SI = keyboard queue descriptor
	call PUTQ			# append [AL] to keyboard queue (read by CHSNS)
	pop esi
	ret

# Keyboard Trap Table (for KEY TRAPPING of non-soft keys). Referenced by TRPCHK.
	.section .rodata
TRPKTB:
	.byte 30, NMKEYF+0		# ON KEY (Cursor Up)
	.byte 29, NMKEYF+1		# ON KEY (Cursor Left)
	.byte 28, NMKEYF+2		# ON KEY (Cursor Right)
	.byte 31, NMKEYF+3		# ON KEY (Cursor Down)
	.byte 0				# end-of-table

	.section .text

# ---------------------------------------------------------------- CHSNS
# Get key from keyboard if one is ready.
# Exit - if Z is true, no key was ready, else [AX]=key. If C is true, returns 16 bit character.
#        Soft keys are expanded if not being trapped, and not null.
#        if NZ AND NC AND [AL] = FF then a two byte character is returned in DX.
#        All other registers are preserved.
	.globl CHSNS
CHSNS:
	push ebx			# save caller's registers
	push cx
	push esi
	push edi
CHSNSI:	xor dl, dl			# clear high byte of 2-byte sequence
CHSNS0:	call CONIN			# check for SOFTKEY
	jnz CHSNSS			# BRIF key found
	or dl, dl
	jz CHSNSX			# BRIF no key sensed and not 2nd of 2 bytes
	jmp CHSNS0			# BRIF need 2nd byte of 2 byte character

CHSNSS:	or dl, dl
	jnz CHSNS2			# BRIF 2nd byte of 2 byte character
	call CKDBLK			# see if this is the 1st byte of 2-byte seq
	jae CHSNG1			# branch not
	mov dl, al			# Put first byte in [DL]
	jmp CHSNS0			# Go get 2nd byte
CHSNS2:	mov ah, dl			# Put 1st byte in [AH]
	cmp ah, 128			# Check for 8080 code which represents single 80
	jnz CHSN2A
	cmp ah, al
	jz CHSNG1			# Return 80 for 8080 char code
CHSN2A:	call CHKFKY			# see if AX is a non-null function key
	jz CHSNS3			# BRIF not a string key that needs expansion
	jb CHSNG1			# BRIF is super shift key (return first byte now)
	mov BYTE PTR F_SUPR, 0
	jmp CHSNSI			# Is string key, get first expansion and return
CHSNS3:
	or ah, ah			# else NZ, C Indicates 2 byte character
	stc
	jmp CHSNSX
CHSNG1:	or al, al			# set NZ, no-carry (got 1 byte)
	mov ah, 0			# high-byte = 0 for single byte characters
# Check for 3 byte sequence case
	pushfd
	cmp al, 254			# Test for three byte sequence
	jnz CHSNG2			# Not a three byte sequence
	push ax				# Save first byte
	call CONIN			# Get second byte
	mov dh, al
	call CONIN			# Get third byte
	mov dl, al
	pop ax				# Restore third byte
CHSNG2:	popfd
CHSNSX:	pop edi
	pop esi
	pop cx
	pop ebx
	ret

# CKDBLK - is AL the first byte of a two-byte sequence (80,nn or FF,nn)? Carry set if so.
CKDBLK:
	cmp al, 128			# First check for 80,nn key
	stc
	je CKDBLX			# branch if 1st of 2-byte sequence
	cmp al, 255
	stc
	je CKDBLX			# BRIF 1st byte of two byte control function
	clc
CKDBLX:	ret

# CONIN gets the next key from the soft-key being expanded (if any). If no soft-key is active,
# it calls POLKEY to get and queue a key from the OS, then tries to get a key from the keyboard
# queue. On exit, Flags.Z is set if no key is ready, else [AL]=key.
	.globl CONIN
CONIN:
	push ebx
	call GETFKY			# expand soft-key if active
	jnz CONINX			# branch if expanding soft-key
	push esi
	call POLKEY			# try to get and queue key from OS
	mov esi, offset KYBQDS
	call GETQ			# [AL]=next key from keyboard queue
	pop esi
CONINX:	pop ebx
	ret

# CHKFKY - returns NZ if AX is the universal key code for a non-null soft key. Sets NC for a
# soft key and C for a super shift key. (UNSUPPORTED: soft keys F1..F10 and super shift. The
# Win32 console delivers no function-key codes, so only a raw 0x80 byte can reach the soft-key
# branch, and that stops with ADVERR rather than expanding a key.)
CHKFKY:
	cmp ah, 128
	jne NTFKYX			# branch if not soft-key
	push ax
	sub al, 32
	jb NTFKY1			# branch if not soft-key
	cmp al, NMKEYF
	jae NTFKY1			# branch if not soft-key
	pop ax
	jmp ADVERR			# UNSUPPORTED: soft-key expansion (F1..F10) not in Stage 4
NTFKY1:
	jb NTFKY9			# BRIF not super shift key
	pop ax
	push ax
	cmp al, 'A'
	jb NTFKY9			# BRIF not super shift key
	cmp al, 'Z'+1
	jb SUPRKY			# BRIF is super shift key
NTFKY9:
	pop ax
NTFKYX:	xor bl, bl			# set FLAGS.Z - not soft-key (and FLAGS.NC not super shift)
	ret

# SUPRKY - super shift key expansion (direct mode only; the program-mode case returns to
# NTFKY9). UNSUPPORTED: the reserved-word lookup needs the OEM MAPSUP and the ALPTAB/ALPTAX
# tables, and super shift keys come only from function-key codes. Not translated.
SUPRKY:
	cmp WORD PTR CURLIN, 65535	# Test for direct mode
	jnz NTFKY9			# Not direct mode - don't expand Supershift key
	pop ax
	jmp ADVERR			# UNSUPPORTED: super shift expansion (direct mode)

# GETFKY: called to retrieve a character from the SOFTKEY buffer.
# Entry - CMDOFF is the index into the buffer. Exit - flags.Z set: no key available.
# UNSUPPORTED: GTSFKY and EOKTST (the soft-key character fetch and its end test) are not
# translated. They run only while CMDOFF is nonzero, and CHKFKY and SUPRKY stop with ADVERR
# before CMDOFF can be set, so this entry is reached only with CMDOFF = 0.
GETFKY:
	cmp WORD PTR CMDOFF, 0		# Softkey available?
	jz GETFKX			# No special key available
	jmp ADVERR			# UNSUPPORTED: soft-key expansion in progress (not in Stage 4)
GETFKX:	ret

# SFTOFF - turn off softkey expansion for the current softkey (documented to OEMs for the
# PEEK/POKE filters).
	.globl SFTOFF
SFTOFF:
	mov WORD PTR CMDOFF, 0		# Stop soft key expansion
	mov BYTE PTR F_SUPR, 0		# Turn off super shift flag
	ret

# KYBSNS - Detect whether keys are available in the keyboard buffer (documented to OEMs).
# Exit - PSW.Z set indicates that the keyboard buffer is empty. All registers preserved.
	.globl KYBSNS
KYBSNS:
	push ax
	push esi
	mov esi, offset KYBQDS		# SI = keyboard queue descriptor
	call NUMQ			# Get the number of keys available
	or ax, ax			# Set flags
	pop esi
	pop ax
	ret

# FKYSNS - sense the availability of a softkey (documented to IBMLIK OEMs for a PEEK address).
# Exit - PSW.C set indicates softkey expansion is in progress. PSW.Z set indicates that the next
# softkey is not the last key of a supershift key. All registers preserved.
# (Stage 4: CMDOFF stays 0, so this always reports no expansion in progress.)
	.globl FKYSNS
FKYSNS:
	cmp BYTE PTR CMDOFF, 0		# Test for expansion in progress
	jz FKYSNX			# Expansion not in progress
	cmp BYTE PTR F_SUPR, 255	# Test for super-shift expansion
	jz FKYSNW			# SS key but not last key
	cmp BYTE PTR F_SUPR, 0		# Test for super-shift expansion
	jz FKYSNW			# Function key expansion in progress
FKYSNW:
	stc
FKYSNX:	ret

# INKEY$ - get key from key-queue if one exists, else return null string.
# Returns a 2-byte string for DBLCHR. For function keys, returns the next char of the function
# key expansion if the key is not null. If the F-key is null, returns the 2-byte string that
# identifies the function key (UNSUPPORTED in Stage 4: no function-key codes).
# This never waits: CHSNS -> POLKEY -> KEYINP tests PLAT_CHAVAIL before PLAT_CHIN, and queued
# type-ahead (KYBQDS) is still returned first.
	.globl INKEY
INKEY:
	call CHRGTR
	push ebx			# save text pointer
INKGET:
	call CHSNS			# get next key from queue
	jz NULRT			# branch if no key is queued
	call INKMAP			# OEM map routine for INKEY$
	jz INKGET			# OEM has no associated character
	jae INKEY1			# branch if not 2-byte sequence
	push ax				# save char code
	mov al, 2
	call STRINI			# initialize 2-byte string
	mov ebx, DWORD PTR DSCPTR
	pop dx				# restore char code
	xchg dh, dl			# return high-byte in left end of string
	mov WORD PTR [ebx], dx
	jmp PUTNEW
INKEY1:
	push ax
	call STRIN1			# MAKE ONE CHAR STRING
	pop ax
	mov dl, al
	xchg ah, al			# put bytes in correct order
	call SETSTR			# STUFF IN DESCRIPTOR AND GOTO PUTNEW
NULRT:
	mov ebx, offset DSEGZ		# GUARANTEED ZERO IN DATA SEGMENT
	mov DWORD PTR FACLO, ebx
	mov BYTE PTR VALTYP, 3
	pop ebx				# restore text pointer
	ret

# STCTYP - set the new cursor type.
# Entry - carry set: the cursor is the user cursor. Carry reset: insert or overwrite cursor.
# Exit  - all registers preserved.
	.globl STCTYP
STCTYP:
	push ax
	mov al, 3			# Assume user cursor
	jb CSRSET			# Assumption correct
	dec al				# Assume overwrite mode cursor
	test BYTE PTR F_INST, 255	# Test for insert mode
	jz CSRSET			# Ovewrite mode discovered
	dec al				# Set for insert mode
CSRSET:	mov BYTE PTR CSRTYP, al		# Save the type
	pop ax
	ret

# SETCSR (imported from scrout.S) sets the cursor to CSRTYP.

# ---------------------------------------------------------------- dispatch table
# KYBDSP: device dispatch table. Each entry is a 4-byte address (was DW OFFSET in the original).
	.section .rodata
	.globl KYBDSP
KYBDSP:
	.long KYBEOF			# test EOF for file opened to this device
	.long KYBLOC			# LOC
	.long KYBLOF			# LOF
	.long KYBCLS			# perform special CLOSE functions for this device
	.long SCNSWD			# set device width
	.long DERBFM			# GET/PUT random record from/to this device
	.long KYBOPN			# perform special OPEN functions for this device
	.long KYBSIN			# input 1 byte from file opened on this device
	.long SCNSOT			# output 1 byte to file opened on this device
	.long SCNGPS			# POS
	.long SCNGWD			# get device width
	.long SCNSCW			# set device comma width
	.long SCNGCW			# get device comma width
	.long DEVBIN			# block input from file opened on this device
	.long DEVBOT			# block output to file opened on this device

	.section .text

# ---------------------------------------------------------------- not translated
# UNSUPPORTED (Stage 4): function-key reachable code only. Not present in this file:
#   GTSFKY, EOKTST (soft-key expansion; see GETFKY), SUPRKY body (super shift reserved-word
#   lookup; uses the OEM MAPSUP and ALPTAB/ALPTAX). The entries are kept as ADVERR jumps above.
