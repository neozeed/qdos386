/* QDOS/386 R12C - the default _prog_name (C68 convention): an empty title.  This member defines nothing else, so the
 * linker takes it only for a program that does not define _prog_name itself (crt0 refers to the symbol).
 * docs/R12C-PROGRAM-TITLE-ABI.md. */
char _prog_name[] = "";
