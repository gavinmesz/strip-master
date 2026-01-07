/*
 * Display Task:
 * Manipulates outgoing frame buffer with UI Inputs.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_display.h"

#include <string.h>

#include "spi.h"
#include "../User/OLED/OLED_2in42.h"
#include "../GUI/GUI_Paint.h"

UWORD Imagesize;
UBYTE *ImgBuffer;
int spiFlag;

int OLED_Update(const UBYTE * Img) {
    int status = 1;

    if (!spiFlag) {
        spiFlag=1;

        //Assume SPI transfer correct.
        //Assume image buffer properly sized (1024 bytes).
        //Convert Image to array (vertical addressing as setup in register init)
        UBYTE frameBuf[Imagesize];
        for (int index=0; index<Imagesize; index++) {
            frameBuf[index] = Img[index+7-(index-8*((index)>>3))*(2)]; //Rearranging the buffer to array.
        }

        HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit_DMA(&hspi1, frameBuf, Imagesize); //Send whole frame buffer in one shot

        status = 0;
    }

    return status;
}

int OLED_Clear() {
    int status = 1;

    if (!spiFlag) {
        spiFlag=1;
        //Assume SPI transfer correct.
        //Assume image buffer properly sized (1024 bytes).
        //Convert Image to array (vertical addressing)
        UBYTE frameBuf[Imagesize];
        memset(frameBuf, 0, Imagesize);

        HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit_DMA(&hspi1, frameBuf, Imagesize); //Send whole frame buffer in one shot

        status = 0;
    }

    return status;
}

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

    Paint_NewImage(ImgBuffer, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 270, BLACK);
    vTaskDelay(500);
    Paint_SelectImage(ImgBuffer);
    vTaskDelay(500);
    Paint_Clear(BLACK);
    return 1;
}

void vDisplayTask()
{
    for(;;){
        //Interrupt flag from new data. Encoder, state machine, or
        OLED_Clear();
        //Paint new data on image

        //Update buffer, start SPI transfer using wrappers below

        vTaskDelay(100);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hspi);

    //Callback when the SPI DMA transfer is finished
    HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, GPIO_PIN_RESET);
    spiFlag=0;
}