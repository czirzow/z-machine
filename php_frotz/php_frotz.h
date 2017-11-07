
#ifndef PHP_FROTZ_H
#define PHP_FROTZ_H

#include "php.h"
#include <frotz.h>

/**************************************************
 *  Some compile settings that are commonly changed
 **************************************************/
 
/* Define this if you want to be able to set you Interperator number
 * within php. */
#define FROTZ_MY_INTERP 

/* Or use this instead */
/*#define FROTZ_INTERP_NUM INTERP_MSDOS */

/**************************************************/



#define FROTZ_VERSION "0.8.0"

/* PHP required */
extern zend_module_entry frotz_module_entry;
#define frotz_module_ptr &frotz_module_entry
#define phpext_frotz_ptr frotz_module_ptr


PHP_MINFO_FUNCTION(frotz);
PHP_MINIT_FUNCTION(frotz);
PHP_RINIT_FUNCTION(frotz);
PHP_GINIT_FUNCTION(frotz);
PHP_MSHUTDOWN_FUNCTION(frotz);

PHP_FUNCTION(frotz_play);
PHP_FUNCTION(frotz_get_var);
PHP_FUNCTION(frotz_set_var);
PHP_FUNCTION(frotz_output);
PHP_FUNCTION(frotz_close);



typedef struct _frotz_char {
        zchar c;
		zchar color;
		zchar style;
		zchar font;
} frotz_char;


typedef struct {

	/* input buffer */
	char *input_buffer;        /* Input string */
	int   input_length;        /* lenfth of input string */
	int   input_index;         /* Current char we are at */

	/* output buffer */
	frotz_char *frotz_buffer;  /* Current screen   */
	frotz_char **frotz_scroll; /* Array of screens */
	int scroll_size;           /* How many screens we have */

	uint frotz_alloc;          /* Memory allocated for screen */
	uint frotz_index;          /* current index position */
	short frotz_more;          /* more flag */ 
	short frotz_state;         /* state of the play */

	short screen_rows;
	short screen_cols;
	short screen_fcolor;
	short screen_bcolor;
#ifdef FROTZ_MY_INTERP
	short frotz_interp;
#endif

	/* current attribs */
	unsigned char color;
	unsigned char style;
	unsigned char font;
	
	int read_state; /* state of reading (readln | readkey) */
	int beeps;

	/* save game */
	unsigned char *game_str;
	unsigned long game_len;

} php_frotz_globals;

#ifdef ZTS
#define FLS_D php_frotz_globals *frotz_globals
#define FLS_C frotz_globals
#define FG(v) (frotz_globals->v)
#define FLS_FETCH() php_frotz_globals *frotz_globals =  ts_resource(frotz_globals_id)
ZEND_API extern int frotz_globals_id;
#else 
#define FLS_D void
#define FLS_C
#define FG(v) (frotz_globals.v)
#define FLS_FETCH()
ZEND_API extern php_frotz_globals frotz_globals;
#endif

#define FROTZ_OUTPUT(i) (  i < FG(frotz_alloc) )

	
#define FROTZ_STATE_NULL 0x00
#define FROTZ_STATE_INIT 0x01
#define FROTZ_STATE_PLAY 0x02
#define FROTZ_STATE_MORE 0x04

#define FROTZ_CURSOR_CHAR  0x01


#if 0
extern int current_text_style;    /* ux_init */
extern char unix_plain_ascii;     /* ux_init */
extern int current_color;         /* ux_text */
#endif




/* do not remove */
#ifdef FROTZ_MY_INTERP
#define FROTZ_INTERP  FG(frotz_interp)
#else
#define FROTZ_INTERP FRTOZ_MY_INTERP_NUM
#endif


#define FROTZ_PRINTC(c) frotz_print_char(c)
#define FROTZ_PRINTC_AT(i,c) frotz_print_char_at(i,c)

 /* (row_pos * screen_width) + col_pos - 1 for 0 base index */
#define FROTZ_INDEX_OF(y,x) ( ( (y-1) * h_screen_width) + x - 1) 
#define FROTZ_INDEX_SET(y,x) { FG(frotz_index) = FROTZ_INDEX_OF(y,x); }


#define FROTZ_PUTC(c) {  FROTZ_PRINTC(c); }


#define FROTZ_READLN  0xE4
#define FROTZ_READKEY 0xF6

#define FROTZ_SAVE_SCROLL 0x01
#define FROTZ_SAVE_SETTINGS 0x02
#define FROTZ_SAVE_GAME 0x03

#define FROTZ_PRINT_PRE 0x01
#define FROTZ_PRINT_DUMP 0x02


PHPAPI char *php_escape_html_entities(unsigned char *old, int oldlen, int *newlen, int all);
int php_z_save(void);

int frotz_restore(int);
int frotz_save(int);

frotz_char *php_frotz_buf_new();
int php_frotz_buf_del();
frotz_char *php_frotz_buf_get();

int frotz_print_html_pre();
int frotz_print_html_raw();

int bufferch();

int php_frotz_restore(zval **);
char *php_frotz_save_game(int *);


int frotz_print_cursor(void);

void frotz_init_pictures (char *);


#endif /* PHP_FROTZ_H */


