#include "hal/draw_stuff.h"
#include "hal/olive.h"

#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <stdbool.h>
#include <assert.h>


static UWORD *s_fb;
static bool isInitialized = false;

static int max_chars_per_line = -1;
static int max_num_lines = -1;

uint32_t screen_buff_pixels[LCD_WIDTH * LCD_HEIGHT];

static Olivec_Canvas screen_buffer = {
    .pixels = screen_buff_pixels,
    .width = LCD_WIDTH,
    .height = LCD_HEIGHT,
    .stride = LCD_WIDTH
};

void draw_stuff_init()
{
    assert(!isInitialized);
    max_chars_per_line = LCD_WIDTH / Font16.Width;
    max_num_lines = LCD_HEIGHT / Font16.Height;
    // Exception handling:ctrl + c
    // signal(SIGINT, Handler_1IN54_LCD);
    
    // Module Init
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    // LCD Init
    DEV_Delay_ms(2000);
	LCD_1IN54_Init(HORIZONTAL);
	LCD_1IN54_Clear(WHITE);
	LCD_SetBacklight(1023);


    UDOUBLE Imagesize = LCD_1IN54_HEIGHT*LCD_1IN54_WIDTH*2;
    if((s_fb = (UWORD *)malloc(Imagesize)) == NULL) {
        perror("Failed to apply for black memory");
        exit(0);
    }
    isInitialized = true;
}
void draw_stuff_cleanup()
{
    assert(isInitialized);

    draw_stuff_update_screen(" ");

    LCD_1IN54_Clear(BLACK);
    LCD_1IN54_SetBacklight(0);

    // Module Exit
    free(s_fb);
    s_fb = NULL;
	DEV_ModuleExit();
    isInitialized = false;
}

void draw_stuff_update_screen(char* msg)
{
    assert(isInitialized);

    const int x = 0;
    const int y = 0;

    // process string so that whenever a \n is found the remaining space in the line is filled with spaces
    // if the full string doesn't fit cut off the extra characters
    char* buf = malloc(max_num_lines * max_chars_per_line + 1);
    int msg_i = 0;
    int buf_i = 0;
    int characters_in_current_line = 0;
    int num_lines = 0;
    while(msg[msg_i] != '\0' && num_lines < max_num_lines) {
        if(msg[msg_i] == '\n') {
            // fill remaining characters in line with spaces
            for(int i = 0; i < max_chars_per_line - characters_in_current_line; i++) {
                buf[buf_i] = ' ';
                buf_i++;
            }
            characters_in_current_line = max_chars_per_line;
            num_lines++;
        } else {
            buf[buf_i] = msg[msg_i];
            characters_in_current_line++;
            buf_i++;
        }

        assert(characters_in_current_line <= max_chars_per_line);
        if(characters_in_current_line >= max_chars_per_line) {
            characters_in_current_line = 0;
            num_lines++;
        }

        msg_i++;
    }
    buf[buf_i] = '\0';

    // Initialize the RAM frame buffer to be blank (white)
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Draw into the RAM frame buffer
    // WARNING: Don't print strings with `\n`; will crash!
    // Paint_DrawString_EN(x, y, msg, &Font16, WHITE, BLACK);
    Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    free(buf);

    // Send the RAM frame buffer to the LCD (actually display it)
    // Option 1) Full screen refresh (~1 update / second)
    // LCD_1IN54_Display(s_fb);
    // Option 2) Update just a small window (~15 updates / second)
    //           Assume font height <= 20
    // LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y+20, s_fb);
    LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y + LCD_HEIGHT, s_fb);
}

uint16_t to_16_bit_colour(int32_t colour)
{
    // rrrr rggg gggb bbbb
    uint16_t r = (OLIVEC_RED(colour) / 8) << 11;
    uint16_t g = (OLIVEC_GREEN(colour) / 4) << 5;
    uint16_t b = OLIVEC_BLUE(colour) / 8;
    return r | g | b;
}

void draw_stuff_screen(Olivec_Canvas* img)
{
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);

    Paint_Clear(WHITE);
    int img_i = 0;
    for(size_t y_draw = 0; y_draw < img->height; y_draw++) {
        for(size_t x_draw = 0; x_draw < img->width; x_draw++) {
            if(x_draw < LCD_1IN54_WIDTH && y_draw < LCD_1IN54_HEIGHT) {
                Paint_SetPixel(x_draw, y_draw, to_16_bit_colour(img->pixels[y_draw * img->width + x_draw]));
            }
            img_i++;
        }     
    }
        
    LCD_1IN54_Display(s_fb);
}

void draw_stuff_canvas(Olivec_Canvas* screen)
{
    // assert(screen->width <= LCD_WIDTH);
    // assert(screen->height <= LCD_HEIGHT);

    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);

    Paint_Clear(WHITE);
    int img_i = 0;
    for(size_t y_draw = 0; y_draw < screen->height; y_draw++) {
        for(size_t x_draw = 0; x_draw < screen->width; x_draw++) {
            if(x_draw < LCD_1IN54_WIDTH && y_draw < LCD_1IN54_HEIGHT) {
                Paint_SetPixel(x_draw, y_draw, to_16_bit_colour(screen->pixels[y_draw * screen->width + x_draw]));
            }
            img_i++;
        }     
    }
        
    LCD_1IN54_Display(s_fb);
}

void olivec_test()
{
    int width = 100;
    int height = 100;
    uint32_t green = OLIVEC_RGBA(0, 255, 0, 255);
    uint32_t red = OLIVEC_RGBA(255, 0, 0, 255);
    olivec_blend_color(&green, OLIVEC_RGBA(255, 255, 255, 0));
    uint32_t* px1 = malloc(sizeof(*px1) * width * height);
    Olivec_Canvas oc = olivec_canvas(px1, width, height, width);
    olivec_fill(oc, OLIVEC_RGBA(255, 255, 255, 0));
    olivec_triangle3c(oc, 0, 0, 50, 0, 0, 50, green, green, green);

    uint32_t* px2 = malloc(sizeof(*px2) * width * height);
    Olivec_Canvas oc2 = olivec_canvas(px2, width, height, width);
    olivec_fill(oc2, OLIVEC_RGBA(255, 255, 255, 0));
    olivec_triangle3c(oc2, 50, 50, 50, 0, 0, 50, red, red, green);


    olivec_sprite_blend(oc, 0, 0, oc2.width * 3 / 2, oc2.height * 3 / 2, oc2);
    printf("reached\n");

    printf("%hx %d %d %d %d\n", to_16_bit_colour(oc.pixels[20]), OLIVEC_RED(oc.pixels[20]), OLIVEC_GREEN(oc.pixels[20]), OLIVEC_BLUE(oc.pixels[20]), OLIVEC_ALPHA(oc.pixels[20]));
    draw_stuff_canvas(&oc);
    free(px1);
    free(px2);

    // so I always have to have a background with alpha!!!

    // uint32_t b3[2];
    // Olivec_Canvas oc3 = olivec_canvas(b3, 2, 1, 2);
    // olivec_fill(oc3, OLIVEC_RGBA(255, 255, 255, 0));
    // olivec_rect(oc3, 0, 0, 1, 1, green);
    // printf("oc3 %08X %08X\n", OLIVEC_PIXEL(oc3, 0, 0), OLIVEC_PIXEL(oc3, 1, 0));

    // uint32_t b4[2];
    // Olivec_Canvas oc4 = olivec_canvas(b4, 2, 1, 2);
    // olivec_fill(oc4, OLIVEC_RGBA(255, 255, 255, 0));
    // olivec_rect(oc4, 1, 0, 1, 1, green);
    // printf("oc4 %08X %08X\n", OLIVEC_PIXEL(oc4, 0, 0), OLIVEC_PIXEL(oc4, 1, 0));

    // olivec_sprite_blend(oc3, 0, 0, oc4.width, oc4.height, oc4);
    // printf("%08X %08X\n", OLIVEC_PIXEL(oc3, 0, 0), OLIVEC_PIXEL(oc3, 1, 0));

    // uint32_t blend_test = green;
    // olivec_blend_color(&green, OLIVEC_RGBA(255, 255, 255, 0));
    // printf("%08X\n", blend_test);
}



void draw_stuff_draw_buffer()
{
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);

    Paint_Clear(WHITE);
    int img_i = 0;
    for(size_t y_draw = 0; y_draw < screen_buffer.height; y_draw++) {
        for(size_t x_draw = 0; x_draw < screen_buffer.width; x_draw++) {
            if(x_draw < LCD_1IN54_WIDTH && y_draw < LCD_1IN54_HEIGHT) {
                Paint_SetPixel(x_draw, y_draw, to_16_bit_colour(screen_buffer.pixels[y_draw * screen_buffer.width + x_draw]));
            }
            img_i++;
        }     
    }
        
    LCD_1IN54_Display(s_fb);
}

/*
Notes:
everything ultimately seems to be drawn by Paint_SetPixel
Using some library to draw to a buffer is probably the most straight forward
*/