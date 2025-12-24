//main includes
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

//specific includes
#include "task_display.h"
#include "../User/OLED/OLED_2in42.h"
#include "../GUI/GUI_Paint.h"
#include "usart.h"

int COUNTER_VAR = 0;

int initDisplay(){
    // printf("2.42inch OLED test demo\n");

    if(System_Init() != 0) {
        return -1;
    }

    //Initialize the Display
    // printf("OLED Init...\r\n");
    OLED_2in42_Init();
    HAL_Delay(pdMS_TO_TICKS(500));

    // 0.Create a new image cache
    if((BlackImage = (UBYTE *)pvPortMalloc(Imagesize)) == NULL) {
        // printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    Paint_NewImage(BlackImage, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 270, BLACK);
    Paint_SelectImage(BlackImage);
    HAL_Delay(pdMS_TO_TICKS(500));
    Paint_Clear(BLACK);
    return 1;
}

void vDisplayTask(void *argument)
{
    //Streets saying we should have SPI running with DMA

    /* USER CODE BEGIN StartTask02 */

    uint8_t dispMsg[] = {1};
    /* Infinite loop */
    for(;;)
    {
        HAL_UART_Transmit(&huart3, dispMsg, 1, 1000);
        // printf("Display: %d\r\n", COUNTER_VAR);
        // Paint_DrawString_EN(10, 0, "waveshare", &Font16, WHITE, BLACK);
        // Paint_DrawString_EN(10, 17, "hello world", &Font8, WHITE, BLACK);
        // Paint_DrawNum(10, 30, COUNTER_VAR, &Font8, 4, WHITE, BLACK);
        // Paint_DrawNum(10, 43, 987654, &Font12, 5, WHITE, BLACK);
        // OLED_2in42_Display(BlackImage);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    /* USER CODE END StartTask02 */
}
