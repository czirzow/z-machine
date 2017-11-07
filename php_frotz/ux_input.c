/*
 * * ux_input.c
 * *
 * * Unix interface, input functions
 * *
 * */

#define __UNIX_PORT_FILE

#include "php_frotz.h"


extern bool is_terminator (zchar); /* z-mach */


/*
 * * unix_read_char
 * *
 * * This uses the curses getch() routine to get the next character typed,
 * * and returns it unless the global timeout is reached.  It returns values
 * * which the standard considers to be legal input, and also returns editing
 * * and frotz hot keys.  If called with a non-zero flag, it will also return
 * * line-editing keys like INSERT, etc,
 * *
 * */

static int unix_read_char(int flag)
{
	int c;
	struct timeval ctime;

	while(1) {
		c = bufferch();


		/* Catch 98% of all input right here... */
		if ( ((c >= 32) && (c <= 126)) || (c == 13) || (c == 8)) 
			return c;

		/* ...and the other 2% makes up 98% of the code. :( */
		switch(c) {
		 /*  catch end of input treat is as a carriage return */
			case 0:  return 13;
			case 10: return 13;

			case ZC_ARROW_UP:
			case ZC_ARROW_DOWN:
			case ZC_ARROW_LEFT:
			case ZC_ARROW_RIGHT:
					 return c;


			default: break; /* Who knows? */
		}

		/* Finally, if we're in full line mode (os_read_line), we might return
		 * codes which aren't legal Z-machine keys but are used by the editor. */
		if (flag) return c;
	}
}



int os_buffer (zbyte code) {

	FLS_FETCH();

	FG(read_state) = code;

	if (FG(input_index) >= FG(input_length)) {
		php_z_save();
		return 0;
	}
	return 1;
}

/*
 * * os_read_line
 * *
 * * Read a line of input from the keyboard into a buffer. The buffer
 * * may already be primed with some text. In this case, the "initial"
 * * text is already displayed on the screen. After the input action
 * * is complete, the function returns with the terminating key value.
 * * The length of the input should not exceed "max" characters plus
 * * an extra 0 terminator.
 * *
 * * Terminating keys are the return key (13) and all function keys
 * * (see the Specification of the Z-machine) which are accepted by
 * * the is_terminator function. Mouse clicks behave like function
 * * keys except that the mouse position is stored in global variables
 * * "mouse_x" and "mouse_y" (top left coordinates are (1,1)).
 * *
 * * Furthermore, Frotz introduces some special terminating keys:
 * *
 * *     ZC_HKEY_KEY_PLAYBACK (Alt-P)
 * *     ZC_HKEY_RECORD (Alt-R)
 * *     ZC_HKEY_SEED (Alt-S)
 * *     ZC_HKEY_UNDO (Alt-U)
 * *     ZC_HKEY_RESTART (Alt-N, "new game")
 * *     ZC_HKEY_QUIT (Alt-X, "exit game")
 * *     ZC_HKEY_DEBUGGING (Alt-D)
 * *     ZC_HKEY_HELP (Alt-H)
 * *
 * * If the timeout argument is not zero, the input gets interrupted
 * * after timeout/10 seconds (and the return value is 0).
 * *
 * * The complete input line including the cursor must fit in "width"
 * * screen units.
 * *
 * * The function may be called once again to continue after timeouts,
 * * misplaced mouse clicks or hot keys. In this case the "continued"
 * * flag will be set. This information can be useful if the interface
 * * implements input line history. 
 * *
 * * The screen is not scrolled after the return key was pressed. The
 * * cursor is at the end of the input line when the function returns.
 * *
 * * Since Inform 2.2 the helper function "completion" can be called
 * * to implement word completion (similar to tcsh under Unix).
 * *
 * */


zchar os_read_line (int max, zchar *buf, int timeout, int width, int continued)
{
	int ch, scrpos, pos, y, x;

	scrpos = pos = strlen((char *) buf);

	do {

		ch = unix_read_char(1);
		if (is_terminator(ch)) /* incase of empty string */
			break;

		/* Backspace */
		if ((ch == 8) && (scrpos)) {
			pos--; scrpos--;
			if (scrpos != pos)
				memmove(&(buf[scrpos]), &(buf[scrpos+1]), pos-scrpos);
		}

		/* Delete */
		if (((ch == 127) && scrpos < pos)) {
			pos--;
			memmove(&(buf[scrpos]), &(buf[scrpos+1]), pos-scrpos);
		}

		/* Left key */
		if ((ch == 131) && (scrpos)) {
			scrpos--;
		}
		/* Right key */
		if ((ch == 132) && (scrpos < pos)) {
			scrpos++;
		}

		/* ASCII printable */
		if ((ch >= 32) && (ch <= 126)) {
			if (pos == scrpos) {
				/* Append to end of buffer */
				if ((pos < max) && (pos < width)) {
					buf[pos++] = (char) ch;
					scrpos++; x++;
				} else os_beep(0);
			}
		}
	} while (!is_terminator(ch));

	buf[pos] = '\0';

	os_display_string(buf);

	return ch;

}/* os_read_line */

/*
 * * os_read_key
 * *
 * * Read a single character from the keyboard (or a mouse click) and
 * * return it. Input aborts after timeout/10 seconds. (disabled)
 * *
 * */
zchar os_read_key (int timeout, int cursor)
{
	zchar c;

	c = (zchar) unix_read_char(0);

	return c;

}/* os_read_key */

/*
 * * os_read_file_name
 * *
 * * Return the name of a file. Flag can be one of:
 * *
 * *    FILE_SAVE     - Save game file
 * *    FILE_RESTORE  - Restore game file
 * *    FILE_SCRIPT   - Transscript file
 * *    FILE_RECORD   - Command file for recording
 * *    FILE_PLAYBACK - Command file for playback
 * *    FILE_SAVE_AUX - Save auxilary ("preferred settings") file
 * *    FILE_LOAD_AUX - Load auxilary ("preferred settings") file
 * *
 * */

int os_read_file_name (char *file_name, const char *default_name, int flag)
{

	return 0;

} /* os_read_file_name */
