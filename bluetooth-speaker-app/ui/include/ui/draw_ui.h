// defines a state machine for a button
#ifndef _DRAW_UI_H
#define _DRAW_UI_H

#include "hal/olive.h"

Olivec_Canvas* draw_ui_text(char* str);
Olivec_Canvas* draw_ui_progress_bar(int width, int height, float progress, uint32_t primary_colour);
int draw_ui_blend_centered(Olivec_Canvas canvas, Olivec_Canvas sprite, int y);

#endif