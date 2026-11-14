# platform_qdos.S: the QDOS/386 console and file boundary for the GW-BASIC port (GNU as 1.38,
# AT&T syntax). It replaces src/platform_win32.S in the QDOS build (Makefile.qdos); the
# interface and register rules are the same:
#   PLAT_INIT     : open the CON channel and set it to raw mode (no echo: the interpreter echoes).
#   PLAT_CHOUT    : AL = character, buffered; written out when the buffer is full.
#   PLAT_FLUSH    : write the buffered output now.
#   PLAT_CHIN     : blocking read of one character (qd_con_getc). Flushes output first.
#                   Returns AL = character, CF=0; at end of input AL=0, CF=1.
#   PLAT_CHAVAIL  : ZF=0 when PLAT_CHIN will not wait (a key is queued), ZF=1 otherwise.
#   PLAT_EXIT     : EAX = exit code. Flushes output and ends the job (qd_exit). Does not return.
#   PLAT_FOPEN    : EBX = name, EDX = 0 to read, nonzero to create or replace for writing.
#                   CF=0 and EAX = channel, or CF=1 and EAX = an error code (2 not found, 5 I/O).
#   PLAT_FSIZE    : EBX = channel. CF=0 and EAX = size, or CF=1.
#   PLAT_FREAD    : EBX = channel, ECX = count, EDX = buffer. CF=0 only if all COUNT bytes were
#                   read; a short read is CF=1 with EAX = 38.
#   PLAT_FWRITE   : EBX = channel, ECX = count, EDX = buffer. CF=0 only if all bytes were written.
#   PLAT_FCLOSE   : EBX = channel. Always returns (ECX and EDX may change).
# Registers EBX, ECX, EDX, ESI, EDI and EBP are preserved except as stated; EAX carries results.
#
# The libqdos calls use the C convention of GCC 1.40 (cdecl; arguments pushed right to left): they
# clobber EAX, ECX and EDX only, so these routines save those when they call into libqdos.
#
# Files: a name becomes "WIN1_" + the name in upper case, an 8.3 name of at most 12 characters.
# The kernel must have the writable WIN1_ volume (qd_open_ex). A write is chunked to QD_WRITE_MAX
# bytes per qd_write call.

	.set QD_WRITE_MAX, 256
	.set PLAT_OBSIZE, 4096
	.set PLAT_POLL_MASK, 63		# the kernel is asked for queued keys on every 64th poll
	.set PLAT_IDLE_MAX, 32		# empty PLAT_CHAVAIL polls in a row before one tick of sleep

	.text

# ---------------------------------------------------------------- PLAT_INIT
	.globl PLAT_INIT
PLAT_INIT:
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl $plat_con_name
	call _qd_open
	addl $4,%esp
	movl %eax,plat_con
	pushl $0			# QD_CON_* flags: 0 = raw, no echo
	pushl plat_con
	call _qd_con_mode
	addl $8,%esp
	popl %edx
	popl %ecx
	popl %eax
	ret

# ---------------------------------------------------------------- plat_conwrite (internal)
# ECX = count, EDX = buffer. Writes all of it to the console, in chunks of QD_WRITE_MAX.
# Clobbers EAX, ECX, EDX; preserves EBX, ESI, EDI.
plat_conwrite:
	pushl %ebx
	pushl %esi
	movl %edx,%esi			# next byte to write
	movl %ecx,%ebx			# bytes still to write
.Lcw_loop:
	testl %ebx,%ebx
	jz .Lcw_done
	movl %ebx,%eax
	cmpl $QD_WRITE_MAX,%eax
	jbe .Lcw_chunk
	movl $QD_WRITE_MAX,%eax
.Lcw_chunk:
	pushl %eax			# count
	pushl %esi			# buffer
	pushl plat_con			# channel
	call _qd_write
	addl $12,%esp
	testl %eax,%eax
	jle .Lcw_done			# error or no progress: the rest is dropped
	addl %eax,%esi
	subl %eax,%ebx
	jmp .Lcw_loop
.Lcw_done:
	popl %esi
	popl %ebx
	ret

# ---------------------------------------------------------------- PLAT_FLUSH
	.globl PLAT_FLUSH
PLAT_FLUSH:
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	movl plat_olen,%ecx
	testl %ecx,%ecx
	jz .Lfl_done
	movl $plat_obuf,%edx
	call plat_conwrite
	movl $0,plat_olen
.Lfl_done:
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

# ---------------------------------------------------------------- PLAT_CHOUT
# AL = character. The buffer is written out when it is full.
	.globl PLAT_CHOUT
PLAT_CHOUT:
	movl $0,plat_idle
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	movl plat_olen,%ecx
	cmpl $PLAT_OBSIZE,%ecx
	jb .Lco_store
	call PLAT_FLUSH			# preserves all registers
	xorl %ecx,%ecx
	movb 12(%esp),%al		# the character, from the saved EAX
.Lco_store:
	movl $plat_obuf,%ebx
	movb %al,(%ebx,%ecx,1)
	incl %ecx
	movl %ecx,plat_olen
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	ret

# ---------------------------------------------------------------- PLAT_CHIN
# Returns AL = next character with CF=0, or AL=0 with CF=1 at end of input. qd_con_getc sleeps in
# the kernel until a key arrives. The console's raw mode delivers Enter as CR, which is the BASIC
# line terminator, so no translation is needed.
	.globl PLAT_CHIN
PLAT_CHIN:
	pushl %ebx
	pushl %ecx
	pushl %edx
	call PLAT_FLUSH			# the prompt must be visible before we wait
	pushl plat_con
	call _qd_con_getc
	addl $4,%esp
	cmpl $0,%eax
	jl .Lci_eof
	clc
	jmp .Lci_out
.Lci_eof:
	xorl %eax,%eax
	stc
.Lci_out:
	popl %edx
	popl %ecx
	popl %ebx
	ret

# ---------------------------------------------------------------- PLAT_CHAVAIL
# ZF=0: a key is queued (PLAT_CHIN will not wait). ZF=1: none (or the query failed).
	.globl PLAT_CHAVAIL
PLAT_CHAVAIL:
	pushl %eax
	pushl %ecx
	pushl %edx
	# The kernel is asked only on every PLAT_POLL_MASK+1'th poll. Keys stay queued in the kernel, so
	# a key is seen a few polls later. Repeated console-query traps in a row end the job (see NOTES).
	incl plat_poll
	testl $PLAT_POLL_MASK,plat_poll
	jnz .Lav_idle
	pushl plat_con
	call _qd_con_queued
	addl $4,%esp
	cmpl $0,%eax
	jg .Lav_yes
.Lav_idle:
	# Nothing queued. After a run of empty polls with no output in between, the program is idle: show
	# its output and give the CPU back for one tick. Output is not written on every poll (each write is
	# a kernel call), and a busy loop of kernel calls is not a wait the kernel expects (the R13B
	# SuperBASIC port blocks in qd_con_getkey instead).
	incl plat_idle
	cmpl $PLAT_IDLE_MAX,plat_idle
	jb .Lav_none
	movl $0,plat_idle
	call PLAT_FLUSH			# preserves all registers
	pushl $1			# ticks
	call _qd_yield
	addl $4,%esp
.Lav_none:
	xorl %eax,%eax			# ZF=1: not ready
	jmp .Lav_out
.Lav_yes:
	movl $1,%eax
	testl %eax,%eax			# ZF=0: ready
.Lav_out:
	popl %edx
	popl %ecx
	popl %eax
	ret

# ---------------------------------------------------------------- PLAT_EXIT
# EAX = exit code. Flushes, then qd_exit records the status and unwinds to crt0 (ends the job).
	.globl PLAT_EXIT
PLAT_EXIT:
	call PLAT_FLUSH			# preserves EAX
	pushl %eax
	call _qd_exit
.Lex_hang:
	jmp .Lex_hang

# ---------------------------------------------------------------- plat_mkname (internal)
# EBX = zero-terminated program file name. Builds "WIN1_" + upper case name in plat_qname.
# CF=0 on success; CF=1 with EAX = 2 (not found) for an empty name or one longer than 12 characters.
# Clobbers EAX; preserves EBX, ECX, EDX, ESI.
plat_mkname:
	pushl %ecx
	pushl %esi
	movb $87,plat_qname+0		# W
	movb $73,plat_qname+1		# I
	movb $78,plat_qname+2		# N
	movb $49,plat_qname+3		# 1
	movb $95,plat_qname+4		# _
	movl %ebx,%esi
	xorl %ecx,%ecx
.Lmk_loop:
	movb (%esi),%al
	testb %al,%al
	jz .Lmk_end
	cmpb $97,%al			# a
	jb .Lmk_store
	cmpb $122,%al			# z
	ja .Lmk_store
	subb $32,%al
.Lmk_store:
	cmpl $12,%ecx
	jae .Lmk_bad
	movb %al,plat_qname+5(%ecx)
	incl %ecx
	incl %esi
	jmp .Lmk_loop
.Lmk_end:
	testl %ecx,%ecx
	jz .Lmk_bad
	movb $0,plat_qname+5(%ecx)
	clc
	jmp .Lmk_out
.Lmk_bad:
	movl $2,%eax			# ERROR_FILE_NOT_FOUND
	stc
.Lmk_out:
	popl %esi
	popl %ecx
	ret

# ---------------------------------------------------------------- PLAT_FOPEN
	.globl PLAT_FOPEN
	.globl PLAT_FSIZE
	.globl PLAT_FREAD
	.globl PLAT_FWRITE
	.globl PLAT_FCLOSE
PLAT_FOPEN:
	pushl %ecx
	pushl %edx
	movl %edx,plat_fmode
	call plat_mkname
	jc .Lfo_fail
	cmpl $0,plat_fmode
	jne .Lfo_write
	movl $1,%eax			# QD_O_READ
	jmp .Lfo_go
.Lfo_write:
	movl $14,%eax			# QD_O_WRITE | QD_O_CREATE | QD_O_TRUNC
.Lfo_go:
	pushl %eax			# flags
	pushl $plat_qname		# name
	call _qd_open_ex
	addl $8,%esp
	cmpl $0,%eax
	jle .Lfo_err
	popl %edx
	popl %ecx
	clc
	ret
.Lfo_err:
	cmpl $0,plat_fmode
	je .Lfo_rderr
	# A write open that fails: the kernel's negative code is mapped (see plat_wrerr).
	call plat_wrerr
	jmp .Lfo_fail
.Lfo_rderr:
	movl $2,%eax			# a file that cannot be opened for reading is not found
.Lfo_fail:
	popl %edx
	popl %ecx
	stc
	ret

# ---------------------------------------------------------------- PLAT_FSIZE
PLAT_FSIZE:
	pushl %ecx
	pushl %edx
	pushl %ebx
	call _qd_fsize
	addl $4,%esp
	cmpl $0,%eax
	jl .Lfs_err
	popl %edx
	popl %ecx
	clc
	ret
.Lfs_err:
	movl $5,%eax
	popl %edx
	popl %ecx
	stc
	ret

# ---------------------------------------------------------------- plat_xfer (internal)
# EBX = channel, ECX = count, EDX = buffer. plat_fop = 0 reads, 1 writes. Loops over qd_read or
# qd_write in chunks of QD_WRITE_MAX until all COUNT bytes move. CF=0 when they all did; CF=1 with
# EAX = 38 (read reached the end), 112 (write moved no bytes), 19 (write-protected), 4 (root directory full),
# or 5 (other I/O error). See plat_wrerr.
# Clobbers EAX, ECX, EDX, ESI, EDI; preserves EBX.
plat_xfer:
	movl %ebx,plat_fh
	movl %edx,%esi
	movl %ecx,%edi
.Lxf_loop:
	testl %edi,%edi
	jz .Lxf_ok
	movl %edi,%eax
	cmpl $QD_WRITE_MAX,%eax
	jbe .Lxf_chunk
	movl $QD_WRITE_MAX,%eax
.Lxf_chunk:
	pushl %eax			# count
	pushl %esi			# buffer
	pushl plat_fh			# channel
	cmpl $0,plat_fop
	jne .Lxf_w
	call _qd_read
	jmp .Lxf_called
.Lxf_w:
	call _qd_write
.Lxf_called:
	addl $12,%esp
	cmpl $0,%eax
	jl .Lxf_err
	je .Lxf_short
	addl %eax,%esi
	subl %eax,%edi
	jmp .Lxf_loop
.Lxf_ok:
	clc
	ret
.Lxf_short:
	movl $38,%eax			# ERROR_HANDLE_EOF
	cmpl $0,plat_fop
	je .Lxf_fail
	movl $112,%eax			# ERROR_DISK_FULL
	jmp .Lxf_fail
.Lxf_err:
	cmpl $0,plat_fop
	je .Lxf_ioerr
	call plat_wrerr			# a write: the kernel's code is mapped (write-protect, disk full, ...)
	jmp .Lxf_fail
.Lxf_ioerr:
	movl $5,%eax
.Lxf_fail:
	stc
	ret

# plat_wrerr: EAX = a negative kernel file result from a write or a write open. Returns EAX = the platform
# error code: 19 write-protected (kernel -80, read-only open included), 112 disk full (-81), 4 root directory
# full (-82); anything else (file open elsewhere, sector write failed, ...) is 5, an I/O error.
# Clobbers EAX only.
plat_wrerr:
	cmpl $-80,%eax
	je .Lwe_wp
	cmpl $-81,%eax
	je .Lwe_full
	cmpl $-82,%eax
	je .Lwe_dir
	movl $5,%eax
	ret
.Lwe_wp:
	movl $19,%eax			# PLAT_ERR_WPROT
	ret
.Lwe_full:
	movl $112,%eax			# PLAT_ERR_DISKFULL
	ret
.Lwe_dir:
	movl $4,%eax			# PLAT_ERR_DIRFULL
	ret

PLAT_FREAD:
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	movl $0,plat_fop
	call plat_xfer
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	ret

PLAT_FWRITE:
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	movl $1,plat_fop
	call plat_xfer
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	ret

# ---------------------------------------------------------------- PLAT_FCLOSE
PLAT_FCLOSE:
	pushl %ebx
	call _qd_close
	addl $4,%esp
	ret

# ---------------------------------------------------------------- directory listing (FILES)
# PLAT_DIROPEN : opens the WIN1_ root directory listing. CF=0 with the listing open, or CF=1 on error.
#                The only directory on the volume is the root (the kernel lists it with qd_dir_open).
# PLAT_DIRNEXT : CF=0 with EBX = the next entry's name (NUL-terminated, at most 12 characters, upper case
#                as stored), or CF=1 at the end. Clobbers EAX, ECX, EDX. EBX, ESI, EDI and EBP are preserved.
# PLAT_DIRCLOSE: closes the listing. Clobbers EAX, ECX, EDX.
	.globl PLAT_DIROPEN
	.globl PLAT_DIRNEXT
	.globl PLAT_DIRCLOSE
PLAT_DIROPEN:
	call _qd_dir_open
	cmpl $0,%eax
	jle .Ldo_fail
	movl %eax,plat_dirch
	clc
	ret
.Ldo_fail:
	movl $2,%eax			# no listing (file not found)
	stc
	ret

PLAT_DIRNEXT:
	pushl $plat_dirent		# struct qd_dirent *
	pushl plat_dirch
	call _qd_dir_next
	addl $8,%esp
	cmpl $1,%eax
	jne .Ldn_end
	movl $plat_dirent,%ebx		# name is the first field of the record
	clc
	ret
.Ldn_end:
	stc
	ret

PLAT_DIRCLOSE:
	pushl plat_dirch
	call _qd_close
	addl $4,%esp
	ret

# ---------------------------------------------------------------- data
	.data
plat_con_name:
	.byte 67,79,78,0		# "CON"

	.lcomm plat_con,4
	.lcomm plat_olen,4
	.lcomm plat_obuf,4096
	.lcomm plat_fmode,4
	.lcomm plat_fh,4
	.lcomm plat_fop,4
	.lcomm plat_qname,24
	.lcomm plat_dirch,4
	.lcomm plat_dirent,20
	.lcomm plat_idle,4
	.lcomm plat_poll,4
