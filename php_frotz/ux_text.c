/*
 * ux_text.c
 *
 * Unix interface, text functions
 *
 */

#define __UNIX_PORT_FILE

#include "php_frotz.h"

static char latin1_to_ascii[] =
	"   !  c  L  >o<Y  |  S  '' C  a  << not-  R  _  "
	"^0 +/-^2 ^3 '  my P  .  ,  ^1 o  >> 1/41/23/4?  "
	"A  A  A  A  Ae A  AE C  E  E  E  E  I  I  I  I  "
	"Th N  O  O  O  O  Oe *  O  U  U  U  Ue Y  Th ss "
	"a  a  a  a  ae a  ae c  e  e  e  e  i  i  i  i  "
	"th n  o  o  o  o  oe :  o  u  u  u  ue y  th y  ";



zchar frotz_print_char_at(int idx, zchar c)  
{

	FLS_FETCH();

	if (idx > FG(frotz_alloc))  /* catch it */ {
		char str[2];
		str[0] = c;
		str[1] = '\0';
		php_write(str, 1);
		return 0;
	}
	(FG(frotz_buffer)+idx)->c = c;
	/*(FG(frotz_buffer)+idx)->color = (FG(screen_fcolor)<<4) | FG(screen_bcolor);;*/
	(FG(frotz_buffer)+idx)->color =  FG(color);
	(FG(frotz_buffer)+idx)->style =  FG(style);
	(FG(frotz_buffer)+idx)->font =  FG(font);

	return c;
}



zchar frotz_print_char(zchar c) 
{

	if (frotz_print_char_at(FG(frotz_index)++, c) != c) {
		return 0;
	}
	return c;
}

void frotz_set_picture(int y, int x, zchar c) 
{

	int i;

	i = FROTZ_INDEX_OF(y, x);
	FG(frotz_index) = i;
	frotz_print_char(c);
//	frotz_print_char_at(i,c);

}


/*
int frotz_print_cursor() 
{

	return frotz_print_char(FROTZ_CURSOR_CHAR);
}
*/




void frotz_erase_area(int start, int end) 
{

	int i;

	FLS_FETCH();

	/* clear screen only if we are actually playing the
	 * game. other wise our saved screen will get erased. */
	if (FG(frotz_state) != FROTZ_STATE_PLAY)
		return;

	if (start < 0)
		php_error(E_ERROR, "frotz_erase_area: start is before zero");

	/* prevent overruns */
	if (end >= FG(frotz_alloc))  
		end = FG(frotz_alloc) - 1;

	for (i = start; i <= end; i++) {
		FROTZ_PRINTC_AT(i, '\0');
	}
}

#if 0
  switch(color) {
		case BLACK_COLOUR: return COLOR_BLACK;
		case RED_COLOUR: return COLOR_RED;
		case GREEN_COLOUR: return COLOR_GREEN;
		case YELLOW_COLOUR: return COLOR_YELLOW;
		case BLUE_COLOUR: return COLOR_BLUE;
		case MAGENTA_COLOUR: return COLOR_MAGENTA;
		case CYAN_COLOUR: return COLOR_CYAN;
		case WHITE_COLOUR: return COLOR_WHITE;
  }
  return 0;
#endif


/*
 * os_set_colour
 *
 * Set the foreground and background colours which can be:
 *
 *	 DEFAULT_COLOUR
 *	 BLACK_COLOUR
 *	 RED_COLOUR
 *	 GREEN_COLOUR
 *	 YELLOW_COLOUR
 *	 BLUE_COLOUR
 *	 MAGENTA_COLOUR
 *	 CYAN_COLOUR
 *	 WHITE_COLOUR
 *
 *	 MS-DOS 320 columns MCGA mode only:
 *
 *	 GREY_COLOUR
 *
 *	 Amiga only:
 *
 *	 LIGHTGREY_COLOUR
 *	 MEDIUMGREY_COLOUR
 *	 DARKGREY_COLOUR
 *
 * There may be more colours in the range from 16 to 255; see the
 * remarks on os_peek_colour.
 *
 */
void os_set_colour (int new_fore, int new_back)
{

	FLS_FETCH();

	switch (new_fore) 
	{
		
		case -1: case 13: /* cursor position */

			if (FG(frotz_index) >= 0 && FG(frotz_index) < FG(frotz_alloc) )
				new_fore = ((FG(frotz_buffer)+FG(frotz_index))->color >> 4) & 0x0f;
			else
				new_fore = (FG(color) >> 4) & 0x0f;
			break;

		case 0: /* current */

			new_fore = (FG(color) >> 4) & 0x0f;
			break;

		case 1:  /* default */

			new_fore = h_default_foreground;
			break;
	}

	switch (new_back)
	{

		case -1: case 13:
			if (FG(frotz_index) >= 0 && FG(frotz_index) < FG(frotz_alloc) )
				new_back = ((FG(frotz_buffer)+FG(frotz_index))->color) & 0x0f;
			else
				new_back = FG(color) & 0x0f;
			break;
		case 0:
			new_back = FG(color) & 0x0f;
			break;
		case 1: 
			new_back = h_default_background;
			break;
	}
	FG(color) = (new_fore<<4) | new_back;

}/* os_set_colour */


/*
 * os_font_data
 *
 * Return true if the given font is available. The font can be
 *
 *	TEXT_FONT
 *	PICTURE_FONT
 *	GRAPHICS_FONT
 *	FIXED_WIDTH_FONT
 *
 * The font size should be stored in "height" and "width". If
 * the given font is unavailable then these values must _not_
 * be changed.
 *
 */
int os_font_data (int font, int *height, int *width)
{

	if (font == TEXT_FONT) {
		*height = 1; *width = 1; return 1; /* Truth in advertising */
	}
	if (font = PICTURE_FONT) {
		*height = 1; *width = 1; return 1; /* Truth in advertising */
	}
	return 0;

}/* os_font_data */


/*
 * os_set_cursor
 *
 * Place the text cursor at the given coordinates. Top left is (1,1).
 *
 */
void os_set_cursor (int y, int x)
{

	FLS_FETCH();
	FROTZ_INDEX_SET(y,x);

}/* os_set_cursor */


/*
 * os_set_text_style
 *
 * Set the current text style. Following flags can be set:
 *
 *	 REVERSE_STYLE
 *	 BOLDFACE_STYLE
 *	 EMPHASIS_STYLE (aka underline aka italics)
 *	 FIXED_WIDTH_STYLE
 *
 */
void os_set_text_style (int new_style)
{ 

	FLS_FETCH();

	FG(style) = new_style;

}/* os_set_text_style */


/*
 * os_set_font
 *
 * Set the font for text output. The interpreter takes care not to
 * choose fonts which aren't supported by the interface.
 *
 */
void os_set_font (int new_font)
{

	FLS_FETCH();
	FG(font) = new_font;
	/* Not implemented */

}/* os_set_font */

/*
 * os_display_char
 *
 * Display a character of the current font using the current colours and
 * text style. The cursor moves to the next position. Printable codes are
 * all ASCII values from 32 to 126, ISO Latin-1 characters from 160 to
 * 255, ZC_GAP (gap between two sentences) and ZC_INDENT (paragraph
 * indentation). The screen should not be scrolled after printing to the
 * bottom right corner.
 *
 */
void os_display_char (zchar c)
{

	FLS_FETCH();
   
	if (c >= ZC_LATIN1_MIN && c <= ZC_LATIN1_MAX) {
		FROTZ_PRINTC(c);
#if 0
		if ( unix_plain_ascii) {

		  char *ptr = latin1_to_ascii + 3 * (c - ZC_LATIN1_MIN);
		  char c1 = *ptr++;
		  char c2 = *ptr++;
		  char c3 = *ptr++;

		  FROTZ_PRINTC(c1);

		  if (c2 != ' ')
			FROTZ_PRINTC(c2);
		  if (c3 != ' ')
			FROTZ_PRINTC(c3);

		} else
			FROTZ_PRINTC(c);
#endif
		return;
	}

	if (c >= 32 && c <= 126) {
		FROTZ_PRINTC(c);
		return;
	}

	if (c == ZC_INDENT) {
		FROTZ_PRINTC(' '); FROTZ_PRINTC(' '); FROTZ_PRINTC(' ');
		return;
	}

	if (c == ZC_GAP) {
		FROTZ_PRINTC(' '); FROTZ_PRINTC(' ');
		return;
	}

}/* os_display_char */

/*
 * os_display_string
 *
 * Pass a string of characters to os_display_char.
 *
 */
void os_display_string (const zchar *s)
{

	zchar c;

	while ((c = (unsigned char) *s++) != 0)
		if (c == ZC_NEW_FONT || c == ZC_NEW_STYLE) {
			int arg = (unsigned char) *s++;

			if (c == ZC_NEW_FONT)
				os_set_font (arg);
			if (c == ZC_NEW_STYLE)
				os_set_text_style (arg);

		} else os_display_char (c);

}/* os_display_string */

/*
 * os_char_width
 *
 * Return the width of the character in screen units.
 *
 */
int os_char_width (zchar c)
{

	if (c >= ZC_LATIN1_MIN && c <= ZC_LATIN1_MAX) {

		int width = 0;
		const char *ptr = latin1_to_ascii + 3 * (c - ZC_LATIN1_MIN);
		char c1 = *ptr++;
		char c2 = *ptr++;
		char c3 = *ptr++;

		width++;
		if (c2 != ' ')
		  width++;
		if (c3 != ' ')
		  width++;

		return width;
	}

	return 1;

}/* os_char_width*/


/*
 * os_string_width
 *
 * Calculate the length of a word in screen units. Apart from letters,
 * the word may contain special codes:
 *
 *	NEW_STYLE - next character is a new text style
 *	NEW_FONT  - next character is a new font
 *
 */
int os_string_width (const zchar *s)
{
	int width = 0;
	zchar c;

	while ((c = *s++) != 0)
	  if (c == ZC_NEW_STYLE || c == ZC_NEW_FONT) { 
		  s++; /* No effect */
	  } else width += os_char_width(c);

	return width;

}/* os_string_width */


/*
 * os_more_prompt
 *
 * Display a MORE prompt, wait for a keypress and remove the MORE
 * prompt from the screen.
 *
 */

void os_more_prompt (void)
{
	frotz_char *old, *new;
	int frotz_index;

	FLS_FETCH();
	FG(frotz_more)++;

	/* save this buffer for a second */
	old = FG(frotz_buffer);
	frotz_index = FG(frotz_index); /* save index */
	
	new = php_frotz_buf_new();

	FG(frotz_buffer) = old;
	os_display_string("*** MORE ***\r\n");
	FG(frotz_buffer) = new;
	FG(frotz_index) = frotz_index;
	

}/* os_more_prompt */
