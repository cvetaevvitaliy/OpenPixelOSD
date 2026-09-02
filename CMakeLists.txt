cmake_minimum_required(VERSION 3.22)
# ===== Build variant selection =====
# Orthogonal build options, combine with '+' (e.g. BLINKY+NO_OSC):
#   NO_OSC   -> no external oscillator
#   BLINKY   -> minimal "blinky" bring-up build
#   MCO      -> route the master clock to a pin for verification
#   PA_DEBUG -> compile in the PA closed-loop OSD debug overlay
#               (debug_pa_loop() + its field-edge trigger); off by default
# VTX/PA is NOT a build variant -- it is a property of the selected
# TARGET_BOARD, declared in that board's target.cmake (see below).
set(BUILD_VARIANT "" CACHE STRING "Build variant: NO_OSC, BLINKY, MCO, PA_DEBUG (combine with +)")

set(TARGET_MCU "" CACHE STRING "Target MCU (e.g., STM32G431, STM32G474)")
# set default to G431 if not specified
if(NOT TARGET_MCU)
    set(TARGET_MCU STM32G431)
endif()

# ===== Board target selection =====
# Each board lives in its own directory: src/targets/<TARGET_BOARD>/ ,
# containing:
#   target.h    -- pin/resource defines + PA-type/PID constants. Always
#                  on the include path (included as "target.h").
#   target.c    -- the VTX power table. Required for a VTX board.
#   target.cmake -- OPTIONAL build-flag fragment, included below. Declares
#                  board properties the build reacts to:
#                    set(TARGET_USE_VTX TRUE)  -- compile the RTC6705/VTX
#                                                 source set + target.c,
#                                                 define USE_VTX
#                    set(TARGET_USE_PA  TRUE)  -- define USE_PA (implies VTX)
# Adding a board is just creating that directory -- nothing here or in the
# source tree needs editing.
set(TARGET_BOARD "GENERIC" CACHE STRING "Board target: directory name under src/targets/")

set(TARGET_BOARD_DIR "${CMAKE_SOURCE_DIR}/src/targets/${TARGET_BOARD}")

if(NOT EXISTS "${TARGET_BOARD_DIR}/target.h")
    file(GLOB _target_dirs RELATIVE "${CMAKE_SOURCE_DIR}/src/targets"
         "${CMAKE_SOURCE_DIR}/src/targets/*")
    set(_available_targets "")
    foreach(_d ${_target_dirs})
        if(EXISTS "${CMAKE_SOURCE_DIR}/src/targets/${_d}/target.h")
            list(APPEND _available_targets "${_d}")
        endif()
    endforeach()
    string(JOIN ", " _available_targets ${_available_targets})
    message(FATAL_ERROR "Unknown TARGET_BOARD '${TARGET_BOARD}'. Available: ${_available_targets}")
endif()

message(STATUS "Building for board target: ${TARGET_BOARD} (src/targets/${TARGET_BOARD}/)")
add_compile_definitions(TARGET_BOARD_NAME="${TARGET_BOARD}")

# Per-board build flags. A board with no target.cmake is a plain OSD board.
set(TARGET_USE_VTX FALSE)
set(TARGET_USE_PA FALSE)
include("${TARGET_BOARD_DIR}/target.cmake" OPTIONAL)
if(TARGET_USE_PA)
    set(TARGET_USE_VTX TRUE)   # a PA board always carries the RTC6705 VTX
endif()

message(STATUS "Selected TARGET_MCU=${TARGET_MCU} BUILD_VARIANT=${BUILD_VARIANT} TARGET_BOARD=${TARGET_BOARD}")

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
set(CMAKE_PROJECT_NAME OpenPixelOSD)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME} LANGUAGES C ASM)
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

add_executable(${CMAKE_PROJECT_NAME})


if(TARGET_MCU STREQUAL "STM32G431")
    message(STATUS "Building for STM32G431")
    add_compile_definitions(
            MCU_TYPE=\"STM32G431\"
            STM32G431xx
            LOW_RAM
    )
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/stm32g431cbux_flash.ld)
    set(STARTUP_ASSEMBLY_FILE ./src/stm32g4xx/startup_stm32g431xx.s)
elseif(TARGET_MCU STREQUAL "STM32G474")
    message(STATUS "Building for STM32G474")
    add_compile_definitions(
            MCU_TYPE=\"STM32G474\"
            STM32G474xx
            HIGH_RAM
    )
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/stm32g474ceux_flash.ld)
    set(STARTUP_ASSEMBLY_FILE ./src/stm32g4xx/startup_stm32g474xx.s)
    set(SOURCE src/fonts/font_system.c)
else()
    message(FATAL_ERROR "Unsupported MCU type: ${TARGET_MCU}")
endif()

#The pattern (^|\\+)${FEATURE_NAME}($|\\+) means:
#(^|\\+) - Match either start of string (^) OR a plus sign (\\+)
#${FEATURE_NAME} - Match the feature name exactly
#($|\\+) - Match either end of string ($) OR a plus sign (\\+)

# VTX/PA source set + defines -- driven by the board's target.cmake, not BUILD_VARIANT.
if(TARGET_USE_VTX)
    message(STATUS "Board ${TARGET_BOARD}: VTX enabled")
    if(NOT EXISTS "${TARGET_BOARD_DIR}/target.c")
        message(FATAL_ERROR "VTX board '${TARGET_BOARD}' must provide src/targets/${TARGET_BOARD}/target.c")
    endif()
    set(BUILD_NAME "${BUILD_NAME}_VTX")
    add_compile_definitions(USE_VTX)
    set(VTX_SRC
            ./src/rtc6705.c
            ./src/rf_pa.c
            ./src/vtx_msp.c
            ./src/vtx_power_levels.c
            "${TARGET_BOARD_DIR}/target.c")
endif()

if(TARGET_USE_PA)
    message(STATUS "Board ${TARGET_BOARD}: PA enabled")
    add_compile_definitions(USE_PA)
endif()

if(BUILD_VARIANT MATCHES "(^|\\+)BLINKY($|\\+)")
    message(STATUS "Building 'blinky' variant")
    set(BUILD_NAME "${BUILD_NAME}_BLINKY")
    add_compile_definitions(BUILD_VARIANT_BLINKY)
endif()

if(BUILD_VARIANT MATCHES "(^|\\+)NO_OSC($|\\+)")
    message(STATUS "Building with no external oscillator support")
    set(BUILD_NAME "${BUILD_NAME}_NO_OSC")
    add_compile_definitions(BUILD_VARIANT_NO_OSC)
endif()

if(BUILD_VARIANT MATCHES "(^|\\+)MCO($|\\+)")
    message(STATUS "Building with MCO verification support (PA8 should have a 10.625 clock signal on it)")
    set(BUILD_NAME "${BUILD_NAME}_MCO")
    add_compile_definitions(BUILD_VARIANT_MCO)
endif()

if(BUILD_VARIANT MATCHES "(^|\\+)PA_DEBUG($|\\+)")
    message(STATUS "Building with the PA closed-loop OSD debug overlay enabled")
    set(BUILD_NAME "${BUILD_NAME}_PA_DEBUG")
    add_compile_definitions(BUILD_VARIANT_PA_DEBUG)
endif()

add_compile_definitions(
        USE_FULL_LL_DRIVER
        USE_HAL_DRIVER
)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${LINKER_SCRIPT}")

add_subdirectory(cmake/stm32)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${STARTUP_ASSEMBLY_FILE}
        ${SOURCE}
        ${VTX_SRC}
        ./src/stm32g4xx/stm32g4xx_it.c
        ./src/stm32g4xx/system_stm32g4xx.c
        ./src/stm32g4xx/sysmem.c
        ./src/stm32g4xx/syscalls.c
        ./src/stm32g4xx/system.c
        ./src/msp/msp.c
        ./src/adc.c
        ./src/canvas_char.c
        ./src/comp.c
        ./src/dac.c
        ./src/dma.c
        ./src/flash.c
        ./src/gpio.c
        ./src/led.c
        ./src/msp_displayport.c
        ./src/msp_fc.c
        ./src/opamp.c
        ./src/tim.c
        ./src/video_gen.c
        ./src/video_graphics.c
        ./src/video_overlay.c
        ./src/uart.c
        ./src/usb.c
        ./src/main.c
        ./src/fonts/update_font.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${TARGET_BOARD_DIR}
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
set(HEX_FILE ${CMAKE_PROJECT_NAME}_${TARGET_MCU}${BUILD_NAME}.hex)
set(BIN_FILE ${CMAKE_PROJECT_NAME}_${TARGET_MCU}${BUILD_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${BIN_FILE}
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)

# make a zip file containing the hex, bin, map and elf
set(ZIP_NAME "${CMAKE_PROJECT_NAME}_${TARGET_MCU}_${CMAKE_BUILD_TYPE}${BUILD_NAME}")
set(MAP_FILE "${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")

# Create ZIP after build completes
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Creating ${ZIP_NAME}.zip"
        COMMAND ${CMAKE_COMMAND} -E tar "cf" "${ZIP_NAME}.zip" --format=zip
        "${HEX_FILE}"
        "${BIN_FILE}"
        "${MAP_FILE}"
        "$<TARGET_FILE:${PROJECT_NAME}>"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Creating ZIP archive: ${ZIP_NAME}.zip"
)

# Flash the device using dfu-util
add_custom_target(flash
        COMMAND dfu-util -a 0 -s 0x08000000:leave -D ${PROJECT_BINARY_DIR}/${BIN_FILE} -R )
