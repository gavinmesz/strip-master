/*
 * Display Task:
 * Manipulates outgoing frame buffer with UI Inputs.
 * Modify User config variables in stateMachine task.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_display.h"
#include "task_stateMachine.h"
#include <string.h>

#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "../User/OLED/OLED_2in42.h"
#include "../GUI/GUI_Paint.h"

UWORD Imagesize;
//Main image buffer that is setup to use the provided library to "paint" on.
UBYTE *ImgBuffer;
//frameBuf will hold the image array but in an order that the display is expecting. Needs to be global otherwise it goes out of scope.
UBYTE *frameBuf;
int spiFlag;
int quantity;
int length;
int stripLength;
int stripCut; //Strip or strip and cut (1=Strip and cut)
uint32_t adcVals[4];

/*
 * OLED_Update:
 * - Pass in image buffer
 * - Modify image buffer for horizontal addressing
 * - Return 1 if DMA SPI transfer has not finished (reset in DMA callback later in this task).
 * - Return 0 if DMA SPI transfer was initiated.
 */

int OLED_Update(const UBYTE * Img) {
    int status = 1;

    if (!spiFlag) {
        spiFlag=1;

        //Assume SPI transfer correct (image on screen is OK).
        //Assume image buffer properly sized (1024 bytes).
        //Convert Image buffer to array it's expecting given horizontal addressing
        int count=0;
        for (int page=0; page<8; page++) {
            /* write data */
            for(int column=0; column<128; column++) {
                frameBuf[count] = Img[(7-page) + column*8];
                count++;
            }
        }

        HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit_DMA(&hspi1, frameBuf, Imagesize); //Send whole frame buffer in one shot

        status = 0;
    }

    return status;
}

//Easy clear screen function.
int OLED_Clear() {
    int status = 1;

    if (!spiFlag) {
        spiFlag=1;
        //Assume SPI transfer correct.
        //Assume image buffer properly sized (1024 bytes).
        //Convert Image to array (vertical addressing)
        memset(frameBuf, 0x00, Imagesize);

        HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit_DMA(&hspi1, frameBuf, Imagesize); //Send whole frame buffer in one shot

        status = 0;
    }

    return status;
}

//Display Initialization: Setup image size, configure display, allocate space of image buffers, clear screen
int initDisplay(){
    Imagesize = ((OLED_2IN42_WIDTH + 7) / 8) * OLED_2IN42_HEIGHT;
    if(System_Init() != 0) {
        return -1;
    }

    //Initialize the Display
    OLED_2in42_Init();
    vTaskDelay(500);

    // 0.Create a new image cache
    if((ImgBuffer = (UBYTE *)pvPortMalloc(Imagesize)) == NULL) {
        return -1;
    }
    if((frameBuf = (UBYTE *)pvPortMalloc(Imagesize)) == NULL) {
        return -1;
    }

    //Paint the screen black
    Paint_NewImage(ImgBuffer, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 270, BLACK);
    Paint_SelectImage(ImgBuffer);
    vTaskDelay(500);

    //Clear the Screen
    OLED_Clear();
    vTaskDelay(500);
    return 1;
}

//Main display task. Infinite loop will run.
void vDisplayTask()
{
    //Colour is just a number for what I was testing, it's not a colour.
    int colour = 0;
    quantity = 10;
    Paint_SelectImage(ImgBuffer);
    Paint_Clear(BLACK);

    for(;;){
        //Global config variables held in task_stateMachine.c
        //High priority is to save time for other tasks as much as possible. Be mindful of unnecessary tasks (ie don't Update the OLED if ADC/encoder values have not changed).
        quantity = __HAL_TIM_GET_COUNTER(&htim2);
        //Paint functions from GUIPaint.c if the values are different
        Paint_DrawString_EN(10, 0, "waveshare", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(10, 17, "hello world", &Font8, WHITE, BLACK);
        Paint_DrawNum(10, 30, adcVals[1], &Font8, 4, WHITE, BLACK);
        Paint_DrawNum(10, 43, quantity, &Font12, 2, BLACK, WHITE);

        //Update OLED.
        if (!OLED_Update(ImgBuffer)) {
            colour++;
        }

        //Give up task. Have not setup preemption so this can starve all tasks.
        vTaskDelay(100);
    }
}

//Callback for when the DMA SPI Tx is finished.
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hspi);

    //Callback when the SPI DMA transfer is finished
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
    spiFlag=0;
}