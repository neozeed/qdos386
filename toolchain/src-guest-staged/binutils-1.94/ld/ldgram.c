
/*  A Bison parser, made from /workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y  */

#define YYBISON 1  /* Identify Bison output.  */

#define	INT	258
#define	NAME	259
#define	PLUSEQ	260
#define	MINUSEQ	261
#define	MULTEQ	262
#define	DIVEQ	263
#define	LSHIFTEQ	264
#define	RSHIFTEQ	265
#define	ANDEQ	266
#define	OREQ	267
#define	OROR	268
#define	ANDAND	269
#define	EQ	270
#define	NE	271
#define	LE	272
#define	GE	273
#define	LSHIFT	274
#define	RSHIFT	275
#define	UNARY	276
#define	ALIGN_K	277
#define	BLOCK	278
#define	LONG	279
#define	SHORT	280
#define	BYTE	281
#define	SECTIONS	282
#define	SIZEOF_HEADERS	283
#define	OUTPUT_FORMAT	284
#define	FORCE_COMMON_ALLOCATION	285
#define	OUTPUT_ARCH	286
#define	MEMORY	287
#define	NOLOAD	288
#define	DSECT	289
#define	COPY	290
#define	INFO	291
#define	OVERLAY	292
#define	DEFINED	293
#define	TARGET_K	294
#define	SEARCH_DIR	295
#define	MAP	296
#define	ENTRY	297
#define	OPTION_e	298
#define	OPTION_c	299
#define	OPTION_noinhibit_exec	300
#define	OPTION_s	301
#define	OPTION_S	302
#define	OPTION_sort_common	303
#define	OPTION_format	304
#define	OPTION_F	305
#define	OPTION_u	306
#define	OPTION_Bstatic	307
#define	OPTION_N	308
#define	SIZEOF	309
#define	NEXT	310
#define	ADDR	311
#define	OPTION_d	312
#define	OPTION_dc	313
#define	OPTION_dp	314
#define	OPTION_x	315
#define	OPTION_X	316
#define	OPTION_defsym	317
#define	OPTION_v	318
#define	OPTION_V	319
#define	OPTION_M	320
#define	OPTION_t	321
#define	STARTUP	322
#define	HLL	323
#define	SYSLIB	324
#define	FLOAT	325
#define	NOFLOAT	326
#define	OPTION_n	327
#define	OPTION_r	328
#define	OPTION_o	329
#define	OPTION_b	330
#define	OPTION_R	331
#define	OPTION_l	332
#define	OPTION_L	333
#define	OPTION_T	334
#define	OPTION_Aarch	335
#define	OPTION_Tfile	336
#define	OPTION_Texp	337
#define	OPTION_Ur	338
#define	ORIGIN	339
#define	FILL	340
#define	OPTION_g	341
#define	LENGTH	342
#define	CREATE_OBJECT_SYMBOLS	343
#define	INPUT	344
#define	OUTPUT	345
#define	CONSTRUCTORS	346

#line 21 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"

/*
 * $Id: ldgram.y,v 1.27 1991/11/11 17:59:55 sac Exp $
 */

#define DONTDECLARE_MALLOC

#include "bfd.h"
#include "sysdep.h"
#include "ld.h"    
#include "ldexp.h"
#include "ldver.h"
#include "ldlang.h"
#include "ldemul.h"
#include "ldfile.h"
#include "ldmisc.h"


int yylex(void);
#define YYDEBUG 1

boolean option_v;
extern unsigned int lineno;
extern boolean trace_files;
extern boolean write_map;
extern boolean option_longmap;
boolean hex_mode;

strip_symbols_type strip_symbols=STRIP_NONE;
discard_locals_type discard_locals=DISCARD_NONE;


lang_memory_region_type *region;


lang_memory_region_type *lang_memory_region_lookup();
lang_output_section_statement_type *lang_output_section_statement_lookup();

#ifdef __STDC__

void lang_add_data(int type, union etree_union *exp);
void lang_enter_output_section_statement(char *output_section_statement_name, etree_type *address_exp, int flags, bfd_vma block_value);

#else

void lang_add_data();
void lang_enter_output_section_statement();

#endif /* __STDC__ */

extern args_type command_line;
char *current_file;
boolean ldgram_want_filename = true;
boolean had_script = false;
boolean force_make_executable = false;

boolean ldgram_in_script = false;
boolean ldgram_had_equals = false;
/* LOCALS */





#line 85 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
typedef union {
  bfd_vma integer;
  int voidval;
  char *name;
  int token;
  union etree_union *etree;
  asection *section;
  struct lang_output_section_statement_struct *output_section_statement;
  union  lang_statement_union **statement_ptr;
  int lineno;
  struct {
    FILE *file;
    char *name;
    unsigned int lineno;
  } state;

  
} YYSTYPE;
#line 150 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"

ld_config_type config;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __STDC__
#define const
#endif



#define	YYFINAL		325
#define	YYFLAG		-32768
#define	YYNTBASE	115

#define YYTRANSLATE(x) ((unsigned)(x) <= 346 ? yytranslate[x] : 167)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,   113,     2,     2,     2,    33,    20,     2,    35,
   109,    31,    29,   110,    30,     2,    32,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    15,   108,    23,
     9,    24,    14,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
   111,     2,   112,    19,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    42,    18,    43,   114,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,    10,    11,    12,    13,    16,    17,    21,
    22,    25,    26,    27,    28,    34,    36,    37,    38,    39,
    40,    41,    44,    45,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
    62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
    72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
    82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
    92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
   102,   103,   104,   105,   106,   107
};

static const short yyprhs[] = {     0,
     0,     2,     4,     7,     8,     9,    10,    16,    18,    20,
    22,    24,    26,    28,    30,    32,    34,    37,    39,    41,
    44,    47,    49,    51,    53,    55,    57,    59,    61,    63,
    66,    67,    71,    73,    76,    78,    80,    82,    83,    88,
    89,    93,    94,    99,   101,   104,   105,   111,   114,   115,
   119,   122,   123,   125,   127,   129,   131,   133,   135,   137,
   139,   144,   149,   154,   159,   164,   166,   171,   176,   178,
   182,   185,   190,   193,   196,   197,   202,   205,   207,   211,
   213,   214,   219,   220,   226,   227,   233,   237,   240,   243,
   246,   249,   255,   261,   262,   264,   266,   268,   271,   272,
   274,   276,   278,   280,   282,   284,   286,   288,   290,   292,
   296,   300,   302,   303,   309,   312,   316,   317,   318,   326,
   330,   334,   338,   339,   344,   349,   353,   357,   359,   364,
   368,   369,   371,   373,   376,   380,   385,   388,   391,   394,
   398,   402,   406,   410,   414,   418,   422,   426,   430,   434,
   438,   442,   446,   450,   454,   458,   464,   468,   472,   477,
   479,   481,   486,   491,   496,   498,   499,   512,   516,   520,
   524,   528,   532,   533,   534,   536,   537,   542,   543,   546
};

static const short yyrhs[] = {   117,
     0,     4,     0,   117,   118,     0,     0,     0,     0,    42,
   119,   128,   120,    43,     0,    68,     0,    79,     0,    80,
     0,    82,     0,    81,     0,    88,     0,    69,     0,    62,
     0,    63,     0,    67,     4,     0,    89,     0,    99,     0,
    90,   116,     0,    59,     4,     0,    77,     0,    76,     0,
    61,     0,    64,     0,    73,     0,    74,     0,   102,     0,
    75,     0,    65,     4,     0,     0,    98,   121,     3,     0,
    96,     0,    91,     4,     0,    94,     0,    66,     0,     4,
     0,     0,    60,   116,   122,   126,     0,     0,    97,   123,
   126,     0,     0,    95,   116,   124,   126,     0,    93,     0,
    92,   116,     0,     0,    78,   125,     4,     9,   159,     0,
    30,     4,     0,     0,   127,   128,    43,     0,   128,   129,
     0,     0,   146,     0,   131,     0,   153,     0,   154,     0,
   156,     0,   158,     0,   133,     0,   108,     0,    55,    35,
     4,   109,     0,    56,    35,   116,   109,     0,   106,    35,
   116,   109,     0,    45,    35,     4,   109,     0,    47,    35,
     4,   109,     0,    46,     0,   105,    35,   130,   109,     0,
    57,    35,   116,   109,     0,     4,     0,   130,   110,     4,
     0,   130,     4,     0,    41,    42,   132,    43,     0,   132,
   160,     0,   132,   133,     0,     0,    58,    35,     4,   109,
     0,   144,   143,     0,     4,     0,   134,   145,     4,     0,
     4,     0,     0,   111,   136,   134,   112,     0,     0,     4,
   137,    35,   134,   109,     0,     0,    31,   138,    35,   134,
   109,     0,   139,   144,   143,     0,   139,   104,     0,   139,
   108,     0,   139,   107,     0,   139,   135,     0,   139,   140,
    35,   159,   109,     0,   139,   101,    35,   159,   109,     0,
     0,    38,     0,    39,     0,    40,     0,     9,   159,     0,
     0,     5,     0,     6,     0,     7,     0,     8,     0,    10,
     0,    11,     0,    12,     0,    13,     0,   108,     0,   110,
     0,     4,     9,   159,     0,     4,   142,   159,     0,   110,
     0,     0,    48,    42,   148,   147,    43,     0,   147,   148,
     0,   147,   110,   148,     0,     0,     0,     4,   149,   152,
    15,   150,   145,   151,     0,   100,     9,   159,     0,   103,
     9,   159,     0,    35,     4,   109,     0,     0,    83,    35,
   116,   109,     0,    84,    35,   155,   109,     0,    84,    35,
   109,     0,   155,   145,   116,     0,   116,     0,    85,    35,
   157,   109,     0,   157,   145,   116,     0,     0,    86,     0,
    87,     0,    30,   159,     0,    35,   159,   109,     0,    71,
    35,   159,   109,     0,   113,   159,     0,    29,   159,     0,
   114,   159,     0,   159,    31,   159,     0,   159,    32,   159,
     0,   159,    33,   159,     0,   159,    29,   159,     0,   159,
    30,   159,     0,   159,    27,   159,     0,   159,    28,   159,
     0,   159,    21,   159,     0,   159,    22,   159,     0,   159,
    25,   159,     0,   159,    26,   159,     0,   159,    23,   159,
     0,   159,    24,   159,     0,   159,    20,   159,     0,   159,
    19,   159,     0,   159,    18,   159,     0,   159,    14,   159,
    15,   159,     0,   159,    17,   159,     0,   159,    16,   159,
     0,    54,    35,     4,   109,     0,     3,     0,    44,     0,
    70,    35,     4,   109,     0,    72,    35,     4,   109,     0,
    36,    35,   159,   109,     0,     4,     0,     0,     4,   164,
   162,   165,    15,   163,    42,   161,   139,    43,   141,   166,
     0,    35,    49,   109,     0,    35,    50,   109,     0,    35,
    51,   109,     0,    35,    52,   109,     0,    35,    53,   109,
     0,     0,     0,   159,     0,     0,    37,    35,   159,   109,
     0,     0,    24,     4,     0,     0
};

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   158,   161,   164,   166,   169,   173,   175,   175,   176,   181,
   186,   189,   196,   199,   203,   206,   209,   213,   219,   225,
   229,   232,   235,   239,   243,   246,   249,   253,   257,   261,
   265,   270,   275,   279,   283,   287,   291,   294,   296,   296,
   299,   299,   302,   302,   308,   314,   317,   322,   334,   336,
   342,   344,   349,   351,   352,   353,   354,   355,   356,   357,
   358,   360,   362,   364,   366,   368,   370,   371,   375,   379,
   382,   387,   391,   393,   394,   397,   400,   403,   406,   410,
   415,   420,   421,   425,   426,   430,   433,   435,   438,   439,
   443,   444,   449,   457,   460,   463,   465,   469,   477,   482,
   485,   487,   489,   491,   493,   495,   497,   502,   502,   506,
   511,   519,   520,   523,   527,   529,   530,   534,   537,   544,
   550,   559,   564,   568,   573,   575,   579,   582,   587,   590,
   593,   596,   599,   606,   609,   611,   613,   615,   617,   620,
   622,   624,   626,   628,   630,   632,   634,   636,   638,   640,
   642,   644,   646,   648,   650,   652,   654,   656,   658,   660,
   662,   665,   667,   669,   671,   678,   682,   689,   691,   692,
   693,   694,   695,   698,   708,   711,   714,   721,   724,   727
};

static const char * const yytname[] = {   "$","error","$illegal.","INT","NAME",
"PLUSEQ","MINUSEQ","MULTEQ","DIVEQ","'='","LSHIFTEQ","RSHIFTEQ","ANDEQ","OREQ",
"'?'","':'","OROR","ANDAND","'|'","'^'","'&'","EQ","NE","'<'","'>'","LE","GE",
"LSHIFT","RSHIFT","'+'","'-'","'*'","'/'","'%'","UNARY","'('","ALIGN_K","BLOCK",
"LONG","SHORT","BYTE","SECTIONS","'{'","'}'","SIZEOF_HEADERS","OUTPUT_FORMAT",
"FORCE_COMMON_ALLOCATION","OUTPUT_ARCH","MEMORY","NOLOAD","DSECT","COPY","INFO",
"OVERLAY","DEFINED","TARGET_K","SEARCH_DIR","MAP","ENTRY","OPTION_e","OPTION_c",
"OPTION_noinhibit_exec","OPTION_s","OPTION_S","OPTION_sort_common","OPTION_format",
"OPTION_F","OPTION_u","OPTION_Bstatic","OPTION_N","SIZEOF","NEXT","ADDR","OPTION_d",
"OPTION_dc","OPTION_dp","OPTION_x","OPTION_X","OPTION_defsym","OPTION_v","OPTION_V",
"OPTION_M","OPTION_t","STARTUP","HLL","SYSLIB","FLOAT","NOFLOAT","OPTION_n",
"OPTION_r","OPTION_o","OPTION_b","OPTION_R","OPTION_l","OPTION_L","OPTION_T",
"OPTION_Aarch","OPTION_Tfile","OPTION_Texp","OPTION_Ur","ORIGIN","FILL","OPTION_g",
"LENGTH","CREATE_OBJECT_SYMBOLS","INPUT","OUTPUT","CONSTRUCTORS","';'","')'",
"','","'['","']'","'!'","'~'","file","filename","command_line","command_line_option",
"@1","@2","@3","@4","@5","@6","@7","script_file","@8","ifile_list","ifile_p1",
"input_list","sections","sec_or_group_p1","statement_anywhere","file_NAME_list",
"input_section_spec","@9","@10","@11","statement","length","fill_opt","assign_op",
"end","assignment","opt_comma","memory","memory_spec_list","memory_spec","@12",
"origin_spec","length_spec","attributes_opt","startup","high_level_library",
"high_level_library_NAME_list","low_level_library","low_level_library_NAME_list",
"floating_point_support","exp","section","@13","opt_type","opt_things","opt_exp",
"opt_block","memspec_opt",""
};
#endif

static const short yyr1[] = {     0,
   115,   116,   117,   117,   119,   120,   118,   118,   118,   118,
   118,   118,   118,   118,   118,   118,   118,   118,   118,   118,
   118,   118,   118,   118,   118,   118,   118,   118,   118,   118,
   121,   118,   118,   118,   118,   118,   118,   122,   118,   123,
   118,   124,   118,   118,   118,   125,   118,   118,   127,   126,
   128,   128,   129,   129,   129,   129,   129,   129,   129,   129,
   129,   129,   129,   129,   129,   129,   129,   129,   130,   130,
   130,   131,   132,   132,   132,   133,   133,   134,   134,   135,
   136,   135,   137,   135,   138,   135,   139,   139,   139,   139,
   139,   139,   139,   139,   140,   140,   140,   141,   141,   142,
   142,   142,   142,   142,   142,   142,   142,   143,   143,   144,
   144,   145,   145,   146,   147,   147,   147,   149,   148,   150,
   151,   152,   152,   153,   154,   154,   155,   155,   156,   157,
   157,   158,   158,   159,   159,   159,   159,   159,   159,   159,
   159,   159,   159,   159,   159,   159,   159,   159,   159,   159,
   159,   159,   159,   159,   159,   159,   159,   159,   159,   159,
   159,   159,   159,   159,   159,   161,   160,   162,   162,   162,
   162,   162,   162,   163,   164,   164,   165,   165,   166,   166
};

static const short yyr2[] = {     0,
     1,     1,     2,     0,     0,     0,     5,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     2,     1,     1,     2,
     2,     1,     1,     1,     1,     1,     1,     1,     1,     2,
     0,     3,     1,     2,     1,     1,     1,     0,     4,     0,
     3,     0,     4,     1,     2,     0,     5,     2,     0,     3,
     2,     0,     1,     1,     1,     1,     1,     1,     1,     1,
     4,     4,     4,     4,     4,     1,     4,     4,     1,     3,
     2,     4,     2,     2,     0,     4,     2,     1,     3,     1,
     0,     4,     0,     5,     0,     5,     3,     2,     2,     2,
     2,     5,     5,     0,     1,     1,     1,     2,     0,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
     3,     1,     0,     5,     2,     3,     0,     0,     7,     3,
     3,     3,     0,     4,     4,     3,     3,     1,     4,     3,
     0,     1,     1,     2,     3,     4,     2,     2,     2,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     5,     3,     3,     4,     1,
     1,     4,     4,     4,     1,     0,    12,     3,     3,     3,
     3,     3,     0,     0,     1,     0,     4,     0,     2,     0
};

static const short yydefact[] = {     4,
     1,    37,     0,     5,     0,     0,    24,    15,    16,    25,
     0,    36,     0,     8,    14,    26,    27,    29,    23,    22,
    46,     9,    10,    12,    11,    13,    18,     0,     0,     0,
    44,    35,     0,    33,    40,    31,    19,    28,     3,    48,
    52,    21,     2,    38,    30,    17,     0,    20,    34,    45,
    42,    49,     0,     6,    49,     0,    49,    41,    52,    32,
     0,     0,     0,    66,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   132,   133,     0,     0,    60,     0,    51,
    54,    59,     0,    53,    55,    56,    57,    58,    39,     0,
    43,     0,   100,   101,   102,   103,     0,   104,   105,   106,
   107,     0,    75,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   131,     0,     0,     7,   108,   109,    77,   160,
   165,     0,     0,     0,     0,   161,     0,     0,     0,     0,
     0,     0,    47,    50,   110,   111,     0,     0,     0,   118,
   117,     0,     0,     0,     0,     0,   126,   128,   113,   113,
    69,     0,     0,   138,   134,     0,     0,     0,     0,     0,
     0,   137,   139,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   176,    72,    74,    73,    64,    65,   123,     0,
    61,    62,    68,    76,   124,   125,   112,     0,   129,     0,
    71,    67,     0,    63,   135,     0,     0,     0,     0,     0,
     0,   158,   157,   155,   154,   153,   147,   148,   151,   152,
   149,   150,   145,   146,   143,   144,   140,   141,   142,   175,
   173,     0,     0,   114,     0,   115,   127,   130,    70,   164,
   159,   162,   136,   163,     0,     0,   178,     0,     0,   116,
   156,     0,     0,     0,     0,     0,     0,     0,   122,     0,
   113,   168,   169,   170,   171,   172,     0,   174,     0,     0,
     0,     0,   120,     0,   119,   177,   166,     0,    94,   121,
     0,    80,    85,    95,    96,    97,    99,     0,    88,    90,
    89,    81,    91,     0,     0,     0,     0,     0,   180,     0,
     0,     0,    87,     0,     0,    98,     0,   167,     0,    78,
   113,     0,   113,   113,   179,    93,    82,     0,    92,    84,
    86,    79,     0,     0,     0
};

static const short yydefgoto[] = {   323,
    44,     1,    39,    41,    79,    53,    55,    52,    57,    47,
    58,    59,    54,    80,   152,    81,   137,    82,   311,   293,
   301,   296,   297,   281,   294,   299,   102,   119,    83,   318,
    84,   190,   141,   189,   261,   275,   233,    85,    86,   149,
    87,   150,    88,   133,   186,   279,   247,   272,   231,   258,
   308
};

static const short yypact[] = {-32768,
   272,-32768,    24,-32768,    62,    85,-32768,-32768,-32768,-32768,
    88,-32768,   103,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,    85,   108,    85,
-32768,-32768,    85,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   112,-32768,-32768,-32768,
-32768,-32768,   116,   222,-32768,   111,-32768,-32768,-32768,-32768,
   546,    79,    89,-32768,    90,    84,    92,    95,    96,    97,
    98,    99,   100,-32768,-32768,   101,   105,-32768,   102,-32768,
-32768,-32768,   -49,-32768,-32768,-32768,-32768,-32768,-32768,   114,
-32768,   167,-32768,-32768,-32768,-32768,   114,-32768,-32768,-32768,
-32768,   114,-32768,   134,   137,   138,   142,    85,    85,   147,
    85,     2,-32768,   148,    85,-32768,-32768,-32768,-32768,-32768,
-32768,   114,   114,   114,   118,-32768,   119,   120,   121,   122,
   114,   114,   475,-32768,   475,   475,     4,    50,    51,-32768,
-32768,    52,    53,    58,    60,    67,-32768,-32768,   -86,   -68,
-32768,    -1,    71,-32768,-32768,   267,   114,   178,   179,   114,
   196,-32768,-32768,   114,   114,   114,   114,   114,   114,   114,
   114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
   114,   114,    93,-32768,-32768,-32768,-32768,-32768,   169,     0,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,    85,-32768,    85,
-32768,-32768,   198,-32768,-32768,   361,   107,   109,   381,   110,
   517,   494,   547,   562,   435,   166,   292,   292,    21,    21,
    21,    21,     7,     7,    25,    25,-32768,-32768,-32768,   475,
   174,   207,   202,-32768,   138,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   114,   -20,   183,   123,   129,-32768,
   475,   124,   125,   126,   128,   131,   186,   215,-32768,   229,
   139,-32768,-32768,-32768,-32768,-32768,   114,-32768,   114,   141,
   401,   189,   475,   232,-32768,-32768,-32768,   114,-32768,   475,
   135,     9,-32768,-32768,-32768,-32768,   236,   212,-32768,-32768,
-32768,-32768,-32768,   213,   -49,   220,   221,   114,   233,   114,
   254,   114,-32768,   254,   254,   475,   255,-32768,   421,-32768,
   -99,   455,   -64,   -45,-32768,-32768,-32768,   256,-32768,-32768,
-32768,-32768,   261,   262,-32768
};

static const short yypgoto[] = {-32768,
   -21,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    38,-32768,   205,-32768,-32768,-32768,-32768,   145,  -190,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -30,   -15,  -148,
-32768,-32768,  -180,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,   -97,-32768,-32768,-32768,-32768,-32768,-32768,
-32768
};


#define	YYLAST		595


static const short yytable[] = {   135,
   198,   200,   201,   140,   136,    43,    48,   183,    50,   236,
   197,    51,   317,    93,    94,    95,    96,    97,    98,    99,
   100,   101,   196,   197,   154,   155,   156,    40,   252,   253,
   254,   255,   256,   162,   163,   178,   179,   180,   181,   182,
   199,   197,   234,   -83,   320,   197,   184,   176,   177,   178,
   179,   180,   181,   182,   250,   180,   181,   182,   117,   206,
   118,    70,   209,   321,   197,    42,   211,   212,   213,   214,
   215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
   225,   226,   227,   228,   229,   230,   143,   144,    43,   146,
   148,    45,    89,   153,    91,   120,   121,    93,    94,    95,
    96,    97,    98,    99,   100,   101,    46,   202,   203,   235,
   147,    49,   270,   313,   314,    56,   120,   121,    60,    90,
   103,   122,   123,   104,   105,   106,   107,   124,   125,   108,
   109,   110,   111,   112,   113,   114,   126,   138,   282,   115,
   139,   140,   122,   123,   116,   142,   127,   251,   124,   125,
   145,   151,   157,   158,   159,   160,   161,   126,   187,   188,
   191,   192,   128,   129,   130,   283,   193,   127,   194,   271,
    61,   273,   284,   285,   286,   195,   237,   287,   238,   204,
   280,   207,   208,   128,   129,   130,   170,   171,   172,   173,
   174,   175,   176,   177,   178,   179,   180,   181,   182,   210,
   306,   239,   309,   232,   312,   131,   132,    62,   246,   134,
   248,    63,    64,    65,    66,   241,   249,   242,   244,   257,
   267,    67,    68,    69,    70,    61,   131,   132,   260,   268,
   277,   259,   262,   263,   264,   288,   265,   269,   289,   266,
   278,   290,   291,   274,   298,   292,   300,   302,   197,    71,
    72,    73,    74,    75,   304,   305,   307,   310,   315,   322,
   324,   325,    62,    92,   303,   295,    63,    64,    65,    66,
     0,    76,    77,     0,    78,     2,    67,    68,    69,    70,
   164,   185,   165,   166,   167,   168,   169,   170,   171,   172,
   173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     0,     3,     0,     0,    71,    72,    73,    74,    75,     0,
     0,     0,     0,     4,   172,   173,   174,   175,   176,   177,
   178,   179,   180,   181,   182,     0,    76,    77,     0,    78,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,     0,     0,     0,    16,    17,    18,    19,    20,    21,
    22,    23,    24,    25,     0,     0,     0,     0,     0,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,     0,     0,    38,   164,   205,   165,   166,   167,   168,
   169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
   179,   180,   181,   182,   164,     0,   165,   166,   167,   168,
   169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
   179,   180,   181,   182,   164,     0,   165,   166,   167,   168,
   169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
   179,   180,   181,   182,   164,     0,   165,   166,   167,   168,
   169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
   179,   180,   181,   182,   169,   170,   171,   172,   173,   174,
   175,   176,   177,   178,   179,   180,   181,   182,   164,   240,
   165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
   175,   176,   177,   178,   179,   180,   181,   182,   164,   243,
   165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
   175,   176,   177,   178,   179,   180,   181,   182,     0,   276,
   166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
   176,   177,   178,   179,   180,   181,   182,     0,     0,   316,
   164,   245,   165,   166,   167,   168,   169,   170,   171,   172,
   173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
    93,    94,    95,    96,    97,    98,    99,   100,   101,     0,
     0,     0,     0,   319,   167,   168,   169,   170,   171,   172,
   173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
   168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
   178,   179,   180,   181,   182
};

static const short yycheck[] = {    97,
   149,   150,     4,     4,   102,     4,    28,     4,    30,   190,
   110,    33,   112,     5,     6,     7,     8,     9,    10,    11,
    12,    13,   109,   110,   122,   123,   124,     4,    49,    50,
    51,    52,    53,   131,   132,    29,    30,    31,    32,    33,
   109,   110,    43,    35,   109,   110,    43,    27,    28,    29,
    30,    31,    32,    33,   235,    31,    32,    33,   108,   157,
   110,    58,   160,   109,   110,     4,   164,   165,   166,   167,
   168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
   178,   179,   180,   181,   182,   183,   108,   109,     4,   111,
   112,     4,    55,   115,    57,     3,     4,     5,     6,     7,
     8,     9,    10,    11,    12,    13,     4,   109,   110,   110,
   109,     4,   261,   304,   305,     4,     3,     4,     3,     9,
    42,    29,    30,    35,    35,    42,    35,    35,    36,    35,
    35,    35,    35,    35,    35,    35,    44,     4,     4,    35,
     4,     4,    29,    30,    43,     4,    54,   245,    35,    36,
     4,     4,    35,    35,    35,    35,    35,    44,   109,   109,
   109,   109,    70,    71,    72,    31,   109,    54,   109,   267,
     4,   269,    38,    39,    40,   109,   198,    43,   200,   109,
   278,     4,     4,    70,    71,    72,    21,    22,    23,    24,
    25,    26,    27,    28,    29,    30,    31,    32,    33,     4,
   298,     4,   300,    35,   302,   113,   114,    41,    35,    43,
     4,    45,    46,    47,    48,   109,    15,   109,   109,    37,
    35,    55,    56,    57,    58,     4,   113,   114,   100,    15,
    42,   109,   109,   109,   109,   101,   109,     9,   104,   109,
     9,   107,   108,   103,     9,   111,    35,    35,   110,    83,
    84,    85,    86,    87,    35,    35,    24,     4,     4,     4,
     0,     0,    41,    59,   295,   281,    45,    46,    47,    48,
    -1,   105,   106,    -1,   108,     4,    55,    56,    57,    58,
    14,   137,    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    -1,    30,    -1,    -1,    83,    84,    85,    86,    87,    -1,
    -1,    -1,    -1,    42,    23,    24,    25,    26,    27,    28,
    29,    30,    31,    32,    33,    -1,   105,   106,    -1,   108,
    59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
    69,    -1,    -1,    -1,    73,    74,    75,    76,    77,    78,
    79,    80,    81,    82,    -1,    -1,    -1,    -1,    -1,    88,
    89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
    99,    -1,    -1,   102,    14,   109,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    33,    14,    -1,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    33,    14,    -1,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    33,    14,    -1,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    33,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    14,   109,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    14,   109,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    -1,   109,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    -1,    -1,   109,
    14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    -1,
    -1,    -1,    -1,   109,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
    29,    30,    31,    32,    33
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 169 "bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yysp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yysp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
#ifdef YYLSP_NEEDED
		 &yyls1, size * sizeof (*yylsp),
#endif
		 &yystacksize);

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symboles being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 158 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_final(); ;
    break;}
case 5:
#line 171 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldgram_in_script = true; ;
    break;}
case 6:
#line 173 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldgram_in_script = false; ;
    break;}
case 8:
#line 175 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ;
    break;}
case 9:
#line 177 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{	
			ldversion();
			option_v = true;
			;
    break;}
case 10:
#line 182 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{	
			ldversion();
			option_v = true;
			;
    break;}
case 11:
#line 186 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			trace_files = true;
			;
    break;}
case 12:
#line 189 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			if (write_map) {
			    option_longmap = true;
			}
			write_map = true;

			;
    break;}
case 13:
#line 196 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			config.magic_demand_paged = false;
			;
    break;}
case 14:
#line 199 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			config.text_read_only = false;
			config.magic_demand_paged = false;
	                ;
    break;}
case 15:
#line 203 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
	  		strip_symbols = STRIP_ALL;
			;
    break;}
case 16:
#line 206 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			strip_symbols = STRIP_DEBUGGER;
			;
    break;}
case 17:
#line 209 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			ldlang_add_undef(yyvsp[0].name);
	      	;
    break;}
case 18:
#line 213 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			config.relocateable_output = true;
			config.build_constructors = false;
			config.magic_demand_paged = false;
 			config.text_read_only = false;
			;
    break;}
case 19:
#line 219 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			config.relocateable_output = true;
			config.build_constructors = true;
			config.magic_demand_paged = false;
 			config.text_read_only = false;
		      ;
    break;}
case 20:
#line 226 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			lang_add_output(yyvsp[0].name); 
			;
    break;}
case 21:
#line 230 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_entry(yyvsp[0].name); 
			;
    break;}
case 22:
#line 232 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			discard_locals = DISCARD_L;
		;
    break;}
case 23:
#line 235 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			discard_locals = DISCARD_ALL;
		;
    break;}
case 24:
#line 240 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			force_make_executable = true;
			;
    break;}
case 25:
#line 243 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
	config.sort_common = true;
      ;
    break;}
case 26:
#line 246 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			  command_line.force_common_definition = true;
			;
    break;}
case 27:
#line 250 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			  command_line.force_common_definition = true;
			;
    break;}
case 28:
#line 254 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			/* Ignored */
			;
    break;}
case 29:
#line 258 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			  command_line.force_common_definition = true;
			;
    break;}
case 30:
#line 262 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			  lang_add_target(yyvsp[0].name);
       		   ;
    break;}
case 31:
#line 266 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ 
			hex_mode  =true; 
		;
    break;}
case 32:
#line 270 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ 
			lang_section_start(yyvsp[-2].name,exp_intop(yyvsp[0].integer));
			hex_mode = false; 
		;
    break;}
case 33:
#line 276 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ 
			ldfile_add_arch(yyvsp[0].name); 
		;
    break;}
case 34:
#line 280 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			lang_add_target(yyvsp[0].name);
			;
    break;}
case 35:
#line 284 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			ldfile_add_library_path(yyvsp[0].name);
			;
    break;}
case 36:
#line 288 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		/* Ignore */
		;
    break;}
case 37:
#line 292 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_input_file(yyvsp[0].name,lang_input_file_is_file_enum,
				 (char *)NULL); ;
    break;}
case 38:
#line 295 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldfile_open_command_file(yyvsp[0].name); ;
    break;}
case 40:
#line 297 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldfile_open_command_file(yyvsp[0].name); ;
    break;}
case 42:
#line 300 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldfile_open_command_file(yyvsp[0].name); ;
    break;}
case 44:
#line 303 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			  lang_add_input_file(yyvsp[0].name,
					 lang_input_file_is_l_enum,
					 (char *)NULL);
			;
    break;}
case 45:
#line 309 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			lang_add_input_file(yyvsp[0].name,
				lang_input_file_is_symbols_only_enum,
				(char *)NULL);
			;
    break;}
case 46:
#line 315 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			;
    break;}
case 47:
#line 319 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			lang_add_assignment(exp_assop(yyvsp[-1].token,yyvsp[-2].name,yyvsp[0].etree));
			;
    break;}
case 48:
#line 323 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ info("%P%F Unrecognised option -%s\n", yyvsp[0].name);  ;
    break;}
case 49:
#line 335 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldgram_in_script = true; ;
    break;}
case 50:
#line 337 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldgram_in_script = false; ;
    break;}
case 61:
#line 359 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_target(yyvsp[-1].name); ;
    break;}
case 62:
#line 361 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldfile_add_library_path(yyvsp[-1].name); ;
    break;}
case 63:
#line 363 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_output(yyvsp[-1].name); ;
    break;}
case 64:
#line 365 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_output_format(yyvsp[-1].name); ;
    break;}
case 65:
#line 367 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldfile_set_output_arch(yyvsp[-1].name); ;
    break;}
case 66:
#line 369 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ command_line.force_common_definition = true ; ;
    break;}
case 68:
#line 372 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_map(yyvsp[-1].name); ;
    break;}
case 69:
#line 377 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_input_file(yyvsp[0].name,lang_input_file_is_file_enum,
				 (char *)NULL); ;
    break;}
case 70:
#line 380 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_input_file(yyvsp[0].name,lang_input_file_is_file_enum,
				 (char *)NULL); ;
    break;}
case 71:
#line 383 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_input_file(yyvsp[0].name, lang_input_file_is_file_enum,
				 (char *)NULL); ;
    break;}
case 76:
#line 399 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_entry(yyvsp[-1].name); ;
    break;}
case 78:
#line 405 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_wild(yyvsp[0].name, current_file); ;
    break;}
case 79:
#line 407 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_add_wild(yyvsp[0].name, current_file); ;
    break;}
case 80:
#line 412 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		lang_add_wild((char *)NULL, yyvsp[0].name);
		;
    break;}
case 81:
#line 416 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			current_file = (char *)NULL;
			;
    break;}
case 83:
#line 422 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			current_file  =yyvsp[0].name;
			;
    break;}
case 85:
#line 427 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{	
			current_file = (char *)NULL;
			;
    break;}
case 88:
#line 436 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
 		  lang_add_attribute(lang_object_symbols_statement_enum); ;
    break;}
case 90:
#line 440 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
 		  lang_add_attribute(lang_constructors_statement_enum); ;
    break;}
case 92:
#line 445 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			lang_add_data(yyvsp[-3].integer,yyvsp[-1].etree);
			;
    break;}
case 93:
#line 450 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			  lang_add_fill
			    (exp_get_value_int(yyvsp[-1].etree,
					       0,
					       "fill value",
					       lang_first_phase_enum));
			;
    break;}
case 95:
#line 462 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = yyvsp[0].token; ;
    break;}
case 96:
#line 464 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = yyvsp[0].token; ;
    break;}
case 97:
#line 466 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = yyvsp[0].token; ;
    break;}
case 98:
#line 471 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		  yyval.integer =	 exp_get_value_int(yyvsp[0].etree,
					   0,
					   "fill value",
					   lang_first_phase_enum); 
		;
    break;}
case 99:
#line 477 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{  yyval.integer = 0; ;
    break;}
case 100:
#line 484 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = '+'; ;
    break;}
case 101:
#line 486 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = '-'; ;
    break;}
case 102:
#line 488 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = '*'; ;
    break;}
case 103:
#line 490 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = '/'; ;
    break;}
case 104:
#line 492 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = LSHIFT; ;
    break;}
case 105:
#line 494 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = RSHIFT; ;
    break;}
case 106:
#line 496 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = '&'; ;
    break;}
case 107:
#line 498 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.token = '|'; ;
    break;}
case 110:
#line 508 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		  lang_add_assignment(exp_assop(yyvsp[-1].token,yyvsp[-2].name,yyvsp[0].etree));
		;
    break;}
case 111:
#line 512 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		  lang_add_assignment(exp_assop('=',yyvsp[-2].name,exp_binop(yyvsp[-1].token,exp_nameop(NAME,yyvsp[-2].name),yyvsp[0].etree)));
		;
    break;}
case 118:
#line 536 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ region = lang_memory_region_lookup(yyvsp[0].name); ;
    break;}
case 119:
#line 539 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		 

		;
    break;}
case 120:
#line 546 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ region->current =
		 region->origin =
		 exp_get_vma(yyvsp[0].etree, 0L,"origin", lang_first_phase_enum); ;
    break;}
case 121:
#line 552 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{  region->length = exp_get_vma(yyvsp[0].etree,
					       ~((bfd_vma)0),
					       "length",
					       lang_first_phase_enum);
		;
    break;}
case 122:
#line 561 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
			lang_set_flags(&region->flags, yyvsp[-1].name);
			;
    break;}
case 124:
#line 570 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_startup(yyvsp[-1].name); ;
    break;}
case 126:
#line 576 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldemul_hll((char *)NULL); ;
    break;}
case 127:
#line 581 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldemul_hll(yyvsp[0].name); ;
    break;}
case 128:
#line 583 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldemul_hll(yyvsp[0].name); ;
    break;}
case 130:
#line 592 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ ldemul_syslib(yyvsp[0].name); ;
    break;}
case 132:
#line 598 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_float(true); ;
    break;}
case 133:
#line 600 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ lang_float(false); ;
    break;}
case 134:
#line 608 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_unop('-', yyvsp[0].etree); ;
    break;}
case 135:
#line 610 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = yyvsp[-1].etree; ;
    break;}
case 136:
#line 612 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_unop(yyvsp[-3].integer,yyvsp[-1].etree); ;
    break;}
case 137:
#line 614 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_unop('!', yyvsp[0].etree); ;
    break;}
case 138:
#line 616 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = yyvsp[0].etree; ;
    break;}
case 139:
#line 618 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_unop('~', yyvsp[0].etree);;
    break;}
case 140:
#line 621 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('*', yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 141:
#line 623 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('/', yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 142:
#line 625 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('%', yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 143:
#line 627 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('+', yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 144:
#line 629 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('-' , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 145:
#line 631 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(LSHIFT , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 146:
#line 633 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(RSHIFT , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 147:
#line 635 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(EQ , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 148:
#line 637 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(NE , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 149:
#line 639 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(LE , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 150:
#line 641 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(GE , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 151:
#line 643 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('<' , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 152:
#line 645 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('>' , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 153:
#line 647 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('&' , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 154:
#line 649 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('^' , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 155:
#line 651 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop('|' , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 156:
#line 653 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_trinop('?' , yyvsp[-4].etree, yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 157:
#line 655 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(ANDAND , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 158:
#line 657 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_binop(OROR , yyvsp[-2].etree, yyvsp[0].etree); ;
    break;}
case 159:
#line 659 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_nameop(DEFINED, yyvsp[-1].name); ;
    break;}
case 160:
#line 661 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_intop(yyvsp[0].integer); ;
    break;}
case 161:
#line 663 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_nameop(SIZEOF_HEADERS,0); ;
    break;}
case 162:
#line 666 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_nameop(SIZEOF,yyvsp[-1].name); ;
    break;}
case 163:
#line 668 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_nameop(ADDR,yyvsp[-1].name); ;
    break;}
case 164:
#line 670 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_unop(ALIGN_K,yyvsp[-1].etree); ;
    break;}
case 165:
#line 672 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = exp_nameop(NAME,yyvsp[0].name); ;
    break;}
case 166:
#line 679 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		lang_enter_output_section_statement(yyvsp[-6].name,yyvsp[-5].etree,yyvsp[-4].integer,yyvsp[-3].integer);
		;
    break;}
case 167:
#line 683 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{
		  lang_leave_output_section_statement(yyvsp[-1].integer, yyvsp[0].name);
		;
    break;}
case 168:
#line 690 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = SEC_NO_FLAGS; ;
    break;}
case 169:
#line 691 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = 0; ;
    break;}
case 170:
#line 692 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = 0; ;
    break;}
case 171:
#line 693 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = 0; ;
    break;}
case 172:
#line 694 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = 0; ;
    break;}
case 173:
#line 695 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS; ;
    break;}
case 174:
#line 699 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{

	;
    break;}
case 175:
#line 710 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree = yyvsp[0].etree; ;
    break;}
case 176:
#line 711 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.etree= (etree_type *)NULL; ;
    break;}
case 177:
#line 716 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer = exp_get_value_int(yyvsp[-1].etree,
					 1L,
					 "block",
					 lang_first_phase_enum); 
		;
    break;}
case 178:
#line 721 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.integer  = 1; ;
    break;}
case 179:
#line 726 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.name = yyvsp[0].name; ;
    break;}
case 180:
#line 727 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
{ yyval.name = "*default*"; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 440 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) xmalloc(size + 15);
	  strcpy(msg, "parse error");

	  if (count < 5)
	    {
	      count = 0;
	      for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
		if (yycheck[x + yyn] == x)
		  {
		    strcat(msg, count == 0 ? ", expecting `" : " or `");
		    strcat(msg, yytname[x]);
		    strcat(msg, "'");
		    count++;
		  }
	    }
	  yyerror(msg);
	  free(msg);
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 730 "/workspace/scratch/562074e27b25/binutils-1.94-ns32k/ld/ldgram.y"
