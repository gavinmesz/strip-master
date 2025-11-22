cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

project(WireStripper C ASM)
message("Build type: ${CMAKE_BUILD_TYPE}")

enable_language(C ASM)

add_executable(${PROJECT_NAME}
        Application/Src/Services/task_manager.c
        Application/Src/Tasks/task_display.c
        Application/Src/Tasks/task_actuatorControl.c
        Core/User/Config/DEV_Config.c
        Core/User/Example/ImageData.c
#        Core/User/Example/OLED_2in42_test.c
        Core/User/Fonts/font8.c
        Core/User/Fonts/font12.c
        Core/User/Fonts/font12CN.c
        Core/User/Fonts/font16.c
        Core/User/Fonts/font20.c
        Core/User/Fonts/font24.c
        Core/User/Fonts/font24CN.c
        Core/User/GUI/GUI_Paint.c
        Core/User/OLED/OLED_2in42.c
        Core/Src/main.c
        Core/Src/gpio.c
        Core/Src/adc.c
        Core/Src/i2c.c
        Core/Src/spi.c
        Core/Src/tim.c
        Core/Src/usart.c
        Core/Src/freertos.c
        Core/Src/stm32f4xx_hal_msp.c
        Core/Src/stm32f4xx_it.c
        Core/Src/stm32f4xx_hal_timebase_tim.c
        Core/Src/syscalls.c
        Core/Src/sysmem.c
        Application/Src/Tasks/task_safety.c
        Application/Inc/Tasks/task_safety.h
        Application/Src/Tasks/task_stateMachine.c
        Application/Inc/Tasks/task_stateMachine.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
        Application/Inc
        Application/Inc/Services
        Application/Inc/Tasks
        BSP/Inc
        Components/Inc
        Utils/Inc
        Core/User/Config
        Core/User/Example
        Core/User/Fonts
        Core/User/GUI
        Core/User/OLED
        Core/Inc
        Drivers/CMSIS/Device/ST/STM32F4xx/Include
        Drivers/CMSIS/Include
        Drivers/STM32F4xx_HAL_Driver/Inc
)

add_subdirectory(cmake/stm32cubemx)

# Ensure ALL CubeMX targets use Cortex-M4 floating-point ABI
foreach(tgt
        STM32_Drivers
        FreeRTOS
        USB_DEVICE
        USB_DEVICE_Library
        USB_Device_Library
)
    if (TARGET ${tgt})
        message(STATUS "Applying Cortex-M4 options to target: ${tgt}")

        target_compile_options(${tgt} PRIVATE
                -mcpu=cortex-m4
                -mthumb
                -mfpu=fpv4-sp-d16
                -mfloat-abi=hard
                -O2
                -ffunction-sections
                -fdata-sections
        )

        target_link_options(${tgt} PRIVATE
                -mcpu=cortex-m4
                -mthumb
                -mfpu=fpv4-sp-d16
                -mfloat-abi=hard
                -Wl,--gc-sections
        )
    endif()
endforeach()

target_compile_options(${PROJECT_NAME} PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -O2
        -ffunction-sections
        -fdata-sections
)

target_compile_options(STM32_Drivers PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -O2
        -ffunction-sections
        -fdata-sections
)

target_link_options(${PROJECT_NAME} PRIVATE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -Wl,--gc-sections
        -T${CMAKE_SOURCE_DIR}/STM32F446xx_FLASH.ld
)

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${PROJECT_NAME}
        stm32cubemx
)
