/*
 * ux_frotz.h
 *
 * Unix interface, declarations
 *
 */

#ifndef _PHP_UX_FROTZ_H
#define _PHP_UX_FROTZ_H 

#include <frotz.h>

#if 0
extern int current_text_style;    /* ux_init */
extern char unix_plain_ascii;     /* ux_init */
extern int current_color;         /* ux_text */
#endif


/* Define this if you want to be able to set you Interperator number
 * within php. */
#define FROTZ_MY_INTERP 

/* Or use this instead */
/*#define FROTZ_INTERP_NUM INTERP_MSDOS */


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


int frotz_print_cursor();

#endif
