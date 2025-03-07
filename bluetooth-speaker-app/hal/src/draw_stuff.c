#include "hal/draw_stuff.h"

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
static const int LCD_WIDTH = 240;
static const int LCD_HEIGHT = 240;
static int max_chars_per_line = -1;
static int max_num_lines = -1;

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


void draw_stuff_update_screen2(void)
{
    assert(isInitialized);

    const int x = 0;
    const int y = 0;


    // Initialize the RAM frame buffer to be blank (white)
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Draw into the RAM frame buffer
    // WARNING: Don't print strings with `\n`; will crash!
    // Paint_DrawString_EN(x, y, msg, &Font16, WHITE, BLACK);
    // Paint_DrawString_EN(x, y, buf, &Font16, WHITE, BLACK);
    // free(buf);

    // Send the RAM frame buffer to the LCD (actually display it)
    // Option 1) Full screen refresh (~1 update / second)
    // LCD_1IN54_Display(s_fb);
    // Option 2) Update just a small window (~15 updates / second)
    //           Assume font height <= 20
    // LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y+20, s_fb);
    LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y + LCD_HEIGHT, s_fb);


    // #if 0
    // Some other things you can do!

    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    printf("Paint_Clear(WHITE)\n");
    Paint_Clear(WHITE);
    // DEV_Delay_ms(2000);

	// // Paint_SetRotate(ROTATE_90);
    // // Paint_SetRotate(ROTATE_180);
    // // Paint_SetRotate(ROTATE_270);

    // // /* GUI */
    // printf("drawing...\r\n");
    // // /*2.Drawing on the image*/
    // Paint_DrawPoint(5, 10, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);//240 240
    // Paint_DrawPoint(5, 25, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    // Paint_DrawPoint(5, 40, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    // Paint_DrawPoint(5, 55, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT);

    // Paint_DrawLine(20, 10, 70, 60, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(70, 10, 20, 60, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(170, 15, 170, 55, RED, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    // Paint_DrawLine(150, 35, 190, 35, RED, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    // Paint_DrawRectangle(20, 10, 70, 60, BLUE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Paint_DrawRectangle(85, 10, 130, 60, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Paint_DrawCircle(170, 35, 20, GREEN, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Paint_DrawCircle(170, 85, 20, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    // Paint_DrawString_EN(5, 70, "hello world", &Font16, WHITE, BLACK);
    // Paint_DrawString_EN(5, 90, "waveshare", &Font20, RED, IMAGE_BACKGROUND);

    // Paint_DrawNum(5, 160, 123456789, &Font20, GREEN, IMAGE_BACKGROUND);
	// Paint_DrawString_CN(5,200, "΢ѩ����",  &Font24CN,IMAGE_BACKGROUND,BLUE);   
    
    // // /*3.Refresh the picture in RAM to LCD*/
    // LCD_1IN54_Display(s_fb);
	// DEV_Delay_ms(2000);

    // /* show bmp */
	// printf("show bmp\r\n");	
	// GUI_ReadBmp("./assets/img/test-red.bmp");    
    // LCD_1IN54_Display(s_fb);
    // DEV_Delay_ms(2000);
    // #endif
}

uint16_t to_16_bit_colour(image_loader_rgba colour)
{
    // red is   1111 1000 0000 0000
    // green is 0000 0111 1110 0000
    // blue is  0000 0000 0001 1111
    uint16_t r = (colour.r / 16) << 11;
    uint16_t g = (colour.g / 8) << 5;
    uint16_t b = colour.b / 16;
    return r | g | b;
}

void draw_stuff_image(image_loader_image* img, int x, int y)
{

    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    printf("%d %d %d %d\n", img->x, img->y, x, y);

    Paint_Clear(WHITE);
    int img_i = 0;
    for(int y_draw = y; y_draw < y + img->y; y_draw++) {
        for(int x_draw = x; x_draw < x + img->x; x_draw++) {
            if(x_draw >= 0 && x_draw < LCD_1IN54_WIDTH && y_draw >= 0 && y_draw < LCD_1IN54_HEIGHT) {

                Paint_SetPixel(x_draw, y_draw, to_16_bit_colour(img->pixels[img_i]));
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