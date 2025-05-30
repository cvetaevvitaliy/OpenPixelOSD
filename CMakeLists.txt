cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME OpenPixelOSD)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

enable_language(C ASM)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

add_executable(${CMAKE_PROJECT_NAME})

add_subdirectory(cmake/stm32)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ./src/stm32g4xx/startup_stm32g431xx.s
        ./src/stm32g4xx/stm32g4xx_it.c
        ./src/stm32g4xx/system_stm32g4xx.c
        ./src/stm32g4xx/sysmem.c
        ./src/stm32g4xx/syscalls.c
        ./src/stm32g4xx/system.c
        ./src/msp/msp.c
        ./src/gpio.c
        ./src/dac.c
        ./src/dma.c
        ./src/opamp.c
        ./src/tim.c
        ./src/comp.c
        ./src/uart.c
        ./src/usb.c
        ./src/video_gen.c
        ./src/video_overlay.c
        ./src/canvas_char.c
        ./src/msp_displayport.c
        ./src/main.c

)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ./src/stm32g4xx
        ./src/msp
        ./src
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE

)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx

)


# Generate hex and bin files
set(HEX_FILE ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${BIN_FILE}
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)

# Flash the device using dfu-util
add_custom_target(flash
        COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${BIN_FILE} -R )
