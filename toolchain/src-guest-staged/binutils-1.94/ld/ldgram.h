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


extern YYSTYPE yylval;
