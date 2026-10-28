.nr _- 0
.PH '''
.SA 1
.nr Cl 5
.ds HP 14 14 14 14 14 14 14
.ds HF 3 3 3 3 3 3 3
.nr Hb 7
.nr Hs 7
.nr Pt 1
.tr \(is'
.tr \(if`
.tr \(pd"

.P 1
.P 1
.ds St GNU\ Binary\ Utilities
.OH '\\*(St''%'
.EH '%''\\*(St'
.ce
The\ GNU\ Binary\ Utilities
.ce
Version\ 1.90
.sp 1
.ce
October\ 1991
.ce
Roland\ H.\ Pesch
.ce
Cygnus\ Support
.bp
.P 1
.P 1
Copyright \(co 1991 Free Software Foundation, Inc.
.P 1
Permission is granted to make and distribute verbatim copies of
this manual provided the copyright notice and this permission notice
are preserved on all copies.
.P 1
Permission is granted to copy and distribute modified versions of this
manual under the conditions for verbatim copying, provided also that
the entire resulting derived work is distributed under the terms of a
permission notice identical to this one.
.P 1
Permission is granted to copy and distribute translations of this manual
into another language, under the above conditions for modified versions.
.bp
.P 1
.P 1
This brief manual contains preliminary documentation for the GNU binary
utilities (collectively version 1.90): 
.VL 5 0
.LI \fR\&\f(CWar\fR\ 
Create, modify, and extract from archives
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWnm\fR\ 
List symbols from object files
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWobjdump\fR\ 
Display information from object files
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWranlib\fR\ 
Generate index to archive contents
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWsize\fR\ 
List section sizes and total size
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWstrip\fR\ 
Discard symbols
.LE
.P 1
.P 1
.P 1
.P 1
.iX "k ar"
.DS 1
\&\fR\&\f(CW
  ar [-]\fIp\fR\&\f(CW\fImod\fR\&\f(CW [ \fImembername\fR\&\f(CW ] \fIarchive\fR\&\f(CW \fIfiles\fR\&\f(CW\&...
.DE
\&\fR
.P 1
The GNU \fR\&\f(CWar\fR program creates, modifies, and extracts from
archives.  An \fIarchive\fR is a single file holding a collection of
other files in a structure that makes it possible to retrieve
the original individual files (called \fImembers\fR of the archive).
.P 1
The original files' contents, mode (permissions), timestamp, owner, and
group are preserved in the archive, and may be reconstituted on
extraction.  
.P 1
GNU \fR\&\f(CWar\fR can maintain archives whose members have names of any
length; however, depending on how \fR\&\f(CWar\fR is configured on your
system, a limit on member-name length may be imposed (for compatibility
with archive formats maintained with other tools).  If it exists, the
limit is often 15 characters (typical of formats related to a.out) or 16
characters (typical of formats related to coff).
.P 1
are most often used as \fIlibraries\fR holding commonly needed
subroutines.
.P 1
object modules in the archive when you specify the modifier \fR\&\f(CW\(ifs\(is\fR.
Once created, this index is updated in the archive whenever \fR\&\f(CWar\fR
makes a change to its contents (save for the \fR\&\f(CW\(ifq\(is\fR update operation).
An archive with such an index speeds up linking to the library, and
allows routines in the library to call each other without regard to
their placement in the archive.
.P 1
You may use \fR\&\f(CW\(ifnm -s\(is\fR or \fR\&\f(CW\(ifnm +print-armap\(is\fR to list this index
table.  If an archive lacks the table, another form of \fR\&\f(CWar\fR called
.P 1
keyletter specifying the \fIoperation\fR (optionally accompanied by other
keyletters specifying \fImodifiers\fR), and the archive name to act on.
.P 1
Most operations can also accept further \fIfiles\fR arguments,
specifying particular files to operate on.
.P 1
GNU \fR\&\f(CWar\fR allows you to mix the operation code \fIp\fR and modifier
flags \fImod\fR in any order, within the first command-line argument.
.P 1
If you wish, you may begin the first command-line argument with a
dash.
.P 1
The \fIp\fR keyletter specifies what operation to execute; it may be
any of the following, but you must specify only one of them:
.P 1
.VL 5 0
.LI \fR\&\f(CWd\fR\ 
\&\fIDelete\fR modules from the archive.  Specify the names of modules to
be deleted as \fIfiles\fR; the archive is untouched if you
specify no files to delete.
.P 1
.ti -\n(Pin
If you specify the \fR\&\f(CW\(ifv\(is\fR modifier, \fR\&\f(CWar\fR will list each module
as it is deleted.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWm\fR\ 
Use this operation to \fImove\fR members in an archive.
.P 1
.ti -\n(Pin
The ordering of members in an archive can make a difference in how
programs are linked using the library, if a symbol is defined in more
than one member.  
.P 1
.ti -\n(Pin
If no modifiers are used with \fR\&\f(CWm\fR, any members you name in the
\&\fIfiles\fR arguments are moved to the \fIend\fR of the archive;
you can use the \fR\&\f(CW\(ifa\(is\fR, \fR\&\f(CW\(ifb\(is\fR, or \fR\&\f(CW\(ifi\(is\fR modifiers to move them to a
specified place instead.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWp\fR\ 
\&\fIPrint\fR the specified members of the archive, to the standard
output file.  If the \fR\&\f(CW\(ifv\(is\fR modifier is specified, show the member
name before copying its contents to standard output.
.P 1
.ti -\n(Pin
If you specify no \fIfiles\fR, all the files in the archive are printed.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWq\fR\ 
\&\fIQuick append\fR; add \fIfiles\fR to the end of \fIarchive\fR,
without checking for replacement.  
.P 1
.ti -\n(Pin
The modifiers \fR\&\f(CW\(ifa\(is\fR, \fR\&\f(CW\(ifb\(is\fR, and \fR\&\f(CW\(ifi\(is\fR do \fInot\fR affect this
operation; new members are always placed at the end of the archive.
.P 1
.ti -\n(Pin
The modifier \fR\&\f(CW\(ifv\(is\fR makes \fR\&\f(CWar\fR list each file as it is appended.
.P 1
.ti -\n(Pin
Since the point of this operation is speed, the archive's symbol table
index is not updated, even if it already existed; you can use \fR\&\f(CW\(ifar s\(is\fR or
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWr\fR\ 
Insert \fIfiles\fR into \fIarchive\fR (with \fIreplacement\fR). This
operation differs from \fR\&\f(CW\(ifq\(is\fR in that any previously existing members
are deleted if their names match those being added.
.P 1
.ti -\n(Pin
If one of the files named in \fIfiles\fR doesn't exist, \fR\&\f(CWar\fR
displays an error message, and leaves undisturbed any existing members
of the archive matching that name.
.P 1
.ti -\n(Pin
By default, new members are added at the end of the file; but you may
use one of the modifiers \fR\&\f(CW\(ifa\(is\fR, \fR\&\f(CW\(ifb\(is\fR, or \fR\&\f(CW\(ifi\(is\fR to request
placement relative to some existing member.
.P 1
.ti -\n(Pin
The modifier \fR\&\f(CW\(ifv\(is\fR used with this operation elicits a line of
output for each file inserted, along with one of the letters \fR\&\f(CW\(ifa\(is\fR or
\&\fR\&\f(CW\(ifr\(is\fR to indicate whether the file was appended (no old member
deleted) or replaced.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWt\fR\ 
Display a \fItable\fR listing the contents of \fIarchive\fR, or those
of the files listed in \fIfiles\fR that are present in the
archive.  Normally only the member name is shown; if you also want to
see the modes (permissions), timestamp, owner, group, and size, you can
request that by also specifying the \fR\&\f(CW\(ifv\(is\fR modifier.
.P 1
.ti -\n(Pin
If you do not specify any \fIfiles\fR, all files in the archive
are listed.
.P 1
.ti -\n(Pin
If there is more than one file with the same name (say, \fR\&\f(CW\(iffie\(is\fR) in
an archive (say \fR\&\f(CW\(ifb.a\(is\fR), \fR\&\f(CW\(ifar t b.a fie\(is\fR will list only the
first instance; to see them all, you must ask for a complete
listing\(emin our example, \fR\&\f(CW\(ifar t b.a\(is\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWx\fR\ 
\&\fIExtract\fR members (named \fIfiles\fR) from the archive.  You can
use the \fR\&\f(CW\(ifv\(is\fR modifier with this operation, to request that
.P 1
.ti -\n(Pin
If you do not specify any \fIfiles\fR, all files in the archive
are extracted.
.P 1
.ti -\n(Pin
.LE
.P 1
A number of modifiers (\fImod\fR) may immediately follow the \fIp\fR
keyletter, to specify variations on an operation's behavior:
.P 1
.VL 5 0
.LI \fR\&\f(CWa\fR\ 
Add new files \fIafter\fR an existing member of the
archive.  If you use the modifier \fR\&\f(CWa\fR, the name of an existing archive
member must be present as the \fImembername\fR argument, before the
\&\fIarchive\fR specification.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWb\fR\ 
Add new files \fIbefore\fR an existing member of the
archive.  If you use the modifier \fR\&\f(CWb\fR, the name of an existing archive
member must be present as the \fImembername\fR argument, before the
\&\fIarchive\fR specification.  (same as \fR\&\f(CW\(ifi\(is\fR).
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWc\fR\ 
\&\fICreate\fR the archive.  The specified \fIarchive\fR is always
created if it didn't exist, when you request an update.  But a warning is
issued unless you specify in advance that you expect to create it, by
using this modifier.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWi\fR\ 
Insert new files \fIbefore\fR an existing member of the
archive.  If you use the modifier \fR\&\f(CWi\fR, the name of an existing archive
member must be present as the \fImembername\fR argument, before the
\&\fIarchive\fR specification.  (same as \fR\&\f(CW\(ifb\(is\fR).
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWl\fR\ 
This modifier is accepted but not used.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWo\fR\ 
Preserve the \fIoriginal\fR dates of members when extracting them.  If
you do not specify this modifier, files extracted from the archive
will be stamped with the time of extraction.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWs\fR\ 
Write an object-file index into the archive, or update an existing one,
even if no other change is made to the archive.  You may use this modifier
flag either with any operation, or alone.  Running \fR\&\f(CW\(ifar s\(is\fR on an
archive is equivalent to running \fR\&\f(CW\(ifranlib\(is\fR on it.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWu\fR\ 
Normally, \fR\&\f(CWar r\fR\&... inserts all files
listed into the archive.  If you would like to insert \fIonly\fR those
of the files you list that are newer than existing members of the same
names, use this modifier.  The \fR\&\f(CW\(ifu\(is\fR modifier is allowed only for the
operation \fR\&\f(CW\(ifr\(is\fR (replace).  In particular, the combination \fR\&\f(CW\(ifqu\(is\fR is
not allowed, since checking the timestamps would lose any speed
advantage from the operation \fR\&\f(CW\(ifq\(is\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWv\fR\ 
This modifier requests the \fIverbose\fR version of an operation.  Many
operations display additional information, such as filenames processed,
when the modifier \fR\&\f(CW\(ifv\(is\fR is appended.
.P 1
.ti -\n(Pin
.LE
.P 1
.iX "k ld"
The GNU linker \fR\&\f(CWld\fR is now described in a separate manual.
See section  GLD: the GNU linker.
.P 1
.iX "k nm"
.P 1
.DS 1
\&\fR\&\f(CW
  nm [ -a | +debug-syms ]  [ -g | +extern-only ]
      [ -s | +print-armap ]  [ -o | +print-file-name ]  
      [ -n | +numeric-sort ]  [ -p | +no-sort ]
      [ -r | +reverse-sort ]  [ -u | +undefined-only ]  
      [ +target \fIbfdname\fR\&\f(CW ]
      [ \fIobjfiles\fR\&\f(CW\&... ]
.DE
\&\fR
.P 1
GNU \fR\&\f(CWnm\fR will list the symbols from object files \fIobjfiles\fR.
.P 1
The long and short forms of options, shown here as alternatives, are
equivalent.
.P 1
.VL 5 0
.LI \fR\&\f(CW\fIobjfiles\fR\&\f(CW\&...\fR\ 
.iX "k a.out"
Object files whose symbols are to be listed.  If no object files are
listed as arguments, \fR\&\f(CWnm\fR assumes \fR\&\f(CW\(ifa.out\(is\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-a\fR\ 
.LI \fR\&\f(CW+debug-syms\ \fR\ 
Display debugger-only symbols; normally these are not listed.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-g\fR\ 
.LI \fR\&\f(CW+extern-only\ \fR\ 
Display only external symbols.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-p\fR\ 
.LI \fR\&\f(CW+no-sort\ \fR\ 
Don't bother to sort the symbols in any order; just print them in the
order encountered.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-n\fR\ 
.LI \fR\&\f(CW+numeric-sort\ \fR\ 
Sort symbols numerically by their addresses, not alphabetically by their
names. 
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-s\fR\ 
.LI \fR\&\f(CW+print-armap\fR\ 
When listing symbols from archive members, include the index: a mapping
(stored in the archive by \fR\&\f(CWar\fR or \fR\&\f(CWranlib\fR) of what modules
contain definitions for what names.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-o\fR\ 
.LI \fR\&\f(CW+print-file-name\ \fR\ 
Precede each symbol by the name of the input file where it was found,
rather than identifying the input file once only before all of its
symbols. 
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-r\fR\ 
.LI \fR\&\f(CW+reverse-sort\ \fR\ 
Reverse the sense of the sort (whether numeric or alphabetic); let the
last come first.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW+target\ \fIbfdname\fR\&\f(CW\fR\ 
Specify an object code format other than your system's default format.
See section objdump, for information on listing available formats.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-u\fR\ 
.LI \fR\&\f(CW+undefined-only\ \fR\ 
Display only undefined symbols (those external to each object file).
.P 1
.ti -\n(Pin
.LE
.P 1
.P 1
.iX "k objdump"
.P 1
.DS 1
\&\fR\&\f(CW
  objdump [ -a ]  [ -b \fIbfdname\fR\&\f(CW ]  [ -d ]  [ -f ]
          [ -h | +header ]  [ -i ]  [ -j \fIsection\fR\&\f(CW ]  [ -l ]
          [ -m \fImachine\fR\&\f(CW ]  [ -r | +reloc ]  [ -s ]
          [ -t | +syms ]  [ -x ]
          \fIobjfiles\fR\&\f(CW\&...
.DE
\&\fR
.P 1
The options control what particular information to display.  This
information is mostly useful to programmers who are working on the
compilation tools, as opposed to programmers who just want their
program to compile and work.
.P 1
The long and short forms of options, shown here as alternatives, are
equivalent.
.P 1
.VL 5 0
.LI \fR\&\f(CW\fIobjfiles\fR\&\f(CW\&...\fR\ 
The object files to be examined.  When you specify archives,
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-a\fR\ 
If any files from \fIobjfiles\fR are archives, display the archive
header information (in a format similar to \fR\&\f(CW\(ifls -l\(is\fR).  Besides the
information you could list with \fR\&\f(CW\(ifar tv\(is\fR, \fR\&\f(CW\(ifobjdump -a\(is\fR shows
the object file format of each archive member.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-b\ \fIbfdname\fR\&\f(CW\fR\ 
You can specify a particular object-code format for your object files as
\&\fIbfdname\fR.  This may not be necessary; \fIobjdump\fR can
automatically recognize many formats.  For example,
.DS 1
\&\fR\&\f(CW
objdump -b oasys -m vax -h fu.o
.DE
\&\fRDisplays summary information from the section headers (\fR\&\f(CW\(if-h\(is\fR) of
\&\fR\&\f(CW\(iffu.o\(is\fR, which is explicitly identified (\fR\&\f(CW\(if-m\(is\fR) as a Vax object
file in the format produced by Oasys compilers.  You can list the
formats available with the \fR\&\f(CW\(if-i\(is\fR option.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-d\fR\ 
Disassemble.  Display the assembler mnemonics for the machine
instructions from \fIobjfiles\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-f\fR\ 
File header.  Display summary information from the overall header of
each file in \fIobjfiles\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-h\fR\ 
.LI \fR\&\f(CW+header\fR\ 
Header.  Display summary information from the section headers of the
object file.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-i\fR\ 
Display a list showing all architectures and object formats available
for specification with \fR\&\f(CW-b\fR or \fR\&\f(CW-m\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-j\ \fIname\fR\&\f(CW\fR\ 
Display information only for section \fIname\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-l\fR\ 
Label the display (using debugging information) with the source filename
and line numbers corresponding to the object code shown.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-m\ \fImachine\fR\&\f(CW\fR\ 
Specify the object files \fIobjfiles\fR are for architecture
\&\fImachine\fR.  You can list available architectures using the \fR\&\f(CW\(if-i\(is\fR
option. 
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-r\fR\ 
.LI \fR\&\f(CW+reloc\fR\ 
Relocation.  Print the relocation entries of the file.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-s\fR\ 
Display the full contents of any sections requested.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-t\fR\ 
.LI \fR\&\f(CW+syms\fR\ 
Symbol Table.  Print the symbol table entries of the file.
This is similar to the information provided by the \fR\&\f(CW\(ifnm\(is\fR program.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-x\fR\ 
Display all available header information, including the symbol table and
relocation entries.  Using \fR\&\f(CW\(if-x\(is\fR is equivalent to specifying all of
\&\fR\&\f(CW\(if-a -f -h -r -t\(is\fR.
.P 1
.ti -\n(Pin
.LE
.P 1
.P 1
.iX "k ranlib"
.P 1
.DS 1
\&\fR\&\f(CW
  ranlib \fIarchive\fR\&\f(CW
.DE
\&\fR
.P 1
stores it in the archive.  The index lists each symbol defined by a
member of an archive that is a relocatable object file.  
.P 1
You may use \fR\&\f(CW\(ifnm -s\(is\fR or \fR\&\f(CW\(ifnm +print-armap\(is\fR to list this index.
.P 1
An archive with such an index speeds up linking to the library, and
allows routines in the library to call each other without regard to
their placement in the archive.
.P 1
The GNU \fR\&\f(CWranlib\fR program is another form of GNU \fR\&\f(CWar\fR; running
See section ar.
.P 1
.P 1
.iX "k size"
.P 1
.DS 1
\&\fR\&\f(CW
  size [ -A | -B | +format \fIcompatibility\fR\&\f(CW ]
       [ +help ]  [ -d | -o | -x | +radix \fInumber\fR\&\f(CW ]
       [ +target \fIbfdname\fR\&\f(CW ]  [ -V | +version ]  
       \fIobjfiles\fR\&\f(CW\&...
.DE
\&\fR
.P 1
The GNU \fR\&\f(CWsize\fR utility lists the section sizes\(emand the total
size\(emfor each of the object files \fIobjfiles\fR in its argument list.
By default, one line of output is generated for each object file or each
module in an archive.
.P 1
The command line options have the following meanings:
.VL 5 0
.LI \fR\&\f(CW\fIobjfiles\fR\&\f(CW\&...\fR\ 
The object files to be examined.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-A\fR\ 
.LI \fR\&\f(CW-B\fR\ 
.LI \fR\&\f(CW+format\ \fIcompatibility\fR\&\f(CW\fR\ 
Using one of these options, you can choose whether the output from GNU
or \fR\&\f(CW\(if+format sysv\(is\fR), or Berkeley \fR\&\f(CWsize\fR (using \fR\&\f(CW\(if-B\(is\fR, or
\&\fR\&\f(CW\(if+format berkeley\(is\fR).  The default is the one-line format similar to
Berkeley's.  
.P 1
.ti -\n(Pin
Here is an example of the Berkeley (default) format of output from
.DS 1
\&\fR\&\f(CW
 eg$  size +format Berkeley ranlib size
text    data    bss     dec     hex     filename
294880  81920   11592   388392  5ed28   ranlib
294880  81920   11888   388688  5ee50   size
.DE
\&\fR
.P 1
.ti -\n(Pin
This is the same data, but displayed closer to System V conventions:
.P 1
.ti -\n(Pin
.DS 1
\&\fR\&\f(CW
 eg$  size +format SysV ranlib size
ranlib  :
section         size         addr
\&\&.text         294880         8192       
\&\&.data          81920       303104       
\&\&.bss           11592       385024       
Total         388392    
\&\&
\&\&
size  :
section         size         addr
\&\&.text         294880         8192       
\&\&.data          81920       303104       
\&\&.bss           11888       385024       
Total         388688    
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW+help\fR\ 
Show a summary of acceptable arguments and options.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-d\fR\ 
.LI \fR\&\f(CW-o\fR\ 
.LI \fR\&\f(CW-x\fR\ 
.LI \fR\&\f(CW+radix\ \fInumber\fR\&\f(CW\fR\ 
Using one of these options, you can control whether the size of each
section is given in decimal (\fR\&\f(CW\(if-d\(is\fR, or \fR\&\f(CW\(if+radix 10\(is\fR); octal
(\fR\&\f(CW\(if-o\(is\fR, or \fR\&\f(CW\(if+radix 8\(is\fR); or hexadecimal (\fR\&\f(CW\(if-x\(is\fR, or
\&\fR\&\f(CW\(if+radix 16\(is\fR).  In \fR\&\f(CW\(if+radix \fInumber\fR\&\f(CW\(is\fR, only the three
values (8, 10, 16) are supported.  The total size is always given in two
radices; decimal and hexadecimal for \fR\&\f(CW\(if-d\(is\fR or \fR\&\f(CW\(if-x\(is\fR output, or
octal and hexadecimal if you're using \fR\&\f(CW\(if-o\(is\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW+target\ \fIbfdname\fR\&\f(CW\fR\ 
You can specify a particular object-code format for \fIobjfiles\fR as
\&\fIbfdname\fR.  This may not be necessary; \fIsize\fR can
automatically recognize many formats.  See section objdump, for information
on listing available formats.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-V\fR\ 
.LI \fR\&\f(CW+version\fR\ 
Display version number information on \fR\&\f(CWsize\fR itself.
.P 1
.ti -\n(Pin
.LE
.P 1
.P 1
.iX "k strip"
.P 1
.DS 1
\&\fR\&\f(CW
  strip [ -s | +strip-all ]  [ -g | -S | +strip-debug ]
         [ -x | +discard-all ]  [ -X | +discard-locals ]
         [ -T \fIbfdname\fR\&\f(CW ]
         \fIobjfiles\fR\&\f(CW\&...
.DE
\&\fR
.P 1
GNU \fR\&\f(CWstrip\fR will discard all symbols from object files
\&\fIobjfiles\fR, if no options are specified; or only certain symbols,
depending on its command-line options.
.P 1
.P 1
.DS I F 5
.nh
\&\fIWARNING:\fR \fR\&\f(CWstrip\fR modifies the files named in its argument,
rather than writing modified copies under different names.
.DE
.P 1
The long and short forms of options, shown here as alternatives, are
equivalent.
.P 1
.VL 5 0
.LI \fR\&\f(CW-s\fR\ 
.LI \fR\&\f(CW+strip-all\ \fR\ 
This is the default case: strip all symbol entries from \fIobjfiles\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-g\fR\ 
.LI \fR\&\f(CW-S\fR\ 
.LI \fR\&\f(CW+strip-debug\ \fR\ 
Discard only debugging symbol information from \fIobjfiles\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-x\fR\ 
.LI \fR\&\f(CW+discard-all\ \fR\ 
Discard all symbols local to each file in \fIobjfiles\fR.
\&\fIWARNING:\fR Note that \fR\&\f(CW+discard-all\fR discards only \fIlocal\fR
symbols, in spite of its name.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-X\fR\ 
.LI \fR\&\f(CW+discard-locals\ \fR\ 
Discard local symbols starting with \fR\&\f(CW\(ifL\(is\fR from each file in
\&\fIobjfiles\fR.  (Some compilers produce internally-used symbols that
begin with \fR\&\f(CW\(ifL\(is\fR.)
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-T\ \fIbfdname\fR\&\f(CW\fR\ 
You can specify a particular object-code format \fIbfdname\fR for
\&\fIobjfiles\fR.  This may not be necessary; \fIstrip\fR can automatically
recognize many formats.  See section objdump, for information on listing
available formats.
.LE
.P 1
.bp
.sp .5
.HU Index
.sp .5
\&\fR
.P 1
.P 1
