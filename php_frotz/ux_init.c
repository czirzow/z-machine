/*
 * ux_init.c
 *
 * Unix interface, initialisation
 *
 */

#define __UNIX_PORT_FILE
#include "php.h"

#include "php_frotz.h"


static int user_random_seed = -1;
static int user_tandy_bit = 0;


char unix_plain_ascii = 0;    /* true if user wants to disable latin-1 */

/*
 * os_fatal
 *
 * Display error message and exit program.
 *
 */

void os_fatal (const char *s)
{

	php_error(E_CORE_ERROR, "fatal error from %s: %s", story_name, s);

}/* os_fatal */


/*
 * os_init_screen
 *
 * Initialise the IO interface. Prepare the screen and other devices
 * (mouse, sound board). Set various OS depending story file header
 * entries:
 *
 *     h_config (aka flags 1)
 *     h_flags (aka flags 2)
 *     h_screen_cols (aka screen width in characters)
 *     h_screen_rows (aka screen height in lines)
 *     h_screen_width
 *     h_screen_height
 *     h_font_height (defaults to 1)
 *     h_font_width (defaults to 1)
 *     h_default_foreground
 *     h_default_background
 *     h_interpreter_number
 *     h_interpreter_version
 *     h_user_name (optional; not used by any game)
 *
 * Finally, set reserve_mem to the amount of memory (in bytes) that
 * should not be used for multiple undo and reserved for later use.
 *
 * (Unix has a non brain-damaged memory model which dosen't require such
 *  ugly hacks, neener neener neener. --GH :)
 *
 */

void os_init_screen (void)
{
	/* disable undos */
	option_undo_slots = 0;

	if (h_version == V3 && user_tandy_bit != 0)
		h_config |= CONFIG_TANDY;

	if (h_version == V3)
		h_config |= CONFIG_SPLITSCREEN;

	if (h_version >= V4)
		h_config |= CONFIG_BOLDFACE | CONFIG_EMPHASIS | CONFIG_FIXED;
	//| CONFIG_TIMEDINPUT;

#ifndef SOUND_SUPPORT_NOT
	if (h_version >= V5)
		h_flags &= ~(GRAPHICS_FLAG | SOUND_FLAG | MOUSE_FLAG | MENU_FLAG);

	if (h_version == V3)
		h_flags &= ~OLD_SOUND_FLAG;
#else
	if (h_version >= V5)
		h_flags &= ~(GRAPHICS_FLAG | MOUSE_FLAG);


	if ((h_version >= 5) && (h_flags & SOUND_FLAG)) 
			h_flags &= ~SOUND_FLAG; /* NOT */

	if ((h_version == 3) && (h_flags & OLD_SOUND_FLAG)) 
			h_flags &= ~OLD_SOUND_FLAG; /* NOT */
#endif

	if (h_version >= V5 && (h_flags & UNDO_FLAG))
		if (option_undo_slots == 0)
			h_flags &= ~UNDO_FLAG;

	h_screen_rows = FG(screen_rows);
	h_screen_cols = FG(screen_cols);

	h_screen_width = h_screen_cols;
	h_screen_height = h_screen_rows;

	h_font_width = 1; h_font_height = 1;

	h_interpreter_number = FROTZ_INTERP;
	h_interpreter_version = 'F';

	if ( h_flags & COLOUR_FLAG) {
		h_config |= CONFIG_COLOUR;
		h_flags |= COLOUR_FLAG; /* FIXME: beyond zork handling? */
		h_default_foreground = (FG(color)>>4) & 0xf;
		h_default_background = FG(color) & 0xf;
		os_set_colour(h_default_foreground, h_default_background);
	} else
		if (h_flags & COLOUR_FLAG) 
			h_flags &= ~COLOUR_FLAG;

}/* os_init_screen */

/*
 * os_reset_screen
 *
 * Reset the screen before the program stops.
 *
 */

void os_reset_screen (void)
{
	os_set_text_style(0);

}/* os_reset_screen */

/*
 * os_restart_game
 *
 * This routine allows the interface to interfere with the process of
 * restarting a game at various stages:
 *
 *     RESTART_BEGIN - restart has just begun
 *     RESTART_WPROP_SET - window properties have been initialised
 *     RESTART_END - restart is complete
 *
 */

void os_restart_game (int stage)
{

}

/*
 * os_random_seed
 *
 * Return an appropriate random seed value in the range from 0 to
 * 32767, possibly by using the current system time.
 *
 */

int os_random_seed (void)
{

    if (user_random_seed == -1)
      /* Use the epoch as seed value */
      return (time(0) & 0x7fff);
    else return user_random_seed;

}/* os_random_seed */
