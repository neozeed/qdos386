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
.ds St GLD,\ the\ GNU\ linker
.OH '\\*(St''%'
.EH '%''\\*(St'
.ce
gld
.ce
The\ GNU\ linker
.sp 1
.ce
Second\ Edition\(em\fR\&\f(CWgld\fR\ version\ 2.0
.ce
April\ 1991
.ce
Steve\ Chamberlain\ and\ Roland\ Pesch
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
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
their data and ties up symbol references. Often the last step in
building a new compiled program to run is a call to \fR\&\f(CWgld\fR.
.P 1
a superset of AT&T's Link Editor Command Language syntax,
to provide explicit and total control over the linking process.
.P 1
This version of \fR\&\f(CWgld\fR uses the general purpose BFD libraries
to operate on object files. This allows \fR\&\f(CWgld\fR to read, combine, and
write object files in many different formats\(emfor example, COFF or
available kind of object file.  See section BFD for a list of formats
supported on various architectures.
.P 1
Aside from its flexibility, the GNU linker is more helpful than other
linkers in providing diagnostic information.  Many linkers abandon
execution immediately upon encountering an error; whenever possible,
(or, in some cases, to get an output file in spite of the error).
.P 1
.P 1
The GNU linker \fR\&\f(CWgld\fR is meant to cover a broad range of situations,
and to be as compatible as possible with other linkers.  As a result,
you have many choices to control its behavior through the command line,
and through environment variables.
.P 1
.P 1
.sp .5
.H 2 Command\ Line\ Options
.sp .5
\&\fR
.P 1
Here is a sketch of the options you can use on the \fR\&\f(CWgld\fR command
line:
.P 1
.DS 1
\&\fR\&\f(CW
gld [-o \fIoutput\fR\&\f(CW ] \fIobjfiles\fR\&\f(CW\&...
    [ -A\fIarchitecture\fR\&\f(CW ]  [ -b \fIinput-format\fR\&\f(CW ]  [ -Bstatic ]  
    [ -c \fIcommandfile\fR\&\f(CW ]  [ -d | -dc | -dp ]  
    [ -defsym \fIsymbol\fR\&\f(CW = \fIexpression\fR\&\f(CW ]
    [ -e \fIentry\fR\&\f(CW ]  [ -F ]  [ -F \fIformat\fR\&\f(CW ]
    [ -format \fIinput-format\fR\&\f(CW ]  [ -g ]  [ -i ]
    [ -l\fIar\fR\&\f(CW ]  [ -L\fIsearchdir\fR\&\f(CW ]  [ -M | -m ]  
    [ -n ]  [ -noinhibit-exec ]  [ -R \fIfilename\fR\&\f(CW ]
    [ -r | -Ur ]  [ -S ]  [ -s ]  [ -T \fIcommandfile\fR\&\f(CW ]  
    [ -Ttext \fItextorg\fR\&\f(CW ]  [ -Tdata \fIdataorg\fR\&\f(CW ]  [ -Tbss \fIbssorg\fR\&\f(CW ]
    [ -t ]  [ -u \fIsym\fR\&\f(CW]  [-v]  [ -X ]  [ -x ] 
    [ { \fIscript\fR\&\f(CW } ] 
.DE
\&\fR
.P 1
This plethora of command-line options may seem intimidating, but in
actual practice few of them are used in any particular context.
For instance, a frequent use of \fR\&\f(CWgld\fR is to link standard Unix
object files on a standard, supported Unix system.  On such a system, to
link a file \fR\&\f(CWhello.o\fR:
.DS 1
\&\fR\&\f(CW
$ gld -o output /lib/crt0.o hello.o -lc
.DE
\&\fRThis tells \fR\&\f(CWgld\fR to produce a file called \fR\&\f(CWoutput\fR as the
result of linking the file \fR\&\f(CW/lib/crt0.o\fR with \fR\&\f(CWhello.o\fR and
the library \fR\&\f(CWlibc.a\fR which will come from the standard search
directories.
.P 1
The command-line options to \fR\&\f(CWgld\fR may be specified in any order, and
may be repeated at will.  For the most part, repeating an option with a
different argument will either have no further effect, or override prior
occurrences (those further to the left on the command line) of an
option.  
.P 1
The exceptions\(emwhich may meaningfully be used more than once\(emare
.P 1
The list of object files to be linked together, shown as \fIobjfiles\fR,
may follow, precede, or be mixed in with command-line options; save that
an \fIobjfiles\fR argument may not be placed between an option flag and
its argument.
.P 1
Usually the linker is invoked with at least one object file, but other
forms of binary input files can also be specified with \fR\&\f(CW-l\fR,
files at all are specified, the linker does not produce any output, and
issues the message \fR\&\f(CW\(ifNo input files\(is\fR.
.P 1
Option arguments must either follow the option letter without intervening
whitespace, or be given as separate arguments immediately following the
option that requires them.
.P 1
.VL 5 0
.LI \fR\&\f(CW\fIobjfiles\fR\&\f(CW\&...\fR\ 
The object files \fIobjfiles\fR to be linked.
.P 1
.ti -\n(Pin
.iX "k -A\fIarch\fR"
.LI \fR\&\f(CW-A\fIarchitecture\fR\&\f(CW\fR\ 
In the current release of \fR\&\f(CWgld\fR, this option is useful only for the
Intel 960 family of architectures.  In that \fR\&\f(CWgld\fR configuration, the
\&\fIarchitecture\fR argument is one of the two-letter names identifying
members of the 960 family; the option specifies the desired output
target, and warns of any incompatible instructions in the input files.
It also modifies the linker's search strategy for archive libraries, to
support the use of libraries specific to each particular
architecture, by including in the search loop names suffixed with the
string identifying the architecture.
.P 1
.ti -\n(Pin
For example, if your \fR\&\f(CWgld\fR command line included 
\&\! \fR\&\f(CW\(if-ACA\(is\fR
 as
well as 
\&\! \fR\&\f(CW\(if-ltry\(is\fR
, the linker would look (in its built-in search
paths, and in any paths you specify with \fR\&\f(CW-L\fR) for a library with
the names
.DS 1
\&\fR\&\f(CW
try
libtry.a
tryca
libtryca.a
.DE
\&\fRThe first two possibilities would be considered in any event; the last
two are due to the use of 
\&\! \fR\&\f(CW\(if-ACA\(is\fR
.
.P 1
.ti -\n(Pin
Future releases of \fR\&\f(CWgld\fR may support similar functionality for
other architecture families.
.P 1
.ti -\n(Pin
You can meaningfully use \fR\&\f(CW-A\fR more than once on a command line, if
an architecture family allows combination of target architectures; each
use will add another pair of name variants to search for when 
\&\! \fR\&\f(CW-l\fR
specifies a library.
.P 1
.ti -\n(Pin
.iX "k -b \fIformat\fR"
.LI \fR\&\f(CW-b\ \fIinput-format\fR\&\f(CW\fR\ 
Specify the binary format for input object files that follow this option
on the command line.  You don't usually need to specify this, as
usual format on each machine.  \fIinput-format\fR is a text string, the
name of a particular format supported by the BFD libraries.  See section BFD.
.P 1
.ti -\n(Pin
You may want to use this option if you are linking files with an unusual
binary format.  You can also use \fR\&\f(CW-b\fR to switch formats explicitly (when
linking object files of different formats), by including
particular format.  
.P 1
.ti -\n(Pin
The default format is taken from the environment variable
format from a script, using the command \fR\&\f(CWTARGET\fR.
.P 1
.ti -\n(Pin
.iX "k -Bstatic"
.LI \fR\&\f(CW-Bstatic\ \fR\ 
This flag is accepted for command-line compatibility with the SunOS linker,
but has no effect on \fR\&\f(CWgld\fR.
.P 1
.ti -\n(Pin
.iX "k -c \fIcmdfile\fR"
.LI \fR\&\f(CW-c\ \fIcommandfile\fR\&\f(CW\fR\ 
Directs \fR\&\f(CWgld\fR to read link commands from the file
\&\fIcommandfile\fR.  These commands will completely override \fR\&\f(CWgld\fR's
default link format (rather than adding to it); \fIcommandfile\fR must
specify everything necessary to describe the target format.
See section Commands.
.P 1
.ti -\n(Pin
You may also include a script of link commands directly in the command
line by bracketing it between \fR\&\f(CW\(if{\(is\fR and \fR\&\f(CW\(if}\(is\fR characters.
.P 1
.ti -\n(Pin
.iX "k -d"
.LI \fR\&\f(CW-d\ \fR\ 
.iX "k -dc"
.LI \fR\&\f(CW-dc\fR\ 
.iX "k -dp"
.LI \fR\&\f(CW-dp\fR\ 
These three options are equivalent; multiple forms are supported for
compatibility with other linkers.  Use any of them to make \fR\&\f(CWld\fR
assign space to common symbols even if a relocatable output file is
specified (\fR\&\f(CW-r\fR).  The script command
.P 1
.ti -\n(Pin
.iX "k -defsym \fIsymbol\fR=\fIexp\fR"
.LI \fR\&\f(CW-defsym\ \fIsymbol\fR\&\f(CW\ =\ \fIexpression\fR\&\f(CW\fR\ 
Create a global symbol in the output file, containing the absolute
address given by \fIexpression\fR.  You may use this option as many
times as necessary to define multiple symbols in the command line.  A
limited form of arithmetic is supported for the \fIexpression\fR in this
context: you may give a hexadecimal constant or the name of an existing
symbol, or use \fR\&\f(CW+\fR and \fR\&\f(CW-\fR to add or subtract hexadecimal
constants or symbols.  If you need more elaborate expressions, consider
using the linker command language from a script.
.P 1
.ti -\n(Pin
.iX "k -e \fIentry\fR"
.LI \fR\&\f(CW-e\ \fIentry\fR\&\f(CW\ \fR\ 
Use \fIentry\fR as the explicit symbol for beginning execution of your
program, rather than the default entry point. See section Entry Point, for a
discussion of defaults and other ways of specifying the
entry point.
.P 1
.ti -\n(Pin
\&\fR\fR
.P 1
.ti -\n(Pin
.iX "k -F"
.LI \fR\&\f(CW-F\fR\ 
.LI \fR\&\f(CW-F\fIformat\fR\&\f(CW\fR\ 
Some older linkers used this option throughout a compilation toolchain
for specifying object-file format for both input and output object
files.  \fR\&\f(CWgld\fR's mechanisms (the \fR\&\f(CW-b\fR or \fR\&\f(CW-format\fR options
for input files, the \fR\&\f(CWTARGET\fR command in linker scripts for output
files, the \fR\&\f(CWGNUTARGET\fR environment variable) are more flexible, but
but it accepts (and ignores) the \fR\&\f(CW-F\fR option flag for compatibility
with scripts written to call the old linker.
.P 1
.ti -\n(Pin
.iX "k -format"
.LI \fR\&\f(CW-format\ \fIinput-format\fR\&\f(CW\fR\ 
Synonym for \fR\&\f(CW-b\fR \fIinput-format\fR.
.P 1
.ti -\n(Pin
.iX "k -g"
.LI \fR\&\f(CW-g\fR\ 
Accepted, but ignored; provided for compatibility with other tools.
.P 1
.ti -\n(Pin
.iX "k -i"
.LI \fR\&\f(CW-i\fR\ 
Perform an incremental link (same as option \fR\&\f(CW-r\fR).
.P 1
.ti -\n(Pin
.iX "k -l\fIar\fR"
.LI \fR\&\f(CW-l\fIar\fR\&\f(CW\ \fR\ 
Add an archive file \fIar\fR to the list of files to link.  This 
option may be used any number of times.  \fR\&\f(CWld\fR will search its
path-list for occurrences of \fR\&\f(CWlib\fIar\fR\&\f(CW.a\fR for every \fIar\fR
specified.
.P 1
.ti -\n(Pin
.iX "k -L\fIdir\fR"
.LI \fR\&\f(CW-L\fIsearchdir\fR\&\f(CW\ \fR\ 
This command adds path \fIsearchdir\fR to the list of paths that
any number of times.
.P 1
.ti -\n(Pin
The default set of paths searched (without being specified with
some cases also on how it was configured.  See section Environment.  The
paths can also be specified in a link script with the \fR\&\f(CWSEARCH_DIR\fR
command.
.P 1
.ti -\n(Pin
.iX "k -M"
.LI \fR\&\f(CW-M\ \fR\ 
.iX "k -m"
.LI \fR\&\f(CW-m\fR\ 
Print (to the standard output file) a link map\(emdiagnostic information
about where symbols are mapped by \fR\&\f(CWld\fR, and information on global
common storage allocation.
.P 1
.ti -\n(Pin
\&\fR\fR\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-n\ \fR\ 
.iX "k -n"
.iX "k NMAGIC"
sets the text segment to be read only, and \fR\&\f(CWNMAGIC\fR is written
if possible.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-noinhibit-exec\fR\ 
.iX "k -noinhibit-exec"
Normally, the linker will not produce an output file if it encounters
errors during the link process.  With this flag, you can specify that
you wish the output file retained even after non-fatal errors.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-o\ \fIoutput\fR\&\f(CW\fR\ 
.iX "k -o \fIoutput\fR"
\&\fIoutput\fR is a name for the program produced by \fR\&\f(CWld\fR; if this
option is not specified, the name \fR\&\f(CW\(ifa.out\(is\fR is used by default.  The
script command \fR\&\f(CWOUTPUT\fR can also specify the output file name.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-R\ \fIfilename\fR\&\f(CW\fR\ 
.iX "k -R \fIfile\fR"
Read symbol names and their addresses from \fIfilename\fR, but do not
relocate it or include it in the output.  This allows your output file
to refer symbolically to absolute locations of memory defined in other
programs.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-r\ \fR\ 
.iX "k -r"
Generates relocatable output\(emi.e., generate an output file that can in
turn serve as input to \fR\&\f(CWgld\fR.  This is often called \fIpartial
linking\fR.  As a side effect, in environments that support standard Unix
magic numbers, this option also sets the output file's magic number to
If this option is not specified, an absolute file is produced.  When
linking C++ programs, this option \fIwill not\fR resolve references to
constructors; \fR\&\f(CW-Ur\fR is an alternative. 
.P 1
.ti -\n(Pin
This option does the same as \fR\&\f(CW-i\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-S\ \fR\ 
.iX "k -S"
Omits debugger symbol information (but not all symbols) from the output file.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-s\ \fR\ 
.iX "k -s"
Omits all symbol information from the output file.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW{\ \fIscript\fR\&\f(CW\ }\fR\ 
.iX "k { \fIscript\fR }"
You can, if you wish, include a script of linker commands directly in
the command line instead of referring to it via an input file.  When the
character \fR\&\f(CW\(if{\(is\fR occurs on the command line, the linker switches to
interpreting the command language until the end of the list of commands
is reached\(emflagged with a closing brace \fR\&\f(CW\(if}\(is\fR.  Other command-line
options will not be recognized while parsing the script.
See section Commands for a description of the command language.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-Tbss\ \fIorg\fR\&\f(CW\fR\ 
.iX "k -Tbss \fIorg\fR"
.LI \fR\&\f(CW-Tdata\ \fIorg\fR\&\f(CW\fR\ 
.iX "k -Tdata \fIorg\fR"
.LI \fR\&\f(CW-Ttext\ \fIorg\fR\&\f(CW\fR\ 
.iX "k -Ttext \fIorg\fR"
Use \fIorg\fR as the starting address for\(emrespectively\(emthe
\&\fItextorg\fR must be a hexadecimal integer.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-T\ \fIcommandfile\fR\&\f(CW\fR\ 
.LI \fR\&\f(CW-T\fIcommandfile\fR\&\f(CW\fR\ 
.iX "k -T \fIscript\fR"
Equivalent to \fR\&\f(CW-c \fIcommandfile\fR\&\f(CW\fR; supported for compatibility with
other tools.  
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-t\ \fR\ 
.iX "k -t"
Prints names of input files as \fR\&\f(CWld\fR processes them.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-u\ \fIsym\fR\&\f(CW\fR\ 
.iX "k -u \fIsym\fR"
Forces \fIsym\fR to be entered in the output file as an undefined symbol.
This may, for example, trigger linking of additional modules from
standard libraries.  \fR\&\f(CW-u\fR may be repeated with different option
arguments to enter additional undefined symbols.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-Ur\ \fR\ 
.iX "k -Ur"
For anything other than C++ programs, this option is equivalent to
turn serve as input to \fR\&\f(CWgld\fR.  When linking C++ programs, \fR\&\f(CW-Ur\fR
\&\fIwill\fR resolve references to constructors, unlike \fR\&\f(CW-r\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-v\fR\ 
.iX "k -v"
Display the version number for \fR\&\f(CWgld\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-X\ \fR\ 
.iX "k -X"
If \fR\&\f(CW-s\fR or \fR\&\f(CW-S\fR is also specified, delete only local symbols
beginning with \fR\&\f(CW\(ifL\(is\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW-x\fR\ 
.iX "k -x"
If \fR\&\f(CW-s\fR or \fR\&\f(CW-S\fR is also specified, delete all local symbols,
not just those beginning with \fR\&\f(CW\(ifL\(is\fR.
.P 1
.ti -\n(Pin
\&\fR\fR\fR
.P 1
.ti -\n(Pin
\&\fR\fR
.P 1
.ti -\n(Pin
\&\fR
.P 1
.ti -\n(Pin
.LE
.P 1
.sp .5
.H 2 Environment\ Variables
.sp .5
\&\fR
.P 1
and \fR\&\f(CWLDEMULATION\fR.  Depending on the setting of the latter, other
environment variables may be used as well.
.P 1
.iX "k GNUTARGET"
use \fR\&\f(CW-b\fR (or its synonym \fR\&\f(CW-format\fR).  Its value should be one
of the BFD names for an input format (see section BFD).  If there is no
of the host. If \fR\&\f(CWGNUTARGET\fR is set to \fR\&\f(CWdefault\fR then BFD attempts to discover the
input format by examining binary input files; this method often
succeeds, but there are potential ambiguities, since there is no method
of ensuring that the magic number used to flag object-file formats is
unique.  However, the configuration procedure for BFD on each system
places the conventional format for that system first in the search-list,
so ambiguities are resolved in favor of convention.
.P 1
.iX "k LDEMULATION"
personality.  Although \fR\&\f(CWgld\fR is flexible enough to permit its use
in many contexts regardless of configuration, you can use this variable
to make it act more like one or another older linker by default.
.P 1
In particular, the value of \fR\&\f(CWLDEMULATION\fR controls what default
linker script is used (thereby controlling the default input and output
formats; see section BFD); what default paths are searched for
archive libraries; and in some cases whether additional linker script
commands are available.
.P 1
Here is the current set of emulations available:
.VL 5 0
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWLDEMULATION=gld\fR\ 
.iX "k gld"
Emulate the older GNU linker.  When this emulation is selected, the
default library search paths are
.DS 1
\&\fR\&\f(CW
/lib
/usr/lib
/usr/local/lib/lib
.DE
\&\fRThe default output format is set to \fR\&\f(CWa.out-generic-big\fR, and the
default machine is the system's configured BFD default.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWLDEMULATION=gld68k\fR\ 
.iX "k gld68k"
A variant of the \fR\&\f(CWgld\fR emulation; only differs in specifically
setting the default BFD machine as \fR\&\f(CWm68k\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWLDEMULATION=gld960\fR\ 
.iX "k gld960"
.iX "k G960LIB"
.iX "k G960BASE"
Emulate the Intel port of the older \fR\&\f(CWgld\fR for the i960
architectures.  The default library search paths are taken from two
other environment variables, \fR\&\f(CWG960LIB\fR and \fR\&\f(CWG960BASE\fR.  The
default architecture is \fR\&\f(CWi960\fR.  The default output format is set
to \fR\&\f(CWb.out.big\fR, and in fact the default output file name (if
format, for this emulation.
.P 1
.ti -\n(Pin
.iX "k GNU960"
This emulation can behave slightly differently depending on the setting
of the \fR\&\f(CWgld\fR compile-time switch \fR\&\f(CWGNU960\fR.  If \fR\&\f(CWgld\fR is
compiled with \fR\&\f(CWGNU960\fR defined, then an additional environment
variable\(em\fR\&\f(CWGNUTARGET\fR\(emis available; its value, if available,
specifies some other default output format than \fR\&\f(CWb.out.big\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWLDEMULATION=gldm88kbcs\fR\ 
.iX "k gldm88kbcs"
Sets the output format to \fR\&\f(CWm88kbcs\fR and the architecture to
.DS 1
\&\fR\&\f(CW
/lib
/usr/lib
/usr/local/lib
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWLDEMULATION=lnk960\fR\ 
.iX "k lnk960"
Emulate the Intel linker \fR\&\f(CWlnk960\fR.  The default output format is
supports the additional script commands \fR\&\f(CWHLL\fR and \fR\&\f(CWSYSLIB\fR for
specification of library archives.  This is the only emulation with
extensive support for the \fR\&\f(CW-A\fR (architecture) command-line option.
By default, the architecture \fR\&\f(CWCORE\fR is assumed, but you can choose
additional features from the i960 architecture family by using one of
the following with \fR\&\f(CW-A\fR (or by using the \fR\&\f(CWOUTPUT_ARCH\fR command
from a script):
.DS 1
\&\fR\&\f(CW
CORE
KB
SB
MC
XA
CA
KA
SA
.DE
\&\fR
.P 1
.ti -\n(Pin
The default libraries are chosen with some attention to the architecture
selected; the core library \fR\&\f(CW\(ifcg\(is\fR is always included, but the library
.P 1
.ti -\n(Pin
.iX "k GNU960"
Like \fR\&\f(CWgld960\fR, this emulation uses additional environment variables
to set the default library search paths.  Also like \fR\&\f(CWgld960\fR, the
behavior of this emulation is slightly different depending on whether
.P 1
.ti -\n(Pin
.iX "k G960BASE"
.iX "k G960LIB"
.iX "k I960BASE"
If your \fR\&\f(CWgld\fR was compiled with \fR\&\f(CWGNU960\fR defined, the default
paths are taken from all three of \fR\&\f(CWG960LIB\fR, \fR\&\f(CWG960BASE\fR, and
suffixed with \fR\&\f(CW\(if/lib/libcoff\(is\fR; for the last, your path is
automatically suffixed with \fR\&\f(CW\(if/lib\(is\fR.
.P 1
.ti -\n(Pin
If your \fR\&\f(CWgld\fR was \fInot\fR compiled with \fR\&\f(CWGNU960\fR defined,
the default paths are taken from \fR\&\f(CWI960BASE\fR, and \fR\&\f(CWG960BASE\fR is
only consulted if \fR\&\f(CWI960BASE\fR is undefined.  In this case
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWLDEMULATION=vanilla\fR\ 
.iX "k vanilla"
This is the least specific setting for \fR\&\f(CWgld\fR.  You can set
setting makes \fR\&\f(CWgld\fR take the default machine from the BFD
configuration on your system; \fR\&\f(CWa.out-generic-big\fR is the default
target.  No other defaults are specified.
.P 1
.ti -\n(Pin
.LE
.P 1
.P 1
The command language allows explicit control over the link process,
allowing complete specification of the mapping between the linker's
input files and its output.  This includes:
.VL 5 0
.LI \(bu\ \ 
input files 
.LI \(bu\ \ 
file formats
.LI \(bu\ \ 
output file format
.LI \(bu\ \ 
addresses of sections
.LI \(bu\ \ 
placement of common blocks
.LE
.P 1
You may supply a command file (also known as a link script) to the
linker either explicitly through the \fR\&\f(CW-c\fR option, or implicitly as
an ordinary file. If the linker opens a file which it cannot recognize
as a supported object or archive format, it tries to interpret the file
as a command file.
.P 1
You can also include a script directly on the \fR\&\f(CWgld\fR command line,
delimited by the characters \fR\&\f(CW\(if{\(is\fR and \fR\&\f(CW\(if}\(is\fR.
.P 1
.P 1
.sp .5
.H 2 Linker\ Scripts
.sp .5
\&\fRThe \fR\&\f(CWgld\fR command language is a collection of statements; some are
simple keywords setting a particular flag, some are used to select and
group input files or name output files; and two particular statement
types have a fundamental and pervasive impact on the linking process.
.P 1
The most fundamental command of the \fR\&\f(CWgld\fR command language is the
script must have a \fR\&\f(CWSECTIONS\fR command: it specifies a
``picture'' of the output file's layout, in varying degrees of detail.
No other command is required in all cases.
.P 1
The \fR\&\f(CWMEMORY\fR command complements \fR\&\f(CWSECTIONS\fR by describing the
available memory in the target architecture.  This command is optional;
if you don't use a \fR\&\f(CWMEMORY\fR command, \fR\&\f(CWgld\fR assumes sufficient
memory is available in a contiguous block for all output.
See section MEMORY.
.P 1
You may include comments in linker scripts just as in C: delimited
by \fR\&\f(CW\(if/*\(is\fR and \fR\&\f(CW\(if*/\(is\fR.  As in C, comments are syntactically
equivalent to whitespace.
.P 1
.sp .5
.H 2 Expressions
.sp .5
\&\fRMany useful commands involve arithmetic expressions.  The syntax for
expressions in the command language is identical to that of C
expressions, with the following features:
.VL 5 0
.LI \(bu\ \ 
All expressions evaluated as integers and
are of ``long'' or ``unsigned long'' type.
.LI \(bu\ \ 
All constants are integers.
.LI \(bu\ \ 
All of the C arithmetic operators are provided.
.LI \(bu\ \ 
You may reference, define, and create global variables.
.LI \(bu\ \ 
You may call special purpose built-in functions.
.LE
.P 1
.P 1
.sp .5
.H 3 Integers
.sp .5
\&\fRAn octal integer is \fR\&\f(CW\(if0\(is\fR followed by zero or more of the octal
digits (\fR\&\f(CW\(if01234567\(is\fR).
.DS 1
\&\fR\&\f(CW
_as_octal = 0157255;
.DE
\&\fR
.P 1
A decimal integer starts with a non-zero digit followed by zero or
more digits (\fR\&\f(CW\(if0123456789\(is\fR).
.DS 1
\&\fR\&\f(CW
_as_decimal = 57005;
.DE
\&\fR
.P 1
.iX "k 0x"
A hexadecimal integer is \fR\&\f(CW\(if0x\(is\fR or \fR\&\f(CW\(if0X\(is\fR followed by one or
more hexadecimal digits chosen from \fR\&\f(CW\(if0123456789abcdefABCDEF\(is\fR.
.DS 1
\&\fR\&\f(CW
_as_hex = 0xdead;
.DE
\&\fR
.P 1
Decimal integers have the usual values.  To write a negative integer, use
the prefix operator \fR\&\f(CW\(if-\(is\fR; see section Operators.
.DS 1
\&\fR\&\f(CW
_as_neg = -57005;
.DE
\&\fR
.P 1
Additionally the suffixes \fR\&\f(CWK\fR and \fR\&\f(CWM\fR may be used to scale a
constant by 
respectively. For example, the following all refer to the same quantity:
.P 1
.DS 1
\&\fR\&\f(CW
        _fourk_1 = 4K;
        _fourk_2 = 4096;
        _fourk_3 = 0x1000;
.DE
\&\fR
.P 1
.sp .5
.H 3 Symbol\ Names
.sp .5
\&\fR
.iX "k ""
Unless quoted, symbol names start with a letter, underscore, point or
hyphen and may include any letters, underscores, digits, points,
and minus signs.  Unquoted symbol names must not conflict with any
keywords.  You can specify a symbol which contains odd characters or has
the same name as a keyword, by surrounding the symbol name in double quotes:
.DS 1
\&\fR\&\f(CW
        "SECTION" = 9;
        "with a space" = "also with a space" + 10;
.DE
\&\fR
.P 1
.sp .5
.H 3 The\ Location\ Counter
.sp .5
\&\fR
.iX "k ."
The special linker variable \fIdot\fR \fR\&\f(CW\(if.\(is\fR always contains the
current output location counter. Since the \fR\&\f(CW.\fR always refers to
a location in an output section, it must always appear in an
expression within a \fR\&\f(CWSECTIONS\fR command. The \fR\&\f(CW.\fR symbol
may appear anywhere that an ordinary symbol is allowed in an
expression, but its assignments have a side effect. Assigning a value
to the \fR\&\f(CW.\fR symbol will cause the location counter to be moved.
This may be used to create holes in the output section.  The location
counter may never be moved backwards.
.DS 1
\&\fR\&\f(CW
SECTIONS
{
        output :
        {
        file1(.text)
        . = . + 1000;
        file2(.text)
        . += 1000;
        file3(.text)
        } = 0x1234;
}
.DE
\&\fRIn the previous example, \fR\&\f(CWfile1\fR is located at the beginning of the
output section, then there is a 1000 byte gap.  Then \fR\&\f(CWfile2\fR
appears, also with a 1000 byte gap following before \fR\&\f(CWfile3\fR is
loaded.  The notation \fR\&\f(CW\(if= 0x1234\(is\fR specifies what data to write in
the gaps (see section Section Options).
.P 1
.sp .5
.H 3 Operators
.sp .5
\&\fRThe linker recognizes  the standard C set of arithmetic operators, with
the standard bindings and precedence levels:
.DS 1
\&\fR\&\f(CW
precedence      associativity   Operators                Notes
(highest)
1               left            !  -  ~                  (1)
2               left            *  /  %
3               left            +  -
4               left            >>  <<
5               left            ==  !=  >  <  <=  >=
6               left            &
7               left            |
8               left            &&
9               left            ||
10              right           ? :
11              right           &=  +=  -=  *=  /=       (2)
(lowest)
.DE
\&\fRNotes:
(1) Prefix operators 
(2) See section Assignment
.P 1
.sp .5
.H 3 Evaluation
.sp .5
\&\fR
.P 1
The linker uses ``lazy evaluation'' for expressions; it only calculates
an expression when absolutely necessary.  The linker needs the value of
the start address, and the lengths of memory regions, in order to do any
linking at all; these values are computed as soon as possible when the
linker reads in the command file.  However, other values (such as symbol
values) are not known or needed until after storage allocation.  Such
values are evaluated later, when other information (such as the sizes of
output sections) is available for use in the symbol assignment
expression.
.P 1
.sp .5
.H 3 Assignment:\ Defining\ Symbols
.sp .5
\&\fRYou may create global symbols, and assign values (addresses) to global
symbols, using any of the C assignment operators:
.P 1
.VL 5 0
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ \ =\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ &=\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ +=\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ -=\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ *=\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ /=\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.LE
.P 1
Two things distinguish assignment from other operators in \fR\&\f(CWgld\fR
expressions.
.VL 5 0
.LI \(bu\ \ 
Assignment may only be used at the root of an expression;
\&\fR\&\f(CW\(ifa=b+3;\(is\fR is allowed, but \fR\&\f(CW\(ifa+b=3;\(is\fR is an error.
.P 1
.ti -\n(Pin
.iX "k ;"
.LI \(bu\ \ 
A trailing semicolon is required at the end of an assignment
statement. 
.LE
.P 1
Assignment statements may appear:
.VL 5 0
.LI \(bu\ \ 
as commands in their own right in a \fR\&\f(CWgld\fR script; or
.LI \(bu\ \ 
as independent statements within a \fR\&\f(CWSECTIONS\fR command; or
.LI \(bu\ \ 
as part of the contents of a section definition in a
.LE
.P 1
The first two cases are equivalent in effect\(emboth define a symbol with
an absolute address; the last case defines a symbol whose address is
relative to a particular section (see section SECTIONS).
.P 1
When a linker expression is evaluated and assigned to a variable, it is
given either an absolute or a relocatable type. An absolute expression
type is one in which the symbol contains the value that it will have in
the output file, a relocateable expression type is one in which the
value is expressed as a fixed offset from the base of a section.
.P 1
The type of the expression is controlled by its position in the script
file.  A symbol assigned within a section definition is created relative
to the base of the section; a symbol assigned in any other place is
created as an absolute symbol. Since a symbol created within a
section definition is relative to the base of the section, it
will remain relocatable if relocatable output is requested.  A symbol
may be created with an absolute value even when assigned to within a
section definition by using the absolute assignment function
is the last byte of an output section named \fR\&\f(CW.data\fR:
.DS 1
\&\fR\&\f(CW
SECTIONS{ \&...
\&\&.data : 
        {
                *(.data)
                _edata = ABSOLUTE(.) ;
        } 
\&\&... }
.DE
\&\fR
.P 1
The linker tries to put off the evaluation of an assignment until all
the terms in the source expression are known (see section Evaluation).  For
instance the sizes of sections cannot be known until after allocation,
so assignments dependent upon these are not performed until after
allocation. Some expressions, such as those depending upon the location
counter \fIdot\fR, \fR\&\f(CW\(if.\(is\fR must be evaluated during allocation. If the
result of an expression is required, but the value is not available,
then an error results.  For example, a script like the following
.DS 1
\&\fR\&\f(CW
SECTIONS { \&...
      text 9+this_isnt_constant: 
                { \&...
                }
\&\&... }
.DE
\&\fR
.iX "k Non constant expression"
will cause the error message ``\fR\&\f(CWNon constant expression for initial
address\fR''. 
.P 1
.sp .5
.H 3 Built-In\ Functions
.sp .5
\&\fRThe command language includes a number of special purpose built-in
functions for use in link script expressions.
.VL 5 0
.LI \fR\&\f(CWABSOLUTE(\fIexp\fR\&\f(CW)\fR\ 
.iX "k ABSOLUTE(\fIexp\fR)"
returns the absolute value of the expression \fIexp\fR.  Primarily
useful to assign an absolute value to a symbol within a section
definition, where symbol values are normally section-relative.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWADDR(\fIsection\fR\&\f(CW)\fR\ 
.iX "k ADDR(\fIsection\fR)"
returns the absolute address of the named \fIsection\fR.  Your script must
previously have defined the location of that section. In the following
example the \fR\&\f(CWsymbol_1\fR and \fR\&\f(CWsymbol_2\fR are assigned identical
values:
.DS 1
\&\fR\&\f(CW
SECTIONS{ \&...
        .output1:
                { 
                start_of_output_1 = ABSOLUTE(.);
                \&...
                }
        .output:
                {
                symbol_1 = ADDR(.output1);
                symbol_2 = start_of_output_1;
                }
\&\&... }
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWALIGN(\fIexp\fR\&\f(CW)\fR\ 
.iX "k ALIGN(\fIexp\fR)"
returns the result of the current location counter (\fR\&\f(CW.\fR) aligned to
the next \fIexp\fR boundary.  \fIexp\fR must be an expression whose
value is a power of two.  This is equivalent to 
.DS 1
\&\fR\&\f(CW
(. + \fIexp\fR\&\f(CW -1) & ~(\fIexp\fR\&\f(CW-1)
.DE
\&\fR
.P 1
.ti -\n(Pin
does arithmetic on it.  As an example, to align the output \fR\&\f(CW.data\fR
section to the next \fR\&\f(CW0x2000\fR byte boundary after the preceding
section and to set a variable within the section to the next
.DS 1
\&\fR\&\f(CW
SECTIONS{ \&...
        .data ALIGN(0x2000): {
                *(.data)
                variable = ALIGN(0x8000);
        }
\&\&... }
.DE
\&\fRThe first use of \fR\&\f(CWALIGN\fR in this example specifies the location of
a section because it is used as the optional \fIstart\fR attribute of a
section definition (see section Section Options).  The second use simply
defines the value of a variable.
.P 1
.ti -\n(Pin
The built-in \fR\&\f(CWNEXT\fR is closely related to \fR\&\f(CWALIGN\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWDEFINED(\fIsymbol\fR\&\f(CW)\fR\ 
.iX "k DEFINED(\fIsymbol\fR)"
Returns \fR\&\f(CW1\fR if \fIsymbol\fR is in the linker global symbol table and is
defined, otherwise it returns \fR\&\f(CW0\fR.  You can use this to provide default
values for symbols.  For example, this command-file fragment shows how
to set a global symbol \fR\&\f(CWbegin\fR to the first location in the
existed, its value is preserved:
.DS 1
\&\fR\&\f(CW
SECTIONS{ \&...
        .text: {
                begin = DEFINED(begin) ? begin : . ;
                \&...
        }
\&\&... }
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWNEXT(\fIexp\fR\&\f(CW)\fR\ 
.iX "k NEXT(\fIexp\fR)"
Returns the next unallocated address that is a multiple of \fIexp\fR.
This command is closely related to \fR\&\f(CWALIGN(\fIexp\fR\&\f(CW)\fR; unless you
use the \fR\&\f(CWMEMORY\fR command to define discontinuous memory for the
output file, the two commands are equivalent.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWSIZEOF(\fIsection\fR\&\f(CW)\fR\ 
.iX "k SIZEOF(\fIsection\fR)"
returns the size in bytes of the named \fIsection\fR, if the section has
been allocated.  In the following example the \fR\&\f(CWsymbol_1\fR and
.DS 1
\&\fR\&\f(CW
SECTIONS{ \&...
        .output {
                .start = . ;
                \&...
                .end = .;
                }
        symbol_1 = .end - .start;
        symbol_2 = SIZEOF(.output);
\&\&... }
\&\&
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWSIZEOF_HEADERS\fR\ 
.iX "k SIZEOF_HEADERS"
.LI \fR\&\f(CWsizeof_headers\fR\ 
.iX "k sizeof_headers"
the size in bytes of the output file's headers.  You can use this number
as the start address of the first section, if you choose, to facilitate
paging. 
.P 1
.ti -\n(Pin
.LE
.P 1
.sp .5
.H 2 MEMORY\ Command
.sp .5
\&\fR
.iX "k MEMORY"
The linker's default configuration permits allocation of all memory.
You can override this by using the \fR\&\f(CWMEMORY\fR command.  The
memory in the target.  By using it carefully, you can describe which
memory regions may be used by the linker, and which memory regions it
must avoid.  The linker does not shuffle sections to fit into the
available regions, but does move the requested sections into the correct
regions and issue errors when the regions become too full.  
.P 1
Command files may contain at most one use of the \fR\&\f(CWMEMORY\fR
command; however, you can define as many blocks of memory within it as
you wish.  The syntax is:
        
.DS 1
\&\fR\&\f(CW
MEMORY 
      {
       \fIname\fR\&\f(CW (\fIattr\fR\&\f(CW): ORIGIN = \fIorigin\fR\&\f(CW, LENGTH = \fIlen\fR\&\f(CW
       \&...
      }
.DE
\&\fR
.VL 5 0
.LI \fR\&\f(CW\fIname\fR\&\f(CW\fR\ 
is a name used internally by the linker to refer to the region. Any
symbol name may be used.  The region names are stored in a separate
name space, and will not conflict with symbols, filenames or section
names.  Use distinct names to specify multiple regions.
.LI \fR\&\f(CW(\fIattr\fR\&\f(CW)\fR\ 
is an optional list of attributes, permitted for compatibility with the
AT&T linker but not used by \fR\&\f(CWgld\fR beyond checking that the
attribute list is valid.  Valid attribute lists must be made up of the
characters ``\fR\&\f(CWLIRWX\fR''.  If you omit the attribute list, you may
omit the parentheses around it as well.
.LI \fR\&\f(CW\fIorigin\fR\&\f(CW\fR\ 
.iX "k ORIGIN="
.iX "k o="
.iX "k org="
is the start address of the region in physical memory.  It is expressed as
an expression, which must evaluate to a constant before
memory allocation is performed. The keyword \fR\&\f(CWORIGIN\fR may be
abbreviated to \fR\&\f(CWorg\fR or \fR\&\f(CWo\fR.
.LI \fR\&\f(CW\fIlen\fR\&\f(CW\fR\ 
.iX "k LENGTH="
.iX "k len="
.iX "k l="
is the size in bytes of the region (an expression).
The keyword \fR\&\f(CWLENGTH\fR may be abbreviated to \fR\&\f(CWlen\fR or \fR\&\f(CWl\fR.
.LE
.P 1
For example, to specify that memory has two regions available for
allocation\(emone starting at \fR\&\f(CW0\fR for 256 kilobytes, and the other
starting at \fR\&\f(CW0x40000000\fR for four megabytes:
.P 1
.DS 1
\&\fR\&\f(CW
MEMORY 
        {
        rom : ORIGIN= 0, LENGTH = 256K
        ram : org= 0x40000000, l = 4M
        }
.DE
\&\fR
.P 1
Once you have defined a region of memory named \fImem\fR, you can direct
specific output sections there by using a command ending in
\&\fR\&\f(CW\(if>\fImem\fR\&\f(CW\(is\fR within the \fR\&\f(CWSECTIONS\fR command (see section Section
Options).  If the combined output sections directed to a region are too
big for the region, the linker will issue an error message.
.P 1
.sp .5
.H 2 SECTIONS\ Command
.sp .5
\&\fR
.iX "k SECTIONS"
The \fR\&\f(CWSECTIONS\fR command controls exactly where input sections are
placed into output sections, their order and to which output sections
they are allocated.
.P 1
You may use at most one \fR\&\f(CWSECTIONS\fR command in a commands file,
but you can have as many statements within it as you wish.  Statements
within the \fR\&\f(CWSECTIONS\fR command can do one of three things:
.VL 5 0
.LI \(bu\ \ 
define the entry point;
.LI \(bu\ \ 
assign a value to a symbol;
.LI \(bu\ \ 
describe the placement of a named output section, and what input
sections make it up.
.LE
.P 1
The first two possibilities\(emdefining the entry point, and defining
symbols\(emcan also be done outside the \fR\&\f(CWSECTIONS\fR command:
see section Entry Point, see section Assignment.  They are permitted here as
well for your convenience in reading the script, so that symbols or the
entry point can be defined at meaningful points in your output-file
layout.
.P 1
When no \fR\&\f(CWSECTIONS\fR command is specified, the default action
of the linker is to place each input section into an identically named
output section in the order that the sections are first encountered in
the input files; if all input sections are present in the first file,
for example, the order of sections in the output file will match the
order in the first input file.
.P 1
.P 1
.sp .5
.H 3 Section\ Definitions
.sp .5
\&\fRThe most frequently used statement in the \fR\&\f(CWSECTIONS\fR command is
the \fIsection definition\fR, which you can use to specify the
properties of an output section: its location, alignment, contents,
fill pattern, and target memory region can all be specified.  Most of
these specifications are optional; the simplest form of a section
definition is
.DS 1
\&\fR\&\f(CW
SECTIONS { \&...
\&\fIsecname\fR\&\f(CW : {
                 \fIcontents\fR\&\f(CW
                }
\&\&... }
.DE
\&\fR\fIsecname\fR is the name of the output section, and \fIcontents\fR a
specification of what goes there\(emfor example a list of input files or
sections of input files.  As you might assume, the whitespace shown is
optional; you do need the colon \fR\&\f(CW\(if:\(is\fR and the braces \fR\&\f(CW\(if{}\(is\fR,
however. 
.P 1
\&\fIsecname\fR must meet the constraints of your output format.  In
formats which only support a limited number of sections, such as
(\fR\&\f(CWa.out\fR, for example, allows only \fR\&\f(CW.text\fR, \fR\&\f(CW.data\fR or
with numbers and not names (as is the case for Oasys), the name should be
supplied as a quoted numeric string.  A section name may consist of any
sequence characters, but any name which does not conform to the standard
.P 1
.sp .5
.H 3 Section\ Contents
.sp .5
\&\fRIn a section definition, you can specify the contents of an output section by
listing particular object files; by listing particular input-file
sections; or a combination of the two.  You can also place arbitrary
data in the section, and define symbols relative to the beginning of the
section. 
.P 1
The \fIcontents\fR of a section definition may include any of the
following kinds of statement.  You can include as many of these as you
like in a single section definition, separated from one another by
whitespace. 
.P 1
.VL 5 0
.LI \fR\&\f(CW\fIfilename\fR\&\f(CW\fR\ 
.iX "k \fIfilename\fR"
You may simply name a particular input file to be placed in the current
output section; \fIall\fR sections from that file are placed in the
current section definition. To specify a list of particular files by
name:
.DS 1
\&\fR\&\f(CW
\&\&.data: { afile.o bfile.o cfile.o }
.DE
\&\fRThe example also illustrates that multiple statements can be included in
the contents of a section definition, since each filename is a separate
statement. 
.P 1
.ti -\n(Pin
If the file name has already been mentioned in another section
definition, with an explicit section name list, then only those sections
which have not yet been allocated are used.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW\fIfilename\fR\&\f(CW(\ \fIsection\fR\&\f(CW\ )\fR\ 
.LI \fR\&\f(CW\fIfilename\fR\&\f(CW(\ \fIsection\fR\&\f(CW,\ \fIsection\fR\&\f(CW,\ \&...\ )\fR\ 
.LI \fR\&\f(CW\fIfilename\fR\&\f(CW(\ \fIsection\fR\&\f(CW\ \fIsection\fR\&\f(CW\ \&...\ )\fR\ 
.iX "k \fIfilename\fR(\fIsection\fR)"
You can name one or more sections from your input files, for
insertion in the current output section.  If you wish to specify a list
of input-file sections inside the parentheses, you may separate the
section names by either commas or whitespace.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW*\ (\fIsection\fR\&\f(CW)\fR\ 
.LI \fR\&\f(CW*\ (\fIsection\fR\&\f(CW,\ \fIsection\fR\&\f(CW,\ \&...)\fR\ 
.LI \fR\&\f(CW*\ (\fIsection\fR\&\f(CW\ \fIsection\fR\&\f(CW\ \&...\fR\ 
.iX "k *(\fIsection\fR)"
Instead of explicitly naming particular input files in a link control
script, you can refer to \fIall\fR files from the \fR\&\f(CWgld\fR command
line: use \fR\&\f(CW\(if*\(is\fR instead of a particular filename before the
parenthesized input-file section list.  
.P 1
.ti -\n(Pin
For example, to copy sections \fR\&\f(CW1\fR through \fR\&\f(CW4\fR from a Oasys file
into the \fR\&\f(CW.text\fR section of an \fR\&\f(CWa.out\fR file, and sections \fR\&\f(CW13\fR
and \fR\&\f(CW14\fR into the \fR\&\f(CW.data\fR section:
.DS 1
\&\fR\&\f(CW
SECTIONS {
        .text :{
                *("1" "2" "3" "4")
        }
\&\&
        .data :{
                *("13" "14")
        }
}
.DE
\&\fR
.P 1
.ti -\n(Pin
If you have already explicitly included some files by name, \fR\&\f(CW\(if*\(is\fR
refers to all \fIremaining\fR files\(emthose whose places in the output
file have not yet been defined.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW[\ \fIsection\fR\&\f(CW\ ]\fR\ 
.LI \fR\&\f(CW[\ \fIsection\fR\&\f(CW,\ \fIsection\fR\&\f(CW,\ \&...\ ]\fR\ 
.LI \fR\&\f(CW[\ \fIsection\fR\&\f(CW\ \fIsection\fR\&\f(CW\ \&...\ ]\fR\ 
.iX "k [ \fIsections\fR ]"
This is an alternate notation to specify named sections from all
unallocated input files; its effect is exactly the same as that of
\&\fR\&\f(CW\(if* (\fIsection\fR\&\f(CW\&...)\(is\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW\fIfilename\fR\&\f(CW\fR\&\f(CW(\ COMMON\ )\fR\ 
.LI \fR\&\f(CW[\ COMMON\ ]\fR\ 
.iX "k [COMMON]"
Specify where in your output file to place uninitialized data
with this notation.  \fR\&\f(CW[COMMON]\fR by itself refers to all
uninitialized data from all input files (so far as it is not yet
allocated); \fIfilename\fR\fR\&\f(CW(COMMON)\fR refers to uninitialized data
from a particular file.  Both are special cases of the general
mechanisms for specifying where to place input-file sections:
were in an input-file section named \fR\&\f(CWCOMMON\fR, regardless of the
input file's format.
.LE
.P 1
For example, the following command script arranges the output file into
three consecutive sections, named \fR\&\f(CW.text\fR, \fR\&\f(CW.data\fR, and
sections of all the input files:
.DS 1
\&\fR\&\f(CW
SECTIONS { 
  .text: { *(.text) }
  .data: { *(.data) } 
  .bss:  { *(.bss)  [COMMON] } 
} 
.DE
\&\fR
.P 1
The following example reads all of the sections from file \fR\&\f(CWall.o\fR
and places them at the start of output section \fR\&\f(CWoutputa\fR which
starts at location \fR\&\f(CW0x10000\fR. All of section \fR\&\f(CW.input1\fR from
file \fR\&\f(CWfoo.o\fR follows immediately, in the same output section.  All
of section \fR\&\f(CW.input2\fR from \fR\&\f(CWfoo.o\fR goes into output section
All of the remaining \fR\&\f(CW.input1\fR and \fR\&\f(CW.input2\fR sections from any
files are written to output section \fR\&\f(CWoutputc\fR.
.P 1
.DS 1
\&\fR\&\f(CW
SECTIONS {
        outputa 0x10000 :
                {
                all.o
                foo.o (.input1)
                }
        outputb :
                {
                foo.o (.input2)
                foo1.o (.input1)
                }
        outputc :
                {
                *(.input1)
                *(.input2)
                }
}
.DE
\&\fR
.P 1
There are still more kinds of statements permitted in the contents of
output section definitions.  The foregoing statements permitted you to
arrange, in your output file, data originating from your input files.
You can also place data directly in an output section from the link
command script.  Most of these additional statements involve
expressions; see section Expressions.  Although these statements are shown
separately here for ease of presentation, no such segregation is needed
within a section definition in the \fR\&\f(CWSECTIONS\fR command; you can
intermix them freely with any of the statements we've just described.
.P 1
.VL 5 0
.LI \fR\&\f(CWCREATE_OBJECT_SYMBOLS\fR\ 
.iX "k CREATE_OBJECT_SYMBOLS"
instructs the linker to create a symbol for each input file
in the current section, set with the address of the first byte of
data written from the input file.  For instance, with \fR\&\f(CWa.out\fR
files it is conventional to have a symbol for each input file.  You can
accomplish this by defining the output \fR\&\f(CW.text\fR section as follows:
.DS 1
\&\fR\&\f(CW
SECTIONS {
        .text 0x2020 :
                 {
                CREATE_OBJECT_SYMBOLS
                *(.text)
                _etext = ALIGN(0x2000);
                }
        \&...
}
.DE
\&\fR
.P 1
.ti -\n(Pin
If \fR\&\f(CWobjsym\fR is a file containing this script, and \fR\&\f(CWa.o\fR,
contents like the following\(em
.DS 1
\&\fR\&\f(CW
/* a.c */
\&\&
afunction() { }
int adata=1;
int abss;
.DE
\&\fR
.P 1
.ti -\n(Pin
\&\fR\&\f(CW\(ifgld -M sample a.o b.o c.o d.o\(is\fR would create a map like this,
containing symbols matching the object file names:
.DS 1
\&\fR\&\f(CW
00000000 A __DYNAMIC
00004020 B _abss
00004000 D _adata
00002020 T _afunction
00004024 B _bbss
00004008 D _bdata
00002038 T _bfunction
00004028 B _cbss
00004010 D _cdata
00002050 T _cfunction
0000402c B _dbss
00004018 D _ddata
00002068 T _dfunction
00004020 D _edata
00004030 B _end
00004000 T _etext
00002020 t a.o
00002038 t b.o
00002050 t c.o
00002068 t d.o
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ =\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.iX "k \fIsymbol\fR = \fIexpression\fR ;"
.LI \fR\&\f(CW\fIsymbol\fR\&\f(CW\ \fIf\fR\&\f(CW=\ \fIexpression\fR\&\f(CW\ ;\fR\ 
.iX "k \fIsymbol\fR \fIf\fR= \fIexpression\fR ;"
\&\fIsymbol\fR is any symbol name (see section Symbols).  ``\fIf\fR='' 
refers to any of the operators \fR\&\f(CW&= += -= *= /=\fR which combine
arithmetic and assignment.
.P 1
.ti -\n(Pin
When you assign a value to a symbol within a particular section
definition, the value is relative to the beginning of the section
(see section Assignment).  If you write
.DS 1
\&\fR\&\f(CW
SECTIONS {
  abs = 14 ;
  \&...
  .data: { \&... rel = 14 ; \&... }
  abs2 = 14 + ADDR(.data);
  \&...
}
.DE
\&\fRsame value as \fR\&\f(CWabs2\fR.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWBYTE(\fIexpression\fR\&\f(CW)\fR\ 
.iX "k BYTE(\fIexpression\fR)"
.LI \fR\&\f(CWSHORT(\fIexpression\fR\&\f(CW)\fR\ 
.iX "k SHORT(\fIexpression\fR)"
.LI \fR\&\f(CWLONG(\fIexpression\fR\&\f(CW)\fR\ 
.iX "k LONG(\fIexpression\fR)"
By including one of these three statements in a section definition, you
can explicitly place one, two, or four bytes (respectively) at the
current address of that section.  Multiple-byte quantities are
represented in whatever byte order is appropriate for the output file
format (see section BFD). 
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWFILL(\fIexpression\fR\&\f(CW)\fR\ 
.iX "k FILL(\fIexpression\fR)"
Specifies the ``fill pattern'' for the current section.  Any otherwise
unspecified regions of memory within the section (for example, regions
you skip over by assigning a new value to the location counter \fR\&\f(CW\(if.\(is\fR)
are filled with the two least significant bytes from the
\&\fIexpression\fR argument.  A \fR\&\f(CWFILL\fR statement covers memory
locations \fIafter\fR the point it occurs in the section definition; by
including more than one \fR\&\f(CWFILL\fR statement, you can have different
fill patterns in different parts of an output section.
.LE
.P 1
.sp .5
.H 3 Optional\ Section\ Attributes
.sp .5
\&\fRHere is the full syntax of a section definition, including all the
optional portions:
.P 1
.DS 1
\&\fR\&\f(CW
SECTIONS {
\&\&...
\&\fIsecname\fR\&\f(CW \fIstart\fR\&\f(CW BLOCK(\fIalign\fR\&\f(CW) : { \fIcontents\fR\&\f(CW } =\fIfill\fR\&\f(CW >\fIregion\fR\&\f(CW
\&\&...
}
.DE
\&\fR
.P 1
\&\fIsecname\fR and \fIcontents\fR are required.  See section Section
Definition, and see section Section Contents for details on \fIcontents\fR.
The remaining elements\(em\fIstart\fR, \fR\&\f(CWBLOCK(\fIalign)\fR\&\f(CW\fR,
.P 1
.VL 5 0
.LI \fR\&\f(CW\fIstart\fR\&\f(CW\ \fR\ 
You can force the output section to be loaded at a specified address by
specifying \fIstart\fR immediately following the section name.
\&\fIstart\fR can be represented as any expression. The following
example generates section \fIoutput\fR at location
.DS 1
\&\fR\&\f(CW
SECTIONS {
        \&...
        output 0x40000000: {
               \&...
          }
        \&...
}
.DE
\&\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWBLOCK(\fIalign\fR\&\f(CW)\fR\ 
.iX "k BLOCK(\fIalign\fR)"
You can include \fR\&\f(CWBLOCK()\fR specification to advance the location of
the location counter \fR\&\f(CW.\fR prior to the beginning of the section, so
that the section will begin at the specified alignment.  \fIalign\fR is
an expression.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW=\fIfill\fR\&\f(CW\fR\ 
.iX "k =\fIfill\fR"
You may use any expression to specify \fIfill\fR.  Including
value for that section. Any unallocated holes in the current output
section when written to the output file will be filled with the two
least significant bytes of the value, repeated as necessary.  You can
also change the fill value with a \fR\&\f(CWFILL\fR statement in the
\&\fIcontents\fR of a section definition.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CW>\fIregion\fR\&\f(CW\fR\ 
.iX "k >\fIregion\fR"
Assign this section to a previously defined region of memory.  
See section MEMORY.
.P 1
.ti -\n(Pin
.LE
.P 1
.sp .5
.H 2 The\ Entry\ Point
.sp .5
\&\fR
.iX "k ENTRY(\fIsymbol\fR)"
The linker command language includes a command specifically for
defining the first executable instruction in an output file (its
\&\fIentry point\fR).  Its argument is a symbol name:
.DS 1
\&\fR\&\f(CW
ENTRY(\fIsymbol\fR\&\f(CW)
.DE
\&\fR
.P 1
Like symbol assignments, the \fR\&\f(CWENTRY\fR command may be placed either
as an independent command in the command file, or among the section
definitions within the \fR\&\f(CWSECTIONS\fR command\(emwhatever makes the most
sense for your layout.
.P 1
You may indicate it in any of the following ways (shown in descending
order of priority: methods higher in the list override methods lower down).
.VL 5 0
.LI \(bu\ \ 
the \fR\&\f(CW-e\fR \fIentry\fR command-line option;
.LI \(bu\ \ 
the \fR\&\f(CWENTRY(\fIsymbol\fR\&\f(CW\fR command in a linker control script;
.LI \(bu\ \ 
the value of the symbol \fR\&\f(CWstart\fR, if present;
.LI \(bu\ \ 
the value of the symbol \fR\&\f(CW_main\fR, if present;
.LI \(bu\ \ 
the address of the first byte of the \fR\&\f(CW.text\fR section, if present;
.LI \(bu\ \ 
The address \fR\&\f(CW0\fR.
.LE
.P 1
For example, you can use these rules to generate an entry point with an
assignment statement: if no symbol \fR\&\f(CWstart\fR is defined within your
input files, you can simply define it, assigning it an appropriate
value\(em
.DS 1
\&\fR\&\f(CW
start = 0x2020;
.DE
\&\fR
.P 1
The example shows an absolute address, but you can use any expression.
For example, if your input object files use some other symbol-name
convention for the entry point, you can just assign the value of
whatever symbol contains the start address to \fR\&\f(CWstart\fR:
.DS 1
\&\fR\&\f(CW
start = other_symbol;
.DE
\&\fR
.P 1
.sp .5
.H 2 Other\ Commands
.sp .5
\&\fRThe command language includes a number of other commands that you can
use for specialized purposes.  They are similar in purpose to
command-line options.
.P 1
.VL 5 0
.LI \fR\&\f(CWFLOAT\fR\ 
.iX "k FLOAT"
.LI \fR\&\f(CWNOFLOAT\fR\ 
.iX "k NOFLOAT"
These keywords were used in some older linkers to request a particular
math subroutine library.  \fR\&\f(CWgld\fR doesn't use the keywords, assuming
instead that any necessary subroutines are in libraries specified using
the general mechanisms for linking to archives; but to permit the use of
scripts that were written for the older linkers, the keywords
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWFORCE_COMMON_ALLOCATION\fR\ 
.iX "k FORCE_COMMON_ALLOCATION"
This command has the same effect as the \fR\&\f(CW-d\fR command-line option:
to make \fR\&\f(CWld\fR assign space to common symbols even if a relocatable
output file is specified (\fR\&\f(CW-r\fR).
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWHLL\ (\ \fIfile\fR\&\f(CW,\ \fIfile\fR\&\f(CW,\ \&...\ )\fR\ 
.iX "k HLL ( \fIfiles\fR )"
.LI \fR\&\f(CWHLL\ (\ \fIfile\fR\&\f(CW\ \fIfile\fR\&\f(CW\ \&...\ )\fR\ 
.LI \fR\&\f(CWHLL\ (\ )\fR\ 
Include ``high-level libraries'' or archives as input files in the link.
Using \fR\&\f(CWHLL(\fIfile\fR\&\f(CW\fR in a linker script is equivalent to
including \fR\&\f(CW-l\fR\fIfile\fR on the command line.  
.P 1
.ti -\n(Pin
The \fR\&\f(CWHLL\fR command is only supported when \fR\&\f(CWgld\fR emulates
variable.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWINPUT\ (\ \fIfile\fR\&\f(CW,\ \fIfile\fR\&\f(CW,\ \&...\ )\fR\ 
.iX "k INPUT ( \fIfiles\fR )"
.LI \fR\&\f(CWINPUT\ (\ \fIfile\fR\&\f(CW\ \fIfile\fR\&\f(CW\ \&...\ )\fR\ 
Use this command to include binary input files in the link, without
including them in a particular section definition.  Files specified this
way are treated identically to object files listed on the command line.
.P 1
.ti -\n(Pin
\&\fR\fR
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWOUTPUT\ (\ \fIfilename\fR\&\f(CW\ )\fR\ 
.iX "k OUTPUT ( \fIfilename\fR )"
Name the link output file \fIfilename\fR.  The effect of
\&\! \fR\&\f(CW-o \fIfilename\fR\&\f(CW\fR
, and whichever is encountered last will
control the name actually used to name the output file.  In particular,
you can use this command to supply a default output-file name other than
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWOUTPUT_ARCH\ (\ \fIbfdname\fR\&\f(CW\ )\fR\ 
.iX "k OUTPUT_ARCH ( \fIbfdname\fR )"
Specify a particular output machine architecture, with one of the names
used by the BFD back-end routines (see section BFD).  This command is often
unnecessary; the architecture is most often set implicitly by either the
system BFD configuration or as a side effect of the \fR\&\f(CWOUTPUT_FORMAT\fR
command.  
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWOUTPUT_FORMAT\ (\ \fIbfdname\fR\&\f(CW\ )\fR\ 
.iX "k OUTPUT_FORMAT ( \fIbfdname\fR )"
Specify a particular output format, with one of the names used by the
BFD back-end routines (see section BFD).  This selection will only affect
the output file; the related command \fR\&\f(CWTARGET\fR affects primarily
input files.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWSEARCH_DIR\ (\ \fIpath\fR\&\f(CW\ )\fR\ 
.iX "k SEARCH_DIR ( \fIpath\fR )"
Add \fIpath\fR to the list of paths where \fR\&\f(CWgld\fR looks for
archive libraries.  \fR\&\f(CWSEARCH_DIR(\fIpath\fR\&\f(CW)\fR has the same
effect as \fR\&\f(CW-L\fIpath\fR\&\f(CW)\fR on the command line.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWSTARTUP\ (\ \fIfilename\fR\&\f(CW\ )\fR\ 
.iX "k STARTUP ( \fIfilename\fR )"
Ensure that \fIfilename\fR is the first input file used in the link
process. 
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWSYSLIB\ (\ \fIfile\fR\&\f(CW,\ \fIfile\fR\&\f(CW,\ \&...\ )\fR\ 
.iX "k SYSLIB ( \fIfile\fR, \fIfile\fR, \&... )"
.LI \fR\&\f(CWSYSLIB\ (\ \fIfile\fR\&\f(CW\ \fIfile\fR\&\f(CW\ \&...\ )\fR\ 
Use the named \fIfile\fRs as binary input files, searching for them in
the same list of paths as archives.
.P 1
.ti -\n(Pin
The \fR\&\f(CWSYSLIB\fR command is only supported when \fR\&\f(CWgld\fR emulates
variable.
.P 1
.ti -\n(Pin
.LI \fR\&\f(CWTARGET\ (\ \fIformat\fR\&\f(CW\ )\fR\ 
.iX "k TARGET ( \fIformat\fR )"
Change the input-file object code format (like the command-line option
one of the strings used by BFD to name binary formats.  In the current
used as the default format for the \fR\&\f(CWgld\fR output file.
See section BFD.
.P 1
.ti -\n(Pin
.iX "k GNUTARGET"
If you don't use the \fR\&\f(CWTARGET\fR command, \fR\&\f(CWgld\fR uses the value of
the environment variable \fR\&\f(CWGNUTARGET\fR, if available, to select the
output file format.  If that variable is also absent, \fR\&\f(CWgld\fR uses
the default format configured for your machine in the BFD libraries.
.P 1
.ti -\n(Pin
.LE
.P 1
.P 1
The linker accesses object and archive files using the BFD libraries.
These libraries allow the linker to use the same routines to operate on
object files whatever the object file format.  A different object file
format can be supported simply by creating a new BFD back end and adding
it to the library.  BFD supports the following combinations of
architectures (row labels below) and object formats (column headings):
.DS 1
\&\fR\&\f(CW
               |ieee
               |  |oasys
               |  |  |a.out-generic-little
               |  |  |  |a.out-generic-big
               |  |  |  |  |m88kbcs
               |  |  |  |  |  |srec
               |  |  |  |  |  |  |coff-Intel-little
               |  |  |  |  |  |  |  |coff-Intel-big
               |  |  |  |  |  |  |  |  |b.out.little
               |  |  |  |  |  |  |  |  |  |b.out.big
               |  |  |  |  |  |  |  |  |  |
           m68k|**|**|  |  |  |**|  |  |  |
            vax|**|**|  |  |  |**|  |  |  |
           i960|**|**|  |  |  |**|**|**|**|**
           a29k|**|**|**|**|  |**|  |  |  |
          sparc|**|**|**|**|  |**|  |  |  |
           mips|**|**|  |  |  |**|  |  |  |
           i386|**|**|**|**|  |**|  |  |  |
          ns32k|**|**|  |  |  |**|  |  |  |
          tahoe|**|**|  |  |  |**|  |  |  |
           i860|**|**|  |  |  |**|  |  |  |
           romp|**|**|  |  |  |**|  |  |  |
        alliant|**|**|  |  |  |**|  |  |  |
         convex|**|**|  |  |  |**|  |  |  |
           m88k|**|**|  |  |**|**|  |  |  |
        pyramid|**|**|  |  |  |**|  |  |  |
         H8/300|**|**|  |  |  |**|  |  |  |
\&\&
.DE
\&\fR
.P 1
As with most implementations, BFD is a compromise between
several conflicting requirements. The major factor influencing
BFD design was efficiency: any time used converting between
formats is time which would not have been spent had BFD not
been involved. This is partly offset by abstraction payback; since
BFD simplifies applications and back ends, more time and care
may be spent optimizing algorithms for a greater speed.
.P 1
One minor artifact of the BFD solution which you should bear in
mind is the potential for information loss.  There are two places where
useful information can be lost using the BFD mechanism; during
conversion and during output. See section BFD information loss.
.P 1
.P 1
.sp .5
.H 2 How\ it\ works:\ an\ outline\ of\ BFD
.sp .5
\&\fRWhen an object file is opened, BFD subroutines automatically
determine the format of the input object file, and build a descriptor in
memory with pointers to routines that will be used to access elements of
the object file's data structures.
.P 1
As different information from the the object files is required
BFD reads from different sections of the file and processes them.
For example a very common operation for the linker is processing symbol
tables.  Each BFD back end provides a routine for converting
between the object file's representation of symbols and an internal
canonical format. When the linker asks for the symbol table of an object
file, it calls through the memory pointer to the relevant BFD
back end routine which reads and converts the table into a canonical
form.  The linker then operates upon the common form. When the link is
finished and the linker writes the symbol table of the output file,
another BFD back end routine is called which takes the newly
created symbol table and converts it into the chosen output format.
.P 1
.sp .5
.H 2 Information\ Loss
.sp .5
\&\fR\fIInformation can be lost during output.\fR The output formats
supported by BFD do not provide identical facilities, and
information which may be described in one form has nowhere to go in
another format. One example of this is alignment information in
alignment information on the contained data, so when a file is linked
from \fR\&\f(CWb.out\fR and an \fR\&\f(CWa.out\fR image is produced, alignment
information will not propagate to the output file. (The linker will
still use the alignment information internally, so the link is performed
correctly).
.P 1
Another example is COFF section names. COFF files may contain an
unlimited number of sections, each one with a textual section name. If
the target of the link is a format which does not have many sections (eg
link cannot be done simply. You can circumvent this problem by
describing the desired input-to-output section mapping with the command
language.
.P 1
\&\fIInformation can be lost during canonicalization.\fR The BFD
internal canonical form of the external formats is not exhaustive; there
are structures in input formats for which there is no direct
representation internally.  This means that the BFD back ends
cannot maintain all possible data richness through the transformation
between external to internal and back to external formats.
.P 1
This limitation is only a problem when using the linker to read one
format and write another. Each BFD back end is responsible for
maintaining as much data as possible, and the internal BFD
canonical form has structures which are opaque to the BFD core,
and exported only to the back ends. When a file is read in one format,
the canonical form is generated for BFD and the linker. At the
same time, the back end saves away any information which may otherwise
be lost. If the data is then written back in the same format, the back
end routine will be able to use the canonical form provided by the
BFD core as well as the information it prepared earlier.  Since
there is a great deal of commonality between back ends, this mechanism
is very useful. There is no information lost for this reason when
linking big endian COFF to little endian COFF, or from \fR\&\f(CWa.out\fR to
only lost from the files whose format differs from the destination.
.P 1
.sp .5
.H 2 Mechanism\ 
.sp .5
\&\fRThe greatest potential for loss of information is when there is least
overlap between the information provided by the source format, that
stored by the canonical format, and the information needed by the
destination format. A brief description of the canonical form may help
you appreciate what kinds of data you can count on preserving across
conversions.
.P 1
.VL 5 0
.LI \fIfiles\fR\ 
Information on target machine architecture, particular implementation
and format type are stored on a per-file basis. Other information
includes a demand pageable bit and a write protected bit.  Note that
information like Unix magic numbers is not stored here\(emonly the magic
numbers' meaning, so a \fR\&\f(CWZMAGIC\fR file would have both the demand pageable
bit and the write protected text bit set.
.P 1
.ti -\n(Pin
The byte order of the target is stored on a per-file basis, so that big-
and little-endian object files may be linked with one another.
.P 1
.ti -\n(Pin
.LI \fIsections\fR\ 
Each section in the input file contains the name of the section, the
original address in the object file, various flags, size and alignment
information and pointers into other BFD data structures.
.P 1
.ti -\n(Pin
.LI \fIsymbols\fR\ 
Each symbol contains a pointer to the object file which originally
defined it, its name, its value, and various flag bits.  When a
BFD back end reads in a symbol table, the back end relocates all
symbols to make them relative to the base of the section where they were
defined.  This ensures that each symbol points to its containing
section.  Each symbol also has a varying amount of hidden data to contain
private data for the BFD back end.  Since the symbol points to the
original file, the private data format for that symbol is accessible.
formats without problems.
.P 1
.ti -\n(Pin
Normal global and simple local symbols are maintained on output, so an
output file (no matter its format) will retain symbols pointing to
functions and to global, static, and common variables.  Some symbol
information is not worth retaining; in \fR\&\f(CWa.out\fR type information is
stored in the symbol table as long symbol names. This information would
be useless to most COFF debuggers and may be thrown away with
appropriate command line switches. (The GNU debugger \fR\&\f(CWgdb\fR does
support \fR\&\f(CWa.out\fR style debugging information in COFF).
.P 1
.ti -\n(Pin
There is one word of type information within the symbol, so if the
format supports symbol type information within symbols (for example COFF,
IEEE, Oasys) and the type is simple enough to fit within one word
(nearly everything but aggregates) the information will be preserved.
.P 1
.ti -\n(Pin
.LI \fIrelocation\ level\fR\ 
Each canonical BFD relocation record contains a pointer to the symbol to
relocate to, the offset of the data to relocate, the section the data
is in and a pointer to a relocation type descriptor. Relocation is
performed effectively by message passing through the relocation type
descriptor and symbol pointer. It allows relocations to be performed
on output data using a relocation method only available in one of the
input formats. For instance, Oasys provides a byte relocation format.
A relocation record requesting this relocation type would point
indirectly to a routine to perform this, so the relocation may be
performed on a byte being written to a COFF file, even though 68k COFF
has no such relocation type.
.P 1
.ti -\n(Pin
.LI \fIline\ numbers\fR\ 
Object formats can contain, for debugging purposes, some form of mapping
between symbols, source line numbers, and addresses in the output file.
These addresses have to be relocated along with the symbol information.
Each symbol with an associated list of line number records points to the
first record of the list.  The head of a line number list consists of a
pointer to the symbol, which allows divination of the address of the
function whose line number is being described. The rest of the list is
made up of pairs: offsets into the section and line numbers. Any format
which can simply derive this information can pass it successfully
between formats (COFF, IEEE and Oasys).
.LE
.P 1
.bp
.sp .5
.HU Index
.sp .5
\&\fR
.P 1
.P 1
.P 1
.P 1
.P 1
.P 1
