// draw text to the LCD
#ifndef _DRAW_STUFF_H_
#define _DRAW_STUFF_H_

void draw_stuff_init();
void draw_stuff_cleanup();

// draw a message to screen. Supports \n. If the message is too long it gets cut off.
void draw_stuff_update_screen(char* message);
void draw_stuff_update_screen2(void);
#endif