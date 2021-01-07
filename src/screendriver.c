#include "screendriver.h"		
#include "EPD_7in5_V2.h"		// EPD_7IN5_V2_Init/Clear/ClearBlack/Display/Sleep
#include <time.h> 

UBYTE *ImageBuffer;


int RUNNING = 0;


void exec_setup(void* args) {
	if (RUNNING) return;

	// Startup the screen
	if(DEV_Module_Init()!=0){
        return -1;
    }
    EPD_7IN5_V2_Init();
	
	// Allocate space for image buffer	
	UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0)? (EPD_7IN5_V2_WIDTH / 8 ): (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if((ImageBuffer = (UBYTE *)malloc(Imagesize)) == NULL) {
        return -1;
    }
	
	// Reset the imagebuffer to default white settings, and select
    Paint_NewImage(ImageBuffer, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);
    Paint_SelectImage(ImageBuffer);
    Paint_Clear(WHITE);

	// Clear the screen
	exec_clear(NULL);
		
	// Update state
	RUNNING = 1;
}



void exec_close(void* args) {
	if (!RUNNING) return;

	// Clear and put screen to sleep
    EPD_7IN5_V2_Clear();
    EPD_7IN5_V2_Sleep();
	
	// Free Imagebuffer
    free(ImageBuffer);
    ImageBuffer = NULL;

	// Wait and shutdown scrren
    DEV_Delay_ms(2000);
    DEV_Module_Exit();

	RUNNING = 0;
}

void exec_push(void* args) {
	if (!RUNNING) return;

	EPD_7IN5_V2_Display(ImageBuffer);
    DEV_Delay_ms(2000);
}

void exec_clear(void* args) {
	if (!RUNNING) return;

	// Refresh screen	
	// todo make this go to black then white
	EPD_7IN5_V2_Clear();				
	DEV_Delay_ms(500);

}



/*
typedef struct apply_args {
	uint16_t command;
	uint16_t x0;
	uint16_t y0;
	uint16_t x1;
	uint16_t y1;
	uint8_t col_f;
	uint8_t col_b;
	uint8_t dot_w;
	uint8_t aux;
	char* dat;
};
*/


// RETURN MESSAGES, MOVE LOOKUP TABLES TO NEW CLASS
//

const UWORD COL_TABLE[2] = {WHITE, BLACK};
const sFONT* FNT_TABLE[5] = {&Font8, &Font12, &Font16, &Font20, &Font24};

void exec_apply(void* args_) {
	if (!RUNNING) return;
	
	apply_args_t *args = (apply_args_t *) args_;
	
	switch(args->command) {
		case 0:
			Paint_Clear(COL_TABLE[args->col_f]);
			break;
		case 1:
			Paint_ClearWindows(args->x0, args->y0, args->x1, args->y1, COL_TABLE[args->col_f]);
			break;
		case 2:
			Paint_SetPixel(args->x0, args->y0, COL_TABLE[args->col_f]);
			break;
		case 3:
			Paint_DrawPoint(args->x0, args->y0, COL_TABLE[args->col_f], args->dot_w, args->aux);
			break;
		case 4:
			Paint_DrawLine(args->x0, args->y0, args->x1, args->y1, COL_TABLE[args->col_f], args->dot_w, args->aux);
			break;
		case 5:
			Paint_DrawRectangle(args->x0, args->y0, args->x1, args->y1, COL_TABLE[args->col_f], args->dot_w, args->aux);
			break;
		case 6:
			Paint_DrawCircle(args->x0, args->y0, args->x1, COL_TABLE[args->col_f], args->dot_w, args->aux);
			break;
		case 7:
			Paint_DrawString_EN(args->x0, args->y0, args->dat, FNT_TABLE[args->aux], COL_TABLE[args->col_f], COL_TABLE[args->col_b]);
			break;
		case 8:
    		GUI_ReadBmp(args->dat, args->x0, args->y0);
			break;
		default:
			printf("(debug) ignoring invalid draw command\n");
			printf("(debug) command is: %d %d %d %d %d - %d %d %d %d - %s\n", args->command, args->x0, args->y0, args->x1, args->y1, args->col_f, args->col_b, args->dot_w, args->aux, args->dat);
	}

}

void exec_flush(void* args) {
	if (!RUNNING) return;

	// Reset the image buffer to white
    Paint_Clear(WHITE);
}


// PUBLIC function pointers:
executable_t d_dosetup = &exec_setup;
executable_t d_doclose = &exec_close;
executable_t d_dopush = &exec_push;
executable_t d_doclear = &exec_clear;
executable_t d_doapply = &exec_apply;
executable_t d_doflush = &exec_flush;


/*
int testProgramSetup()
{

    Paint_DrawRectangle(5, 5, 795, 475, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawCircle(45, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    GUI_ReadBmp("./pic/clock2.bmp", 200, 100);
    GUI_ReadBmp("./pic/piazzalatest.bmp", 450, 100);
    EPD_7IN5_V2_Display(BlackImage);


#if 0  // show bmp
    printf("show window BMP-----------------\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    GUI_ReadBmp("./pic/800x480.bmp", 0, 0);
    EPD_7IN5_V2_Display(BlackImage);
    DEV_Delay_ms(2000);

    printf("show bmp------------------------\r\n");
    Paint_SelectImage(BlackImage);
    GUI_ReadBmp("./pic/100x100.bmp", 0, 0);
    EPD_7IN5_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
#endif        

#if 0   // show image for array   
    printf("show image for array\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(gImage_7in5_V2);
    EPD_7IN5_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
#endif

#if 0   // Drawing on the image
    //1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // 2.Drawing on the image
    printf("Drawing:BlackImage\r\n");
    Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawCircle(45, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(105, 95, 20, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
    Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);
    Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
    Paint_DrawString_CN(130, 0, " ÄãºÃabc", &Font12CN, BLACK, WHITE);
    Paint_DrawString_CN(130, 20, "Î¢Ñ©µç×Ó", &Font24CN, WHITE, BLACK);

    printf("EPD_Display\r\n");
    EPD_7IN5_V2_Display(BlackImage);
    DEV_Delay_ms(2000);
#endif
	return 0;
}


*/
