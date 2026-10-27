/* File: infocom.c */
/* (C)opyright 1987 InfoTaskforce. */

#include	"infocom.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <qdos.h>
#include <sys/qlib.h>
#include <sms.h>

/*! OSAL handel to abstract QL specific definitions */
typedef struct {
  WINDOWDEF_t       windef;
  chanid_t          ch;
  unsigned short    key_code;
  int               idle_ticks;
  int               width;
  int               height;
  int               scale_x;
  int               scale_y;
} t_osal_handle;

/*! game window in QL pixels (high res has max 512x256) */
#define WINDOW_WIDTH      500
#define WINDOW_HEIGHT     250





word		random1 ;
word		random2 ;
word		pc_offset ;
word		pc_page ;
word		resident_blocks ;
word		save_blocks ;
word		status ;

header		data_head ;
object		*obj_list ;
byte		*base_ptr ;
byte		*base_end ;
byte		*vocab ;
byte		*global_ptr ;
byte		*end_resident_area ;
word		*stack_base ;
word		*stack_var_ptr ;
word		*stack ;

pg_table	*strt_page_table ;
byte		*page_strt ;
byte		*prog_block_ptr ;

/* Score Routine Variables */

word		score_time ;
byte		*s_buff_strt ;
byte		*s_buff_ptr ;

/* Input Routine Variables */

byte		*ws_buff_strt ;
byte		*end_of_sentence ;
word		num_vocab_words ;
word		vocab_entry_size ;
byte		*strt_vocab_table ;
byte		*end_vocab_table ;

/* Print Routine Variables */

ProcPtr		PrintChar ;
byte		*common_word_ptr ;
byte		*p_buff_strt ;
byte		*p_buff_ptr ;
byte		*p_buff_end ;
int		linecount;
int		mx_scrx;
int		mx_scry;
int		prt_buff_size;

/* Command Line Options */

Boolean		attribute	= false ;		/* Monitor Object Attributes  */
Boolean		echo_in		= false ;		/* Echo Input Characters      */
Boolean		head_info	= false ;		/* Print Header Information   */
Boolean		objects		= false ;		/* Print Object / Room List   */
Boolean		page_out	= false ;		/* Disable pager */
Boolean		tree		= false ;		/* Print Object / Room Tree */
Boolean		vocabulary	= false ;		/* Print Vocabulary Word List */
Boolean		xfer		= false ;		/* Monitor Object Transfers   */

Boolean		play		= true ;		/* Play the Game              */

/* Character Tables */

char	ws_table[] =	{ ' ','\t','\r','.',',','?','\0','\0' } ;

char	table[] =	{	'a','b','c','d','e','f','g','h','i','j','k','l',
						'm','n','o','p','q','r','s','t','u','v','w','x',
						'y','z','A','B','C','D','E','F','G','H','I','J',
						'K','L','M','N','O','P','Q','R','S','T','U','V',
						'W','X','Y','Z',' ',' ','0','1','2','3','4','5',
						'6','7','8','9','.',',','!','?','_','#','\'',
						'\"','/','\\','-',':','(',')','\0','\0'
					} ;



t_osal_handle* init_osal( void )
{
  t_osal_handle* p;
//  WINDOWDEF_t my_windef = { WHITE_M4, 1, COLOR_BACKGROUND, RED_M4, WINDOW_WIDTH + 4, WINDOW_HEIGHT + 2, 2, 1 };
  WINDOWDEF_t my_windef = { 
   2, /* border colour (red) */
   0, /* border width */
   0, /* paper (black) */
   7, /* ink (white) */
   512, /* width (pixel) */
   256, /* height (pixels) */
   0, /* x origin */
   0 /* y origin */
 };



  p = malloc( sizeof( t_osal_handle ) );
  if(! p )
    return NULL;

  memset( p, 0, sizeof( t_osal_handle ) );
  memcpy( & p->windef, & my_windef, sizeof( WINDOWDEF_t ) );
  p->idle_ticks = 5;

  p->ch =  ut_con ( & p->windef );
  if( p->ch < 0 ) {
    fprintf( stderr, "could not open screen error!\n" );
    free( p );
    p = NULL;
  } else {
    p->width = 100;
    p->height = 50;
    p->scale_x = WINDOW_WIDTH / p->width;
    p->scale_y = WINDOW_HEIGHT / p->height;
  }

  return p;
}


main ( argc,argv )
int		argc ;
char	*argv[] ;
{
	int			i ;
#if 0
	--argc ;
	if (( argc == 1 ) || ( argc == 2 ))
	{
		if ( argc == 2 )
		{
			if ( argv[1][0] == '-' )
			{
				i = 0 ;
				while ( argv[1][++i] != '\0' )
				{
					switch ( argv[1][i] )
					{
						case 'a' :	attribute = true ;
									break ;
						case 'e' :	echo_in = true ;
									break ;
						case 'h' :	head_info = true ;
									play = false ;
									break ;
						case 'o' :	objects = true ;
									play = false ;
									break ;
						case 'p' :	page_out = true ;
									break ;
						case 'r' :	tree = true ;
									play = false ;
									break ;
						case 't' :	xfer = true ;
									break ;
						case 'v' :	vocabulary = true ;
									play = false ;
									break ;
						default  :	printf ( "Unrecognised Option: %c\n",argv[1][i] ) ;
					}
				}
			}
			else
				usage ( argv[0] ) ;
		}
		if ( open_file ( argv[argc] ) )
		{
			init () ;
			if ( play )
				interp () ;
			else
				options ( head_info,objects,vocabulary,tree ) ;
		}
		else
			printf ( "Failed to Open File %s\n",argv[argc] ) ;
	}
	else
		usage ( argv[0] ) ;
#else
//long _stack = 20L * 1024L;
t_osal_handle* p;
char _prog_name[] = "InfoTaskForce";
p=init_osal();


		if ( open_file ( "win1_zork1_z3") )
		{
			init () ;
			if ( play )
				interp () ;
			else
				options ( head_info,objects,vocabulary,tree ) ;
		}
		else
			printf ( "Failed to Open File %s\n",argv[argc] ) ;
#endif
}

usage ( name )
char	*name ;
{
	printf ( "Usage: %s [-aehoprtv] <filename>\n",name ) ;
	exit ( 0 ) ;
}

#if 0
void _LMUL(){}
void _LDIV(){}
void _SRAND(){}
void _FCVT(){}
void _ECVT(){}
void __divsi3(){}
void __modsi3(){}
void __mulsi3(){}
#endif