
/* $Id: */

#include "php.h"
#include "php_frotz.h"
#include "php_globals.h"
#include "ext/standard/info.h"
#include "ext/standard/base64.h"
#include "ext/standard/php_string.h"
#include "ext/standard/md5.h"

#include <frotz.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if HAVE_ZLIB
#include <zlib.h>
#endif


static unsigned char *key;
static unsigned int key_len;

static char *graph_file;

#define _FROTZ_BLACK		"#000000"
#define _FROTZ_RED			"#FF0000"
#define _FROTZ_GREEN		"#00FF00"
#define _FROTZ_BLUE			"#0000FF"
#define _FROTZ_YELLOW		"#FFFF00"
#define _FROTZ_MAGENTA		"#FF00FF"
#define _FROTZ_CYAN			"#00FFFF"
#define _FROTZ_WHITE		"#FFFFFF"
#define _FROTZ_GREY			"#808080"
#define _FROTZ_MEDIUMGREY	"#606060"
#define _FROTZ_DARKGREY		"#404040"

static char *html_colors[] = {
	"current", "default",
	_FROTZ_BLACK, _FROTZ_RED,        _FROTZ_GREEN,    _FROTZ_YELLOW,
	_FROTZ_BLUE,  _FROTZ_MAGENTA,    _FROTZ_CYAN,     _FROTZ_WHITE,
	_FROTZ_GREY,  _FROTZ_MEDIUMGREY, _FROTZ_DARKGREY};



function_entry frotz_functions[] = {
	PHP_FE(frotz_play, NULL)
	PHP_FE(frotz_close, NULL)
	PHP_FE(frotz_get_var, NULL)
	PHP_FE(frotz_set_var, NULL)
	PHP_FE(frotz_output, NULL)
	{NULL, NULL, NULL}
};


zend_module_entry frotz_module_entry = {
		"frotz",		/* extenstion*/
		frotz_functions, 	/* function list */
		PHP_MINIT(frotz),	/* PHP_MINIT(frotz) process startup */
		PHP_MSHUTDOWN(frotz),	/* PHP_MSHUTDOW process shutdown  */
		NULL,			/* PHP_RINIT request startup */
		NULL,			/* PHP_RSHUTDOWN request shutdown */
		PHP_MINFO(frotz),	/* PHP_MINFO extension info */
		PHP_GINIT(frotz),
		NULL, 			/* STANDARD_MODULE_PRPERTIES global startup function */
		NULL 			/* STANDARD_MODULE_PRPERTIES_EX global shutdown function */
 };


#ifdef ZTS
int frotz_globals_id;
#else
php_frotz_globals frotz_globals;
#endif


static void php_frotz_init_globals(FLS_D)
{
	FLS_FETCH();

	FG(input_buffer)  = NULL;
	FG(input_length)  = 0;
	FG(input_index)   = 0;

	
	FG(frotz_buffer)  = NULL;
	FG(frotz_scroll)  = NULL;
	FG(scroll_size)   = 0;

	FG(frotz_alloc)   = 0;
	FG(frotz_index)   = 0;
	FG(frotz_more)    = 0;
	FG(frotz_state)   = FROTZ_STATE_NULL;

	FG(screen_rows)   = 25;
	FG(screen_cols)   = 80;
	FG(screen_fcolor) = 0; /* FIXME: depricate? */
	FG(screen_bcolor) = 0;
#ifdef FROTZ_MY_INTERP
	FG(frotz_interp)  = INTERP_MSDOS; 
#endif


	FG(color)         = (GREEN_COLOUR <<4) | (BLACK_COLOUR);
	FG(style)         = 0;
	FG(font)          = 0;

	FG(read_state)   = 0;
	FG(beeps)        = 0;


	FG(game_str)     = NULL;
	FG(game_len)     = 0;
}

/*
#if defined(COMPILE_DL) || defined(COMPILE_DL_FROTZ)
	ZEND_GET_MODULE(frotz)
#endif
*/

static frotz_char *php_frotz_buf_copy(frotz_char *dst, frotz_char *src, int size) {
	int i;

	for (i=0; i < size; i++) {
		(dst+i)->c = (src+i)->c;
		(dst+i)->color = (src+i)->color;
		(dst+i)->style = (src+i)->style;
		(dst+i)->font = (src+i)->font;
	}
	return dst;

}

static void php_frotz_buf_clear(frotz_char *src, int size) 
{
	int i;

	for(i=0; i< size; i++ ) {
		(src+i)->c = 0;

		(src+i)->color = FG(color);
		/*
		 (src+i)->color  = (FG(screen_fcolor) << 4) & 0xf0;
		(src+i)->color |= FG(screen_bcolor) & 0xf;
		*/
		(src+i)->style = 0;
		(src+i)->font = FG(font);
	}
}

frotz_char *php_frotz_buf_new() 
{
	int size;
	frotz_char **scr, *nc;

	FLS_FETCH();

	size = FG(scroll_size);;
	scr = FG(frotz_scroll);

	scr  = (frotz_char **)erealloc(scr, sizeof(*scr)* (size+1));
	if (scr == NULL) 
		php_error(E_ERROR, "Couldn't reallocate screen while adding buffer");

	nc = (frotz_char *)emalloc(sizeof(*nc)*FG(frotz_alloc));
	if (nc == NULL)
		php_error(E_ERROR, "Couldn't allocate buffer  while adding buffer");

	if (size) {
		php_frotz_buf_copy(nc, scr[size-1], FG(frotz_alloc));
	} else {
		php_frotz_buf_clear(nc, FG(frotz_alloc));
	}
	scr[size] = nc;

	FG(frotz_scroll) = scr;
	FG(scroll_size) = size+1; 

	/* reset current buffer  ptr */
	FG(frotz_buffer) = nc;

	return nc;
}

int php_frotz_buf_del() 
{

	int size,i;
	frotz_char **scr, *pop;

	FLS_FETCH();

	size = FG(scroll_size);
	scr  = FG(frotz_scroll);

	if (size == 0) 
		return 0;

	pop = scr[0];

	/* shift the data */
	size--;
	for (i=1; i <= size; i++) 
		scr[i-1] = scr[i];
	
	if (size == 0) {
		efree(scr);
		scr = NULL;
	}

	FG(frotz_scroll) = scr;
	FG(scroll_size) = size;

	efree(pop);
	return 1;

}

frotz_char *php_frotz_buf_get()
{

	FLS_FETCH();

	if (FG(scroll_size) <= 0)  {
		return NULL;
	}

	return *FG(frotz_scroll);
}


PHP_MINIT_FUNCTION(frotz)
{
	REGISTER_LONG_CONSTANT("FROTZ_READLN", FROTZ_READLN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FROTZ_READKEY", FROTZ_READKEY, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("BLACK_COLOUR", BLACK_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RED_COLOUR", RED_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GREEN_COLOUR", GREEN_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("YELLOW_COLOUR", YELLOW_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("BLUE_COLOUR", BLUE_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MAGENTA_COLOUR", MAGENTA_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CYAN_COLOUR", CYAN_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("WHITE_COLOUR", WHITE_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GREY_COLOUR", GREY_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LIGHTGREY_COLOUR", LIGHTGREY_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MEDIUMGREY_COLOUR", MEDIUMGREY_COLOUR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("DARKGREY_COLOUR", DARKGREY_COLOUR, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FROTZ_PRINT_PRE", FROTZ_PRINT_PRE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FROTZ_PRINT_DUMP", FROTZ_PRINT_DUMP, CONST_CS | CONST_PERSISTENT);


#ifdef FROTZ_MY_INTERP
	REGISTER_LONG_CONSTANT("INTERP_DEC_20", INTERP_DEC_20, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_APPLE_IIE", INTERP_APPLE_IIE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_MACINTOSH", INTERP_MACINTOSH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_AMIGA", INTERP_AMIGA, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_ATARI_ST", INTERP_ATARI_ST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_MSDOS", INTERP_MSDOS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_CBM_64", INTERP_CBM_64, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_CBM_128", INTERP_CBM_128, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_APPLE_IIC", INTERP_APPLE_IIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_APPLE_IIGS", INTERP_APPLE_IIGS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INTERP_TANDY", INTERP_TANDY, CONST_CS | CONST_PERSISTENT);
#else

#if FROTZ_INTERP_NUM > 11
#error "Interp number, too high"
#elif FROTZ_INTERP_NUM < 1
#error "Interp number, too low"
#endif

#endif /* FROTZ_MY_INTERP */


	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(frotz)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(frotz)
{
	return SUCCESS;
}


PHP_MINFO_FUNCTION(frotz)
{
	char buf[255];

	php_info_print_table_start();
	php_info_print_table_header(2, "Frotz Support ", "enabled");
	sprintf(buf, "v%s", FROTZ_VERSION);
	php_info_print_table_row(2, "php Frotz Version", buf);

#if HAVE_ZLIB
	php_info_print_table_row(2, "Using zlib compression", "yes");
#else
	php_info_print_table_row(2, "Using zlib compression", "no");
#endif

	php_info_print_table_end();
}

PHP_GINIT_FUNCTION(frotz)
{
#ifdef ZTS
	frotz_globals_id = ts_allocate_id(sizeof(php_frotz_globals),  (ts_allocate_ctor) php_frotz_init_globals, NULL);
#else
	php_frotz_init_globals(FLS_C);
#endif
	return SUCCESS;
}


char *frotz_hash(char *str, unsigned long len) 
{
    char md5str[33];
    PHP_MD5_CTX context;
    unsigned char digest[16];
    int i;
    char *r;


    md5str[0] = '\0';
    PHP_MD5Init(&context);
    PHP_MD5Update(&context, str, len);
    PHP_MD5Final(digest, &context);
    for (i = 0, r = md5str; i < 16; i++, r += 2) {
        sprintf(r, "%02x", digest[i]);
    }
    *r = '\0';

	return estrdup(md5str);

}


int php_frotz_restore(zval **str)
{
	unsigned char *restore, *ptr;
	unsigned char *s1=NULL, *s2=NULL;
	int ret_length, status, factor=1, maxfactor=8;
	unsigned long length, new_len;
	unsigned char hash_cmp[33], *hash;


	convert_to_string_ex(str);

	restore = (unsigned char *)php_base64_decode((*str)->value.str.val, (*str)->value.str.len, &ret_length);

#if HAVE_ZLIB

	/*
	zlib::uncompress() wants to know the output data length
	if none was given as a parameter
	we try from input length * 2 up to input length * 2^8
	doubling it whenever it wasn't big enough
	that should be eneugh for all real life cases
	*/
	factor = 2;  // start out at 2^2; 2^1 isn't enough
	maxfactor = 8;
	do {
		length = ret_length * (1<<factor++);
		s2 = (unsigned char *) erealloc(s1,length);
		if(! s2) { if(s1) efree(s1); return 0; }
		status = uncompress(s2, &length ,restore, ret_length);
		s1=s2;
	} while((status==Z_BUF_ERROR)&&(factor<maxfactor));

	if(status==Z_OK) {
		s2 = erealloc(s2, length);
	} else {
		efree(s2);
		php_error(E_WARNING,"uncompress: %s",zError(status));
		return 0;
	}

#else 
	s2 = restore;
	length = ret_length;

#endif

#if PHP_DEBUG_NOT
	fprintf(stderr, "\nb64: %d, compressed: %d  raw: %d\n", (*str)->value.str.len, ret_length, length);
#endif

	FLS_FETCH();

	/* save the string information, this could be more efficiant */
	FG(game_str) = s2;
	FG(game_len) = length - 32; /* negate 32 for hash */


	/* grab the hash for comparing later */
	memcpy(hash_cmp, FG(game_str)+FG(game_len), 32);
	hash_cmp[32] = 0;

	new_len = FG(game_len) + key_len;
	ptr = erealloc(FG(game_str), new_len);
	if (ptr == NULL) {
		php_error(E_ERROR, "Key no buffer in save");
	}

	/* append key */
	memcpy(ptr+(FG(game_len)), key, key_len);

	/* generate hash */
	hash = frotz_hash(ptr, new_len);

#if PHP_DEBUG
	fprintf(stderr, "hash     is [%s]\nhash_cmp is [%s]\n", hash, hash_cmp);
#endif
	if (strncmp(hash, hash_cmp, 32)) {
		efree(hash);
		php_error(E_ERROR, "Invalid save game from player");
	}

	/* now remove hash */
	ptr = erealloc(ptr, FG(game_len));
	if (ptr == NULL) {
		php_error(E_ERROR, "Couldn't allocate memory for game");
	}

	/* FG(game_len) =  the right size and increase hash length */
	FG(game_str) = ptr; /* erealloc can change pointer address */


	return 1;
}

/* {{{ proto type name(args)
      description */
PHP_FUNCTION(frotz_play)
{
	zval **yyplay_cmds;
	int finished;

	FLS_FETCH();

	switch(ARG_COUNT(ht)) {
		case 1: 
			if (zend_get_parameters_ex(1, &yyplay_cmds)==FAILURE) {
				RETURN_LONG(-1);
			}
			convert_to_string_ex(yyplay_cmds);
			break;
		default:
			RETVAL_LONG(-2);
			WRONG_PARAM_COUNT;
			break; 
	}


	FG(input_buffer) = (*yyplay_cmds)->value.str.val;
	FG(input_length) = (*yyplay_cmds)->value.str.len;
	FG(input_index) = 0;
	FG(frotz_state)  = FROTZ_STATE_INIT;


	/*
	 * restore Settings if have them 
	 * and screen if we has settings
	 */
	if (FG(game_str) != NULL) {
		if (! frotz_restore(FROTZ_SAVE_SETTINGS) ) {
			php_error(E_WARNING, "failed doing settings");
			RETVAL_LONG(-99);
		}

		if (! frotz_restore(FROTZ_SAVE_SCROLL) ) {
			php_error(E_WARNING, "failed doing scroll");
			RETVAL_LONG(-98);
		}
	} else {
		FG(frotz_alloc) = FG(screen_rows)*FG(screen_cols);
		php_frotz_buf_new();

	    /* Position cursor to bottom of screen */
		FROTZ_INDEX_SET(FG(screen_rows), 1);

	}	

	 
	switch (FG(frotz_more) ) {
		case 0:
			/*
			 * init stuff here
			 * we need to restore game here
			 */
			init_memory();
			os_init_screen();

			if (graph_file)
				frotz_init_pictures(graph_file);

			init_undo ();
			z_restart ();

	 		if (FG(game_str) != NULL) {
				if (! frotz_restore(FROTZ_SAVE_GAME) ) {
					php_error(E_WARNING, "failed doing save");
					RETVAL_LONG(-97);
					break;
				}
			}			

			FG(frotz_state) = FROTZ_STATE_PLAY;

			interpret();
			frotz_close_pictures();

			break;

		default:
			FG(frotz_state) = FROTZ_STATE_MORE;
			FG(frotz_more) = FG(scroll_size)-1;
			break;
	}
	

	finished = get_finished();
	switch (finished) {
		case 9998:
		default:
			/*php_error(E_NOTICE, "finished is %d", finished); */
			break;
	}
	RETVAL_LONG(finished);
	set_finished(0); /* if they quit reset finished */

} /* }}} */



char *php_frotz_save_game(int *ret_length)
{

	unsigned char *result;
	unsigned char *s2=NULL;
	unsigned long l2;
	unsigned long new_len;
	int limit = 9, status;
	unsigned char *hash, *ptr;

	FLS_FETCH();

	*ret_length = 0;

	/* What state are we in */
	switch (FG(frotz_state) ) {
		case FROTZ_STATE_PLAY: case FROTZ_STATE_MORE:
			if (! frotz_save(FROTZ_SAVE_SETTINGS) ) {
				php_error(E_WARNING, "settings not saved");
				return NULL;
			}
			if (! frotz_save(FROTZ_SAVE_SCROLL) ) {
				php_error(E_WARNING, "scroll buffer not saved");
				return NULL;
			}
			break;
		default:
			php_error(E_WARNING, "Invalid frotz_state while saving game");
			return NULL;
			break;

	}


	new_len = FG(game_len) + key_len;
	ptr = erealloc(FG(game_str), new_len);
	if (ptr == NULL) {
		php_error(E_ERROR, "Key no buffer");
	}

	/* append key */
	memcpy(ptr+(FG(game_len)), key, key_len);

	/* generate hash */
	hash = frotz_hash(ptr, new_len);

#if PHP_DEBUG
	fprintf(stderr, "hash is [%s]\n", hash);
#endif

	/* now tak on the hash */
	ptr = erealloc(ptr, FG(game_len)+32);
	if (ptr == NULL) {
		php_error(E_ERROR, "Hash no buffer");
	}

	memcpy(ptr+FG(game_len), hash, 32);

	FG(game_len) += 32; /* and increase hash length */
	FG(game_str) = ptr; /* erealloc can change pointer address */


#if HAVE_ZLIB


	l2 = FG(game_len) + (FG(game_len)/1000) + 15;
	s2 = (unsigned char *) emalloc(l2);

	if(! s2) return NULL;

	if(limit>=0) {
		status = compress2(s2,&l2,FG(game_str), FG(game_len),limit);
	} else {
		status = compress(s2,&l2,FG(game_str), FG(game_len));
	}

	if(status!=Z_OK) {
		efree(s2);
		php_error(E_WARNING,"compress: %s",zError(status));
		return NULL;
	}


#else
	l2 = FG(game_len);
	s2 = FG(game_str);
#endif

	result = (unsigned char*)php_base64_encode(s2, l2, ret_length);

#if HAVE_ZLIB
	efree(s2);
#endif
	efree(hash);

#if PHP_DEBUG
	fprintf(stderr, "\nraw: %ld, compressed: %ld  b64: %d\n", FG(game_len), l2, *ret_length);
#endif
	return result;

}

static char *frotz_chunk_split(char *src, int srclen, char *end, int endlen,
                             int chunklen, int *destlen)
{
    char *dest;
	char *p, *q;
	int chunks; /* complete chunks! */
	int restlen;

	chunks = srclen / chunklen;
	restlen = srclen - chunks * chunklen; /* srclen % chunklen */

	dest = emalloc((srclen + (chunks + 1) * endlen + 1) * sizeof(char));

	for(p = src, q = dest; p < (src + srclen - chunklen + 1); ) {
		memcpy(q, p, chunklen);
		q += chunklen;
		memcpy(q, end, endlen);
		q += endlen;
		p += chunklen;
	}

	if(restlen) {
		memcpy(q, p, restlen);
		q += restlen;
		memcpy(q, end, endlen);
		q += endlen;
	}

	*q = '\0';
	if (destlen) {
		*destlen = q - dest;
	}

	return(dest);
}


/* {{{ proto type name(args)
      description */
PHP_FUNCTION(frotz_get_var)
{
	zval **zstr;


	switch(ARG_COUNT(ht)) {
		case 1: 
			if (zend_get_parameters_ex(1, &zstr)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(zstr);
			break;
		default:
			WRONG_PARAM_COUNT;
			break; 
	}

	if (! strncmp((*zstr)->value.str.val, "read_state", (*zstr)->value.str.len) ) {

		RETVAL_LONG(FG(read_state))

	} else if (! strncmp((*zstr)->value.str.val, "save_game", (*zstr)->value.str.len) ) {
		char *str;
		int len;

		str = php_frotz_save_game(&len);

		return_value->value.str.val = 
			frotz_chunk_split(str, len, "\n", 1 , 70, &(return_value->value.str.len));

		return_value->type = IS_STRING;


	} else if (! strncmp((*zstr)->value.str.val, "beeps", (*zstr)->value.str.len) ) {

		RETVAL_LONG(FG(beeps));

	} else if (! strncmp((*zstr)->value.str.val, "bcolor", (*zstr)->value.str.len) ) {

		RETVAL_STRING(html_colors[FG(color)&0xf],1);

	} else if (! strncmp((*zstr)->value.str.val, "fcolor", (*zstr)->value.str.len) ) {

		RETVAL_STRING(html_colors[(FG(color)>>4)&0xf],1);

	} else {

		php_error(E_WARNING, "frotz_get_var: Unkown variable name, '%s'", (*zstr)->value.str.val);
		RETVAL_FALSE;

	}

} /* }}} */

/* {{{ proto type name(args)
      description */
PHP_FUNCTION(frotz_set_var) 
{
	zval **var, **val;

	switch(ARG_COUNT(ht)) {
		case 2: 
			if (zend_get_parameters_ex(2, &var, &val)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break; 
	}


	convert_to_string_ex(var);

	if (! strncmp( (*var)->value.str.val, "story_file", (*var)->value.str.len) ) {
	    char *slash;

		graph_file = NULL;
		key = NULL;
		key_len = 0;
		php_frotz_init_globals();

		/*
		 * FIXME:
		 *   check story_name validity */
		convert_to_string_ex(val);

		story_name = estrdup((*val)->value.str.val);
		if (access(story_name, R_OK) == -1) {
			php_error(E_ERROR, "Can't read story file '%s'", story_name);
		}

		if (NULL == (slash = strrchr((*val)->value.str.val, '/')) ) {
		    slash = (*val)->value.str.val;
		}
		key = estrdup(slash);
		key_len = strlen(slash);
		key[key_len] = 0;

	} else if (! strncmp( (*var)->value.str.val, "save_game", (*var)->value.str.len) ) {

		if (! php_frotz_restore(val) ) {
			RETURN_FALSE;
		}

	} else if (! strncmp( (*var)->value.str.val, "graphics", (*var)->value.str.len) ) {

		convert_to_string_ex(val);
		graph_file = estrdup((*val)->value.str.val);

	} else if (! strncmp( (*var)->value.str.val, "secret", (*var)->value.str.len) ) {

		convert_to_string_ex(val);
		
		key_len += (*val)->value.str.len;
		key = erealloc(key, key_len);
		if (key == NULL)
			php_error(E_ERROR, "no key in set_var");
		/*
		strncat(key,  (*val)->value.str.val, (*val)->value.str.len); */
		memcpy(key+strlen(key), (*val)->value.str.val, (*val)->value.str.len);

#ifdef FROTZ_MY_INTERP
	} else if (! strncmp((*var)->value.str.val, "interp_num", (*var)->value.str.len) ) {
		FLS_FETCH();

		convert_to_long_ex(val);
		FG(frotz_interp) = (*val)->value.lval;
		if ( FG(frotz_interp) < 1 ) {
			php_error(E_ERROR, "interp_num is too low");
		} else if ( FG(frotz_interp) > 11) {
			php_error(E_ERROR, "interp_num is too high");
		}

#endif
	} else if (! strncmp( (*var)->value.str.val, "rows", (*var)->value.str.len) ) {
		convert_to_long_ex(val);
		FLS_FETCH();
		FG(screen_rows) = (*val)->value.lval;

	} else if (! strncmp( (*var)->value.str.val, "cols", (*var)->value.str.len) ) {
		convert_to_long_ex(val);
		FLS_FETCH();
		FG(screen_cols) = (*val)->value.lval;

	} else if (! strncmp( (*var)->value.str.val, "fcolor", (*var)->value.str.len) ) {
		convert_to_long_ex(val);
		FLS_FETCH();
		FG(screen_fcolor) = (*val)->value.lval & 0xff;
		FG(color) |= ((*val)->value.lval << 4) & 0xf0;

	} else if (! strncmp( (*var)->value.str.val, "bcolor", (*var)->value.str.len) ) {
		convert_to_long_ex(val);
		FLS_FETCH();
		FG(screen_bcolor) = (*val)->value.lval & 0xff;
		FG(color) |= (*val)->value.lval & 0xf;

	} else {

		php_error(E_WARNING, "frotz_set_var: Unkown variable name, '%s'", (*var)->value.str.val);
		RETVAL_FALSE;
	}

	RETURN_TRUE;

} /* }}} */


/* {{{ proto type name(args)
      description */
PHP_FUNCTION(frotz_output) 
{
	zval **ztype;
	int type;

	switch(ARG_COUNT(ht)) {
		case 0:
			type = FROTZ_PRINT_PRE;
			break;
		case 1: 
			if (zend_get_parameters_ex(1, &ztype)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(ztype);
			type = (*ztype)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break; 
	}

	switch (type) 
	{
		case FROTZ_PRINT_PRE:
			frotz_print_html_pre();
			break;
		case FROTZ_PRINT_DUMP:
			frotz_print_html_raw();
			break;
		default:
			php_error(E_WARNING, "Invalid type [%d] for printing", type);
			RETVAL_LONG(0);
	}

} /* }}} */

static void _frotz_print_color(zchar color, int *cnt, int reverse)
{
	char buf[255];
	int i, thecolor;

	if (*cnt) {
		php_write("</SPAN>", 7);
		(*cnt)--;
	}
	thecolor = (!reverse? color: ((color>>4) | (color<<4)) );

	if (color) {
		i = sprintf(buf, "<SPAN style=\"color: %s; background-color: %s\">", 
				 html_colors[((thecolor>>4)&0xf)], html_colors[(thecolor&0xf)]);
		php_write(buf, i);
		(*cnt)++;
	}

}

static int _frotz_flush_buf(char *buf, int *buf_len)
{

	char *html_buf;
	int len;


	if ( 0 >= *buf_len ) 
		return 1;

	html_buf = php_escape_html_entities(buf, *buf_len, &len, 1);
	php_write(html_buf, len);
	*buf_len = 0; /* rest length */

	if (html_buf) 
		efree(html_buf);
	else 
		return 0;

	return 1;
}

int frotz_print_html_pre() 
{

	frotz_char *ptr;
	char *buf, c;
	int x,i,y,alloc;
	zchar cur_color;
	int cnt_color, buf_len;
	int cur_bold, bold;
	int cur_emph, emph;
	int cur_rvrs, rvrs;
	int cursor, cursor_line=0;
	

	FLS_FETCH();

	alloc = FG(screen_rows)*(FG(screen_cols)+1);
	if ( (buf = (char *) emalloc(sizeof(zchar) * (alloc)) ) == NULL) {
		php_error(E_WARNING, "Couldn't allocate memory for output");
		return 0;
	}	

	ptr = php_frotz_buf_get();

	/* Only if we are on the last screen
	 * will we print the cursor onto the screen */
	if (FG(scroll_size) == 1) 
	   cursor = FG(frotz_index);
	else 
	  cursor = -1;

	cnt_color = 0;
	buf_len = 0;

	cur_color = -1;
	cur_bold  = 0;
	cur_emph  = 0;
	cur_rvrs  = 0;

	php_write("<pre>", 5);
	for (y=0; y < FG(screen_rows); y++) {

		i = (y * FG(screen_cols))+y;

		/* just in case */
		if (buf_len+FG(screen_cols)+1 > alloc) 
			_frotz_flush_buf(buf, &buf_len);

		for (x=0; x < FG(screen_cols); x++) {

			c = ptr->c;
			bold = (ptr->style & BOLDFACE_STYLE);
			emph = (ptr->style & EMPHASIS_STYLE);
			rvrs = (ptr->style & REVERSE_STYLE);

			if (!emph && cur_emph) {
				_frotz_flush_buf(buf, &buf_len);
				php_write("</em>", 5);
				cur_emph--;
			}

			if (!bold && cur_bold) {
				_frotz_flush_buf(buf, &buf_len);
				php_write("</strong>", 9);
				cur_bold--;
			}

			/* Color change? */
			if (ptr->color != cur_color) {
				_frotz_flush_buf(buf, &buf_len);
				_frotz_print_color(ptr->color, &cnt_color, 0); 
				cur_color = ptr->color;
			}

			/* reverse */
			if (!rvrs && cur_rvrs) {
				_frotz_flush_buf(buf, &buf_len);
				_frotz_print_color(ptr->color, &cnt_color, 0); 
				cur_rvrs--;
			} 
			if (rvrs && !cur_rvrs) {
				_frotz_flush_buf(buf, &buf_len);
				_frotz_print_color(ptr->color, &cnt_color, 1); 
				cur_rvrs++;
			}

			/* bold */
			if (bold && !cur_bold) {
				_frotz_flush_buf(buf, &buf_len);
				php_write("<strong>", 8);
				cur_bold++;
			}

			/* emphases */
			if (emph && !cur_emph) {
				_frotz_flush_buf(buf, &buf_len);
				php_write("<em>", 4);
				cur_emph++;
			}

			if (cursor > -1 && cursor == FROTZ_INDEX_OF(y+1, x+1) ) {
				_frotz_flush_buf(buf, &buf_len);
				php_write("<blink><u>", 10);
			}
			switch (c) {
			  /*
				case FROTZ_CURSOR_CHAR:
					_frotz_flush_buf(buf, &buf_len);
					php_write("<blink>_</blink>", 16);
					ptr->c = 0; 
					break;
			  */
				case 0: /* set char to space */
					c = ' ';
				default:
					buf[buf_len++] = c;
			}
			if (cursor > -1 && cursor == FROTZ_INDEX_OF(y+1, x+1) ) {
				_frotz_flush_buf(buf, &buf_len);
				php_write("</u></blink>", 12);
			   cursor = -1; //
			}
			ptr++; /* increment this now */

		}
		buf[buf_len++] = '\n';

	}

	_frotz_flush_buf(buf, &buf_len);
	_frotz_print_color(0, &cnt_color,0);  /* this closes any items */
	php_write("</pre>", 6);
		
	efree(buf);

	if (FG(scroll_size) > 1) 
		php_frotz_buf_del();

	return FG(scroll_size)-1;
}


int frotz_print_html_raw() 
{
	int i;

	for(i=FG(scroll_size); i>0; i--) {
		frotz_print_html_pre();
	}
	return (FG(scroll_size)-1);
}


/* {{{ proto type name(args)
      description */
PHP_FUNCTION(frotz_close)
{
	/*
	 * FIXME: make a destructor function
	 */

	FLS_FETCH();	


	/*
	if (FG(game_str) != NULL ) {
		efree(FG(game_str));
	}
	*/
	if (FG(frotz_scroll) != NULL) {
		int i;
		frotz_char **scr = FG(frotz_scroll);

		for(i=FG(scroll_size)-1; i >= 0; i--)
			if (scr[i] != NULL) 
				efree(scr[i]);

		efree(scr);
	}

	if (FG(frotz_state) == FROTZ_STATE_PLAY) {
		reset_memory ();
	}
	php_frotz_init_globals();



	RETURN_TRUE;
}  /* }}} */ /* frotz_close */


/* bufferch
 *   wrapper for getting the input string
 */
int bufferch() 
{ 
	char *ptr;
	
	FLS_FETCH();


	if (FG(input_index) >= FG(input_length) ) {
		return 0;
	}

	ptr = FG(input_buffer) + FG(input_index);
	FG(input_index)++;
	
	if (*ptr == '\\') {
		if (FG(input_index) >= FG(input_length) ) {
			return 0;
		}
		ptr = FG(input_buffer) + FG(input_index);
		FG(input_index)++;

		switch (tolower(*ptr)) {
			case 'u': return ZC_ARROW_UP;
			case 'd': return ZC_ARROW_DOWN;
			case 'l': return ZC_ARROW_LEFT;
			case 'r': return ZC_ARROW_RIGHT;
			default:
				return *ptr;
		}
	}

	return *ptr;

} /* bufferch */


/*
 * php_z_save
 */
int php_z_save (void) 
{
	int status;
	
 	flush_buffer(); /* z engine flush to ensure string is outputed */
	status = frotz_save(FROTZ_SAVE_GAME);
	/* frotz_print_cursor(); */

	return status;

} /* php_z_save */

