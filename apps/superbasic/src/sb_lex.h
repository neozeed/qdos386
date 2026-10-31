/*
 * sb_lex.h - tokenizer for SuperBASIC source lines.
 *
 * Lexical rules implemented (each verified against the JS ROM unless noted
 * in docs/JS-SUPERBASIC-MAP.md):
 *   - keywords and identifiers are case-insensitive; identifiers may end
 *     in '$' (string variables); identifiers are otherwise kept as typed;
 *   - "GO TO" (with any blanks) is one keyword token, GOTO is the same;
 *   - a string is "..." with no escape: it ends at the next quote;
 *   - numbers: 12, 1.5, .5, 1., 2E+3, 1e3;
 *   - REM (or REMARK) makes the rest of the line a comment;
 *   - operators: + - * / ^ & ( ) , ; : = <> < > <= >= == (=)
 *                || && ^^ (bitwise, see the map) and # (channels, unsupported).
 */
#ifndef SB_LEX_H
#define SB_LEX_H

#include <stddef.h>

#include "sb_mem.h"

/* token types */
#define SB_TK_EOL    0
#define SB_TK_NUM    1
#define SB_TK_STR    2
#define SB_TK_IDENT  3
#define SB_TK_KW     4
#define SB_TK_OP     5
#define SB_TK_ERR    6

/* keywords */
#define SB_KW_PRINT     1
#define SB_KW_LET       2
#define SB_KW_REM       3
#define SB_KW_GOTO      4
#define SB_KW_IF        5
#define SB_KW_THEN      6
#define SB_KW_FOR       7
#define SB_KW_TO        8
#define SB_KW_STEP      9
#define SB_KW_NEXT     10
#define SB_KW_STOP     11
#define SB_KW_CONTINUE 12
#define SB_KW_RUN      13
#define SB_KW_LIST     14
#define SB_KW_NEW      15
#define SB_KW_CLEAR    16
#define SB_KW_AND      17
#define SB_KW_OR       18
#define SB_KW_XOR      19
#define SB_KW_NOT      20
#define SB_KW_DIV      21
#define SB_KW_MOD      22
#define SB_KW_LEN      23
#define SB_KW_ERNUM    24
#define SB_KW_ABS      25
#define SB_KW_GOSUB    26
#define SB_KW_RETURN   27
#define SB_KW_INPUT    28
#define SB_KW_DIM      29
#define SB_KW_DATA     30
#define SB_KW_READ     31
#define SB_KW_RESTORE  32
#define SB_KW_DEFINE   33
#define SB_KW_PROCEDURE 34
#define SB_KW_FUNCTION 35
#define SB_KW_END      36
#define SB_KW_LOCAL    37

/* operators */
#define SB_OP_PLUS     1
#define SB_OP_MINUS    2
#define SB_OP_STAR     3
#define SB_OP_SLASH    4
#define SB_OP_CARET    5
#define SB_OP_AMP      6
#define SB_OP_LPAR     7
#define SB_OP_RPAR     8
#define SB_OP_COMMA    9
#define SB_OP_SEMI    10
#define SB_OP_COLON   11
#define SB_OP_EQ      12
#define SB_OP_NE      13
#define SB_OP_LT      14
#define SB_OP_GT      15
#define SB_OP_LE      16
#define SB_OP_GE      17
#define SB_OP_BITOR   18
#define SB_OP_BITAND  19
#define SB_OP_BITXOR  20
#define SB_OP_HASH    21

typedef struct SbLexer {
    const char *src;
    size_t len;
    size_t pos;      /* scan position (after the current token)      */
    size_t tstart;   /* start offset of the current token            */
    int type;        /* SB_TK_*                                      */
    int kw;          /* SB_KW_* when type == SB_TK_KW                */
    int op;          /* SB_OP_* when type == SB_TK_OP                */
    double num;      /* value when type == SB_TK_NUM                 */
    const char *text;/* identifier or string contents (not NUL-ended) */
    size_t tlen;
} SbLexer;

void sb_lex_start(SbLexer *lx, const char *src, size_t len, size_t pos);

/* Advance to the next token; the new token becomes the current one. */
void sb_lex_next(SbLexer *lx);

/* Skip blanks, returning nonzero if the current token is the keyword kw. */
int sb_lex_is_kw(const SbLexer *lx, int kw);

/* Canonical form of a source line: keywords are upper-cased (GOTO becomes
 * "GO TO", REM becomes "REMark" with the rest of the line verbatim), and
 * all other text is kept exactly as typed.  On success *out is a new block
 * owned by the caller.  Returns SB_OK or an SB_E_ code. */
int sb_lex_canonical(SbAlloc *a, const char *src, size_t len,
                     char **out, size_t *outlen);

#endif /* SB_LEX_H */
