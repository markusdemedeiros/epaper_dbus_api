/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to  whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ******************************************************************************/



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
