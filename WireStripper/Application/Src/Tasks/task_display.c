/*
 * Display Task:
 * Manipulates outgoing frame buffer with UI Inputs.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_display.h"
#include "../User/OLED/OLED_2in42.h"
#include "../GUI/GUI_Paint.h"

volatile int basepri;
UWORD Imagesize;
UBYTE *BlackImage;

int initDisplay(){
    Imagesize = ((OLED_2IN42_WIDTH + 7) / 8) * OLED_2IN42_HEIGHT;
    // printf("2.42inch OLED test demo\n");
    vTaskSuspendAll();
    if(System_Init() != 0) {
        return -1;
    }

    //Initialize the Display
    // printf("OLED Init...\r\n");
    OLED_2in42_Init();
    vTaskDelay(pdMS_TO_TICKS(500));

    // 0.Create a new image cache
    if((BlackImage = (UBYTE *)pvPortMalloc(Imagesize)) == NULL) {
        // printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    // vPortSetBASEPRI(0); <- base priority risen to 80 issue.
    Paint_NewImage(BlackImage, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 270, BLACK);
    // Paint_SelectImage(BlackImage);
    vTaskDelay(pdMS_TO_TICKS(500));
    Paint_Clear(BLACK);
    xTaskResumeAll();
    return 1;
}

void vDisplayTask(void *argument)
{
    for(;;){
        counterVar++;
        vTaskDelay(100);
    }
}
