/*
 * * ux_screen.c
 * *
 * * Unix interface, screen manipulation
 * *
 * */

#define __UNIX_PORT_FILE

#include "php_frotz.h"
#include <frotz.h>


/*
 * * os_erase_area
 * *
 * * Fill a rectangular area of the screen with the current background
 * * colour. Top left coordinates are (1,1). The cursor does not move.
 * *
 * */
void os_erase_area (int top, int left, int bottom, int right)
{
	int top_idx, bot_idx;

	top_idx = FROTZ_INDEX_OF(top, left);
	bot_idx = FROTZ_INDEX_OF(bottom, right);

	frotz_erase_area(top_idx, bot_idx);


}/* os_erase_area */

/*
 * * os_scroll_area
 * *
 * * Scroll a rectangular area of the screen up (units > 0) or down
 * * (units < 0) and fill the empty space with the current background
 * * colour. Top left coordinates are (1,1). The cursor stays put.
 * *
 * */
void os_scroll_area (int top, int left, int bottom, int right, int units)
{
	int i,x,y,cur, pre;
	int skip=0;
	frotz_char *ptr;

	FLS_FETCH();

	if (FROTZ_INDEX_OF(bottom, right) >= FG(frotz_alloc) ) {
		php_error(E_ERROR, "scroll passed screen: t:%d l:%d b:%d r:%d u: %d -%d",
				top,left,bottom,right,units,FROTZ_INDEX_OF(bottom,right));
	}


	ptr = FG(frotz_buffer);
	for (i=0; i < units; i++) 
		for (y=top+1; y <= bottom; y++) {
			for (x=left;  x <= right; x++) {
				cur = FROTZ_INDEX_OF(y,x);
				pre = FROTZ_INDEX_OF(y-1,x);

				/* copy */
				(ptr+pre)->c = (ptr+cur)->c;
				(ptr+pre)->color = (ptr+cur)->color;
				(ptr+pre)->style = (ptr+cur)->style;
				(ptr+pre)->font = (ptr+cur)->font;

				/* clear */
				(ptr+cur)->c = 0;
				(ptr+cur)->color = FG(color);
				(ptr+cur)->style = 0;
				(ptr+cur)->font = FG(font);
			}
		}

}/* os_scroll_area */

