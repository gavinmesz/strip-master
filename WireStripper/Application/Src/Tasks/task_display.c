/*
 * Display Task:
 * Manipulates outgoing frame buffer with UI Inputs.
 * Modify User config variables in stateMachine task.
 */

#include "task_manager.h" // Has FreeRTOS functions and globals defined

//specific includes
#include "task_display.h"
#include "task_stateMachine.h"
#include "task_actuatorControl.h"
#include <string.h>

#include "adc.h"
#include "spi.h"
#include "task_safety.h"
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
int stripCut; //Strip or strip and cut (1=cut)
int colour;
uint32_t adcVals1[1]; //UX pot, cut pot
uint32_t adcVals2[1]; //vbat ADC
uint32_t adcVals3[2]; //Light1, Light2

#define TEST 0

#define PD1 adcVals3[INLET] //photodiode 1
#define PD2 adcVals3[OUTLET] //photodiode 2
#define UX_POT adcVals1[0] //UX potentiometer

//Testing
int knob1;
int knob2;
int motorenc1;
int motorenc2;

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
        HAL_SPI_Transmit_DMA(&hspi4, frameBuf, Imagesize); //Send whole frame buffer in one shot

        status = 0;
    }

    return status;
}

//Clear Screen
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
        HAL_SPI_Transmit_DMA(&hspi4, frameBuf, Imagesize); //Send whole frame buffer in one shot

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

static int adc_to_length(int adc) {
    return (20-((20-3)*adc)/4050);
}

uint32_t tempADCpot;

void updateValues() {
    
     HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    //this function just updates all of the values read from the UI and encoders.
    knob1 = __HAL_TIM_GET_COUNTER(&htim2)/2; //knob 1
    knob2 = __HAL_TIM_GET_COUNTER(&htim5)/2; //knob 2
    motorenc1 = __HAL_TIM_GET_COUNTER(&htim3);
    motorenc2 = __HAL_TIM_GET_COUNTER(&htim4);
    //tim3 is motor encoder 1
    //tim4 is motor encoder 2
    // HAL_ADC_PollForConversion(&hadc2, 1);//potentiometer
    // tempADCpot = HAL_ADC_GetValue(&hadc2);

    if (TEST) {
        //Gavin's dumbass crap
        length = 100;
        stripLength = 16;
        stripCut = 1;
        quantity = 1;
    } else {
        //cool stuff
        if (systemState!=JOB_RUNNING) {
            quantity = knob1;
            if(quantity == 0){quantity = 1;}
            if (knob2<=0){length = 70;}
            else{length = 70+knob2*10;}
            stripLength = adc_to_length(adcVals1[0]);
            stripCut = HAL_GPIO_ReadPin(UX_SW_GPIO_Port, UX_SW_Pin);
            if (stripCut){stripLength = 0;}
        }
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
    }
}

void drawScreen() {
    
    if (TEST) {
        //Gavin's cool ass crap 
        //Paint functions from GUIPaint.c if the values are different
        Paint_DrawNum(10, 0, knob2, &Font8, 2, WHITE, BLACK); //WORKS THIS IS CUT KNOB
        Paint_DrawNum(60, 0, knob1, &Font8, 2, WHITE, BLACK); //WORKS, 2 per detent THIS IS QTY Knob
        uint8_t result = 0;
        if (HAL_GPIO_ReadPin(UX_KNOB1_BUT_GPIO_Port, UX_KNOB1_BUT_Pin)) {
            result += 1;
        }
        if (HAL_GPIO_ReadPin(UX_KNOB2_BUT_GPIO_Port, UX_KNOB2_BUT_Pin)) {
            result += 1;
        }
        Paint_DrawNum(90, 0, result, &Font8, 2, WHITE, BLACK); //WORKS: Pull up MCU side

        Paint_DrawNum(10, 13, motorenc1, &Font8, 2, BLACK, WHITE); //UNKNOWN
        Paint_DrawNum(60, 13, motorenc2, &Font8, 2, BLACK, WHITE); //UNKNOWN

        Paint_DrawNum(10, 26, adcVals3[0], &Font8, 2, WHITE, BLACK); // WORKS: ADC data width set to word + circular
        Paint_DrawNum(80, 26, adcVals3[1], &Font8, 2, WHITE, BLACK); // WORKS: ADC data width set to word + circular

        Paint_DrawNum(10, 39, UX_POT, &Font8, 2, BLACK, WHITE); //WORKS STRIP KNOB
        Paint_DrawNum(90, 39, packCurrent, &Font8, 2, BLACK, WHITE); //WORKS

        result = 0;
        if (HAL_GPIO_ReadPin(GAUGE_IN_GPIO_Port, GAUGE_IN_Pin)) {
            result += 1;
        }
        Paint_DrawNum(10, 52, result, &Font8, 2, BLACK, WHITE); //WORKS
        result = 0;
        if (HAL_GPIO_ReadPin(UX_SW_GPIO_Port, UX_SW_Pin)) {
            result += 1;
        }
        Paint_DrawNum(60, 52, result, &Font8, 2, BLACK, WHITE); //WORKS THIS IS THE STRIP+CUT switch
        result = 0;
        if (HAL_GPIO_ReadPin(STOP_BUT_GPIO_Port, STOP_BUT_Pin)) {
            result+=1;
        }
        if (HAL_GPIO_ReadPin(GO_BUT_GPIO_Port, GO_BUT_Pin)) {
            result+=1;
        }
        Paint_DrawNum(90, 52, result, &Font8, 2, WHITE, BLACK); //WORKS
    } else {
        
                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

                int fontWidth = 5;//5 for 8, 7 for 12
                int bottomOffset = 64 - 8;//8 is the fontsize
                sFONT fontsize = Font8;//8 works


                //QUANTITY READOUT
                //Paint_DrawRectangle(0,0,128,64,BLACK,2,1);//Little Screen Wiper
                Paint_DrawString_EN(0,0,"QTY:",&fontsize,WHITE,BLACK);
                Paint_DrawNum(4*fontWidth,0,quantity,&fontsize,1,WHITE,BLACK);
                if ((quantity < 10) & (quantity!=100)){Paint_DrawRectangle(5*fontWidth,0,9*fontWidth,8,BLACK,1,1);}
                else if(quantity ==100){Paint_DrawRectangle(7*fontWidth,0,9*fontWidth,8,BLACK,1,1);}
                else{Paint_DrawRectangle(6*fontWidth,0,9*fontWidth,8,BLACK,1,1);}

                //MODE READOUT
                Paint_DrawString_EN(12*fontWidth,0,"STATE:",&fontsize,WHITE,BLACK);


                //LEN READOUT (font size 8 )
                Paint_DrawString_EN(0,bottomOffset,"CUTLEN:",&fontsize,WHITE,BLACK);
                Paint_DrawNum(7*fontWidth,bottomOffset,(length/10),&fontsize,2,WHITE,BLACK);
                Paint_DrawString_EN((7+4)*fontWidth,bottomOffset,"cm",&fontsize,WHITE,BLACK);

                //STRIP Length READOUT
                //This should probably have a range of 3 to 20mm 
                Paint_DrawString_EN(14*fontWidth,bottomOffset,"STRIP:",&fontsize,WHITE,BLACK);
                

                int wireEndCoord = 32 + 7 +(3* stripLength) - 9;// the end x coordinate for the wire enclosure when drawn

                Paint_DrawRectangle(30,22,98,42,BLACK,2,1);//Little Screen Wiper
                if (stripCut){//cut mode, no strip length required
                    Paint_DrawRectangle(wireEndCoord,24,96,40,WHITE,2,0);//the "sheathed" part of the wire
                    Paint_DrawString_EN((14+6)*fontWidth,bottomOffset,"N/A  ",&fontsize,WHITE,BLACK);
                    Paint_DrawString_EN(wireEndCoord+5,26,"CUT MODE",&Font12,WHITE,BLACK);
                }
                else{
                    Paint_DrawNum((14+6)*fontWidth,bottomOffset,stripLength,&fontsize,2,WHITE,BLACK);
                    Paint_DrawString_EN((14+6+3)*fontWidth,bottomOffset,"mm",&fontsize,WHITE,BLACK);
                    Paint_DrawRectangle(32,27,wireEndCoord,37,WHITE,2,1);//the exposed part of the wire
                    Paint_DrawRectangle(wireEndCoord,24,96,40,WHITE,2,0);//the "sheathed" part of the wire
                }
                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                

        switch (systemState) {
            case (JOB_RUNNING): { //parameters locked, maybe show how many wires have been processed
                int toDo = quantity-finishedWires;
                Paint_DrawNum((12+6)*fontWidth,0,toDo,&fontsize,2,WHITE,BLACK);        
                Paint_DrawString_EN((12+9)*fontWidth,0,"Left",&fontsize,WHITE,BLACK);        

                //update the state thing in the corner saying progress
                break;
            }
            case (HALT): { //System faulted, require restart on stop button press
                //put up a big screen displaying the error
                switch(error_status){
                    case (SYSTEM_OK): {
                        Paint_DrawRectangle(0,0,128,64,BLACK,2,1);//Little Screen Wiper
                        Paint_DrawString_EN(0,26,"ERROR: Ok?",&Font12,WHITE,BLACK); 
                        break;                       
                    }
                    case (BATTERY_DEAD): {
                        Paint_DrawRectangle(0,0,128,64,BLACK,2,1);//Little Screen Wiper
                        Paint_DrawString_EN(0,26,"ERROR:Battery Dead",&Font12,WHITE,BLACK); 
                        break;                       
                    }
                    case (ESTOP): {
                        Paint_DrawRectangle(0,0,128,64,BLACK,2,1);//Little Screen Wiper
                        Paint_DrawString_EN(0,26,"ERROR:EStop",&Font12,WHITE,BLACK); 
                        break;                       
                    }
                    case (BUCK_FAIL): {
                        Paint_DrawRectangle(0,0,128,64,BLACK,2,1);//Little Screen Wiper
                        Paint_DrawString_EN(0,26,"ERROR:Buck Fail",&Font12,WHITE,BLACK); 
                        break;                       
                    }
                    case (OTHER_ERROR): {
                        Paint_DrawRectangle(0,0,128,64,BLACK,2,1);//Little Screen Wiper
                        Paint_DrawString_EN(0,26,"ERROR: Other",&Font12,WHITE,BLACK); 
                        break;                       
                    }
                }
                break;
            }
            default: { //Normal state, configuration
                Paint_DrawString_EN((12+6)*fontWidth,0,"Free",&fontsize,WHITE,BLACK);
                
                Paint_DrawRectangle((12+6+4)*fontWidth+1,0,(12+6+4+3)*fontWidth,8,BLACK,2,1);//clears out the "eft"
                break;
            }
            
        }
        //cool swagged out UI stuff 
        
    }
    


}

//Main display task. Infinite loop will run.
void vDisplayTask()
{
    HAL_GPIO_WritePin(LIGHT_ON1_GPIO_Port, LIGHT_ON1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LIGHT_ON2_GPIO_Port, LIGHT_ON2_Pin, GPIO_PIN_SET);
    //Colour is just a number for what I was testing, it's not a colour.
    Paint_SelectImage(ImgBuffer);
    Paint_Clear(BLACK);

    //Testing
    knob1 = 0;
    knob2 = 0;
    motorenc1 = 0;
    motorenc2 = 0;

    ((&htim2)->Instance->CNT = (0));
    ((&htim5)->Instance->CNT = (0));

    for(;;){

        switch (systemState) {
            default: {
                updateValues();
                drawScreen();

                //Update OLED.
                if (!OLED_Update(ImgBuffer)) {
                    colour++;
                }
            }
        }

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