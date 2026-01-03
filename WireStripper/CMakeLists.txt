cmake_minimum_required(VERSION 3.22)

project(WireStripper C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(${PROJECT_NAME}
        Application/Src/Services/task_manager.c
        Application/Src/Tasks/task_display.c
        Application/Src/Tasks/task_actuatorControl.c
        Application/Src/Tasks/task_safety.c
        Application/Src/Tasks/task_stateMachine.c

        Core/User/Config/DEV_Config.c
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

        startup_stm32f446xx.s
)

target_include_directories(${PROJECT_NAME} PRIVATE
        Application/Inc
        Application/Inc/Services
        Application/Inc/Tasks
        BSP/Inc
        Components/Inc
        Utils/Inc
        Core/User/Config
        Core/User/Fonts
        Core/User/GUI
        Core/User/OLED
        Core/Inc
        Drivers/CMSIS/Device/ST/STM32F4xx/Include
        Drivers/CMSIS/Include
        Drivers/STM32F4xx_HAL_Driver/Inc
)

# Pull in CubeMX-generated targets (STM32_Drivers, FreeRTOS, USB, etc.)
add_subdirectory(cmake/stm32cubemx)

# ---- Common CPU/FPU flags (apply to everything) ----
set(MCU_FLAGS
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
)

# Debug vs Release optimization/debug info
set(DEBUG_FLAGS   -Og -g3 -fno-omit-frame-pointer)
set(RELEASE_FLAGS -O0)

# Code/data sectioning helps reduce size
set(SECTION_FLAGS -ffunction-sections -fdata-sections)

# Apply compile flags to your app
target_compile_options(${PROJECT_NAME} PRIVATE
        ${MCU_FLAGS}
        ${SECTION_FLAGS}
        $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
        $<$<CONFIG:Release>:${RELEASE_FLAGS}>
)

# Link flags to your app
target_link_options(${PROJECT_NAME} PRIVATE
        ${MCU_FLAGS}
        -Wl,--gc-sections
        $<$<CONFIG:Debug>:-g>
)

# Apply the same compile flags to CubeMX targets (if they exist)
foreach(tgt STM32_Drivers FreeRTOS USB_DEVICE USB_DEVICE_Library USB_Device_Library)
    if(TARGET ${tgt})
        target_compile_options(${tgt} PRIVATE
                ${MCU_FLAGS}
                ${SECTION_FLAGS}
                $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
                $<$<CONFIG:Release>:${RELEASE_FLAGS}>
        )
        target_link_options(${tgt} PRIVATE
                ${MCU_FLAGS}
                -Wl,--gc-sections
                $<$<CONFIG:Debug>:-g>
        )
    endif()
endforeach()

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${PROJECT_NAME}
        stm32cubemx
)
