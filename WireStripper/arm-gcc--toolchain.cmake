# Tell CMake we are doing an embedded bare-metal ARM build
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prevent CMake from trying to run executables (cannot run on host)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Compilers
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Optional: if needed, specify path directly
#set(TOOLCHAIN_PATH "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.3 rel1")
#set(CMAKE_C_COMPILER   "${TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc.exe")
#set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/arm-none-eabi-g++.exe")
#set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc.exe")

# Remove default host libraries (kernel32, user32, etc.)
set(CMAKE_EXE_LINKER_FLAGS_INIT "")
