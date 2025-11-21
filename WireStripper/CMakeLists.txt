cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME WireStripper)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create the executable target FIRST
add_executable(${CMAKE_PROJECT_NAME}
        Application/Inc/Services/task_manager.h
        Application/Inc/Tasks/task_display.h
        Application/Inc/Tasks/task_watchdog.h
        Application/Src/Services/task_manager.c
        Application/Src/Tasks/task_display.c
        Application/Src/Tasks/task_watchdog.c
        Core/User/Config/DEV_Config.c
        Core/User/Example/ImageData.c
        Core/User/Example/OLED_2in42_test.c
        Core/User/Fonts/font8.c
        Core/User/Fonts/font12.c
        Core/User/Fonts/font12CN.c
        Core/User/Fonts/font16.c
        Core/User/Fonts/font20.c
        Core/User/Fonts/font24.c
        Core/User/Fonts/font24CN.c
        Core/User/GUI/GUI_Paint.c
        Core/User/OLED/OLED_2in42.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined include paths
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

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
