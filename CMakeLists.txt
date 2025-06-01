cmake_minimum_required(VERSION 3.22)

if(UNIX AND NOT APPLE)
    # Linux
    set(OS_SUPPORTED TRUE)
elseif(APPLE)
    # macOS
    set(OS_SUPPORTED TRUE)
else()
    set(OS_SUPPORTED FALSE)
endif()

if(OS_SUPPORTED)
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            COMMAND cut -c1-10
            OUTPUT_VARIABLE GIT_BRANCH
            RESULT_VARIABLE BRANCH_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    if(BRANCH_RESULT EQUAL 0)
        message(STATUS "Git branch: ${GIT_BRANCH}")
        add_definitions(-DGIT_BRANCH=\"${GIT_BRANCH}\")
    else()
        message(STATUS "Git branch not found")
        add_definitions(-DGIT_BRANCH=\"\")
    endif()

    execute_process(
            COMMAND git rev-parse --short=8 HEAD
            OUTPUT_VARIABLE GIT_HASH
            RESULT_VARIABLE HASH_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    if(HASH_RESULT EQUAL 0)
        message(STATUS "Git hash: ${GIT_HASH}")
        add_definitions(-DGIT_HASH=\"${GIT_HASH}\")
    else()
        message(STATUS "Git hash not found")
        add_definitions(-DGIT_HASH=\"-.-.-\")
    endif()

    execute_process(
            COMMAND git describe --tags --abbrev=0
            OUTPUT_VARIABLE GIT_TAG
            RESULT_VARIABLE TAG_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    if(TAG_RESULT EQUAL 0)
        message(STATUS "Git tag: ${GIT_TAG}")
        add_definitions(-DGIT_TAG=\"${GIT_TAG}\")
    else()
        message(STATUS "Git tag not found")
        add_definitions(-DGIT_TAG=\"unknown\")
    endif()
endif()

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME OpenPixelOSD LANGUAGES C ASM)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

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
        ./src/fonts/update_font.c
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
