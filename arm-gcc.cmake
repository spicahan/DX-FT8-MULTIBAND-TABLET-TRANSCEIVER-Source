# --- arm-gcc.cmake -------------------------------------------------
# Tell CMake we're not building for macOS at all
set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  cortex-m7)

# Prevent macOS defaults that add -arch / SDK paths
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE)
set(CMAKE_OSX_SYSROOT       "" CACHE PATH   "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "" CACHE STRING "" FORCE)

# Cross‑compiler
set(CMAKE_C_COMPILER  arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY     arm-none-eabi-objcopy)
set(CMAKE_SIZE        arm-none-eabi-size)

# Optional: stop CMake from trying to run the produced code
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# -------------------------------------------------------------------
