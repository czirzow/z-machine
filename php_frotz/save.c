
#include "php.h"
#include "php_globals.h"
#include "php_frotz.h"


extern FILE *story_fp;

static unsigned char *frotz_get_type(int type);
static int frotz_save_game();
static int frotz_save_settings();
static int frotz_save_scroll();
static int frotz_restore_scroll(unsigned char *ptr);
static int frotz_restore_game(unsigned char *ptr);
static int frotz_restore_settings(unsigned char *ptr);

static unsigned char *frotz_get_type(int type)
{

	int i, count, size, found;
	unsigned char *ptr;

	FLS_FETCH();

	ptr = FG(game_str);
	if (ptr == NULL) 
		return ptr;

	count = *ptr++;
	found = 0;

	for (i=0; i < count; i++) {
		if (*ptr == type) {
			found = type;
			break;
		}
		ptr++;
		size = (long) *(ptr) << 16;
		size |= (unsigned) *(ptr+1) << 8;
		size |= (unsigned char)*(ptr+2);
		ptr += size;
	}
	return (found? ptr:NULL);
}


static int frotz_restore_game(unsigned char *ptr) 
{

	int data_len;
	zword success = 0;
	long pc;
	zword release;
	zword addr;
	int i,j, frotz_win;

	FLS_FETCH();

	if ((*ptr++) != FROTZ_SAVE_GAME) 
		return success;

	data_len = (long) (*ptr++) << 16;
	data_len |= (unsigned) (*ptr++) << 8;
	data_len |= (unsigned char) *ptr++;


	/* sets up z-machine internal screen information */
	ptr = screen_restore(ptr);


	release = (unsigned) (*ptr++) << 8;
	release |= *ptr++;

	(void) *ptr++;
	(void) *ptr++;

	/* Check the release number */

	if (release == h_release) {

		pc = (long) (*ptr++) << 16;
		pc |= (unsigned) (*ptr++) << 8;
		pc |= *ptr++;

		SET_PC (pc)

		sp = stack + ( (*ptr++) << 8);
		sp += *ptr++;
		fp = stack + ( (*ptr++) << 8);
		fp += *ptr++;

		for (i = (int) (sp - stack); i < STACK_SIZE; i++) {
			stack[i] = (unsigned) (*ptr++) << 8;
			stack[i] |= *ptr++;
		}

		fseek (story_fp, 0, SEEK_SET);

		for (addr = 0; addr < h_dynamic_size; addr++) {
			int skip = *ptr++;
			for (i = 0; i < skip; i++)
				zmp[addr++] = fgetc (story_fp);

			zmp[addr] = *ptr++;
			(void) fgetc (story_fp);
		}

		/* Check for errors */

		if (addr != h_dynamic_size)
		os_fatal ("Error reading save file");

		/* Reset upper window (V3 only) */
		if (h_version == V3)
			split_window (0);

		/* Initialise story header */
		/*restart_header (); */

		/* Success */
		success = 2;

	} else php_error (E_WARNING, "Invalid restore string: release/h_release %d/%d\n", release, h_release);

	return success;

}

static int frotz_restore_scroll(unsigned char *ptr) 
{ /* this ptr is assumed correctly placed */

	int i,j, scroll_size, data_len;
	unsigned char *ptr_orig;
	frotz_char *buf;


	ptr_orig = ptr; /* save this for some checking at the end */

	/* Sane ptr? */
	if (*ptr != FROTZ_SAVE_SCROLL) {
		php_error(E_WARNING, "Invalid buffer in restore string");
		return 0;
	}
	ptr++;                 /* ok this looks like a scroll buffer */

	/* size of the whole buffer */
	data_len = (long) (*ptr++) << 16;
	data_len |= (unsigned) (*ptr++) << 8;
	data_len |= (unsigned char) *ptr++;

	scroll_size = *ptr++;  /* how many scroll buffers we have */
	

	/* FIXME: make this a bit cleaner
	 *   right now there is only one
	 */
	if (FG(frotz_scroll) == NULL) {
		buf = php_frotz_buf_new();
	} else {
		buf = *FG(frotz_scroll);
	}

	/*
	 * Now fill that one buffer up
	 */
	for (j=0; j < FG(frotz_alloc); j++) {
		(buf+j)->c = *ptr++;
		(buf+j)->color  = *ptr++;
		(buf+j)->style  = *ptr++;
		(buf+j)->font  = *ptr++;
	}

	/*
	 * If we have more to do lets do them
	 * from the save game
	 *
	 * But we shouldn't expect anything in the scroll buffer
	 */
	for (i=1; i < scroll_size; i++) {
		buf = php_frotz_buf_new();
		if (buf == NULL) 
			return 0;
		
		for (j=0; j < FG(frotz_alloc); j++) {
			(buf+j)->c = *ptr++;
			(buf+j)->color = *ptr++;
			(buf+j)->style  = *ptr++;
			(buf+j)->font  = *ptr++;
		}
	}

	/* 
	 * Some lame sanity checking
	 *   Possibly remove this
	 */
	if (FG(scroll_size) != scroll_size) {
		php_error(E_WARNING, "scroll over/under run: global: %d local: %d", FG(scroll_size), scroll_size);
		return 0;
	}
	/*
	 * cross check
	 */
	if (data_len != ((ptr-ptr_orig)/sizeof(*ptr)) ) {
		php_error(E_WARNING, "scroll over/under run: should be %d but is %d", data_len, ((ptr-ptr_orig)/sizeof(*ptr)));
		return 0;
	}

	return 1; /* return the new position */

}


static int frotz_restore_settings(unsigned char *ptr)
{
	int data_len;

	FLS_FETCH();

	if (*ptr++ != FROTZ_SAVE_SETTINGS) 
		return 0;

	data_len = (long) (*ptr++) << 16;
	data_len |= (unsigned) (*ptr++) << 8;
	data_len |= (unsigned char) *ptr++;


	FG(read_state) = *ptr++;
	FG(frotz_more) = *ptr++;
	FG(screen_rows) = *ptr++;
	FG(screen_cols) = *ptr++;
	FG(screen_fcolor) = *ptr++;
	FG(screen_bcolor) = *ptr++;
	FG(color) = *ptr++;

	FG(frotz_index) = (long) (*ptr++) << 16;
	FG(frotz_index) = (unsigned) (*ptr++) << 8;
	FG(frotz_index) = (unsigned char) (*ptr++);


	FG(frotz_alloc) = FG(screen_rows) * FG(screen_cols);

	return 1;
}

int frotz_restore(int type) 
{
	unsigned char *ptr;

	ptr = frotz_get_type(type);

	switch (type) {
		case FROTZ_SAVE_GAME:
			return frotz_restore_game(ptr);
			break;
		case FROTZ_SAVE_SETTINGS:
			return frotz_restore_settings(ptr);
			break;
		case FROTZ_SAVE_SCROLL:
			return frotz_restore_scroll(ptr);
			break;
	}
	return 0;
}

static int frotz_save_game() 
{
	long pc;
	int i,j, alloced, data_len, tmp_len, ptr_len;
	zword success = 0, addr, nsp, nfp;

	char tc;
	unsigned char skip;
	unsigned char *ptr;
	


	FLS_FETCH();

	nsp = (int) (sp - stack);

	/*
	 * Increase this if you add more data.
	 */
	alloced = sizeof(*ptr)* ( (h_dynamic_size*2) + ((STACK_SIZE-nsp)*2) + 12 + 37);
	if ( (ptr = (unsigned char*)emalloc(alloced)) == NULL) {
		php_error(E_WARNING, "Couldn't allocate memory for save game");
		return 0;
	}
	ptr_len = 0;

	ptr[ptr_len++] = 2; /* how many data items */

    ptr_len += frotz_save_settings(ptr+ptr_len);

	/* State of Game file */
	ptr[ptr_len++] = FROTZ_SAVE_GAME;
	data_len = ptr_len++;
	ptr[ptr_len++] = 0; /* reserved space */
	ptr[ptr_len++] = 0; /* reserved space */


    /* saves z-machine internal screen information */
    ptr_len += screen_save(ptr+ptr_len);


	ptr[ptr_len++] = ((int) hi (h_release));
	ptr[ptr_len++] = ((int) lo (h_release));
	ptr[ptr_len++] = ((int) hi (h_checksum));
	ptr[ptr_len++] = ((int) lo (h_checksum));

	GET_PC (pc)

	ptr[ptr_len++] = ((int) (pc >> 16) & 0xff);
	ptr[ptr_len++] = ((int) (pc >> 8) & 0xff);
	ptr[ptr_len++] = ((int) (pc) & 0xff);

	nsp = (int) (sp - stack);
	nfp = (int) (fp - stack);

	ptr[ptr_len++] = ((int) hi (nsp));
	ptr[ptr_len++] = ((int) lo (nsp));
	ptr[ptr_len++] = ((int) hi (nfp));
	ptr[ptr_len++] = ((int) lo (nfp));

	for (i = nsp; i < STACK_SIZE; i++) {
		ptr[ptr_len++] = ((int) hi (stack[i]));
		ptr[ptr_len++] = ((int) lo (stack[i]));
	}

	fseek (story_fp, 0, SEEK_SET);

	for (addr = 0, skip = 0; addr < h_dynamic_size; addr++) {
		tc = fgetc (story_fp);
		if (feof(story_fp) ) {
			php_error(E_WARNING, "story read past eof while saving");
			break;
		}
		if (zmp[addr] != tc || skip == 255 || addr + 1 == h_dynamic_size) {
		ptr[ptr_len++] = (skip);
		ptr[ptr_len++] = (zmp[addr]);
		skip = 0;
		} else skip++;
	}
	tmp_len = (ptr_len) - data_len;
	ptr[data_len] = ((tmp_len >> 16) & 0xff);
	ptr[data_len+1] = ((tmp_len >>  8) & 0xff);
	ptr[data_len+2] = (tmp_len & 0xff);
	/* end of state of game */


	/* Success */
	success = 1;

	/* save memory: we initally got a big piece so now throw it away */
	if ( (ptr = erealloc(ptr, sizeof(*ptr)*ptr_len)) == NULL) {
		php_error(E_WARNING, "Couldn't realloc memory for save game");
		success = 0;
	}

	FLS_FETCH();
	FG(game_str) = ptr;
	FG(game_len) = ptr_len;


	return success;

}

static int frotz_save_settings(unsigned char *ptr)
{
    int ptr_len = 0, data_len, tmp_len;

    FLS_FETCH();

	/* State of game Settings */
	ptr[ptr_len++] = FROTZ_SAVE_SETTINGS;
	data_len = ptr_len++; 
	ptr[ptr_len++] = 0; /* reserved space */
	ptr[ptr_len++] = 0; /* reserved space */

	ptr[ptr_len++] = FG(read_state);
	ptr[ptr_len++] = FG(frotz_more);

	ptr[ptr_len++] = FG(screen_rows);
	ptr[ptr_len++] = FG(screen_cols);
	ptr[ptr_len++] = FG(screen_fcolor);
	ptr[ptr_len++] = FG(screen_bcolor);
	ptr[ptr_len++] = FG(color);

	ptr[ptr_len++] = ((FG(frotz_index) >> 16) & 0xff);
	ptr[ptr_len++] = ((FG(frotz_index) >> 8) & 0xff) ;
	ptr[ptr_len++] = (FG(frotz_index) & 0xff);

	tmp_len = (ptr_len) - data_len;
	ptr[data_len] = ((tmp_len >> 16) & 0xff);
	ptr[data_len+1] = ((tmp_len >>  8) & 0xff);
	ptr[data_len+2] = (tmp_len & 0xff);
	/* End of game settings */

    return ptr_len;

}

static int frotz_save_scroll(unsigned char *ptr) 
{
	int  game_len,tmp_len, cur_len=0, alloc, i,j;
	unsigned char *game_str;
	frotz_char **scr;


	FLS_FETCH();

	scr = FG(frotz_scroll);
	if (scr == NULL)  /* Oops. */
		return 0;

	game_str = FG(game_str);
	game_len = FG(game_len);

	if (ptr != NULL)  {
		/* Make adjustment */
		ptr++;
		cur_len = (long) (*ptr++) << 16;
		cur_len |= (unsigned) (*ptr++) << 8;
		cur_len |= (unsigned char) *ptr++;
		game_len -= cur_len;

	} else {
		/* We don't have  a scroll buffer */

		/* any game_str ? */
		if (game_str == NULL)
			return 0;

		cur_len = 0;
		(*game_str)++; /* increase the count by one 
						  this holds how many items we
						  have in the string */
	}

	/* Multiply frotz_alloc number of items in frotz_char 
	 * add 1 for the type
	 * add 3 for the size
	 * add 1 for count of scrolls */

	alloc = ( (FG(frotz_alloc)*4) * FG(scroll_size) ) + 5 + game_len;
	if ( (game_str = (unsigned char *)erealloc(game_str,sizeof(*game_str) * alloc)) == NULL) {
		php_error(E_ERROR, "Couldn't save scroll buffer: out of memory");
	}

	ptr = game_str+game_len; /* set the pointer to end of game_str */

	*ptr++ = FROTZ_SAVE_SCROLL; /* Type of data follows */


	tmp_len = (alloc - game_len);
	*ptr++ = ((tmp_len >> 16) & 0xff);
	*ptr++ = ((tmp_len >>  8) & 0xff);
	*ptr++ = (tmp_len & 0xff); 

	*ptr++ = FG(scroll_size);   /* Contains this many buffers */

	/* Now Write the data */
	for(i=0; i < FG(scroll_size); i++) {
		for(j=0; j < FG(frotz_alloc); j++) {
			*ptr++ = (scr[i]+j)->c;
			*ptr++ = (scr[i]+j)->color;
			*ptr++ = (scr[i]+j)->style;
			*ptr++ = (scr[i]+j)->font;
		}
	}

	/* erealloc may change the address so re-save it */
	FG(game_str) = game_str; /* Save it off */
	FG(game_len) = alloc;    /* remeber the size of string */
			
	return 1;

}

int frotz_save(int type) 
{ 
	unsigned char *ptr;

	ptr = frotz_get_type(type);

	switch(type) {
		case FROTZ_SAVE_GAME:
			if (ptr != NULL) {
				efree(FG(game_str));
			}
			return frotz_save_game();
			break;
		case FROTZ_SAVE_SETTINGS:
			if (ptr != NULL)
				return frotz_save_settings(ptr);
			break;
		case FROTZ_SAVE_SCROLL:
				return frotz_save_scroll(ptr);
			break;
	}
	php_error(E_WARNING, "frotz_save: couldn't find save type [%d]", type);
	return 0;
}

