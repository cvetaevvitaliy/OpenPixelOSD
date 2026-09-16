# OpenPixelOSD
Originally made by @cvetaevvitaliy

OpenPixelOSD is an open-source project for generating and overlaying pixel graphics onto a video signal (OSD), based on the **STM32G431CBUx/** microcontroller.
The project aims to create a software monochrome On-Screen Display (OSD) for VPV to phase out the obsolete *MAX7456* chip which has been discontinued

## Repository Structure
```pgsql
.github/workflows/                   - CI build for STM32G431/G474
doc/                                 - diagrams and images
cmake/                               - toolchain & stm32 library CMake scripts
python/                              - helper scripts (font/logo conversion, font upload)
USB_Device/, Drivers/, Middlewares/  - STM32Cube generated code
src/                                 - firmware source (C code)
  hardware/                          - peripheral initialization, UART and USB
  render/                            - video generation, overlay and DisplayPort
  stm32g4xx/                         - device startup, HAL configuration
  msp/                               - MSP protocol parser/handler
  fonts/, logo/                      - embedded font/logo data and updater
CMakeLists.txt                       - top‑level build file
```

#### Key Components

- `src/main.c` – program entry. Initializes hardware modules, then continually processes **MSP** messages and blinks an **LED**.

- `src/render/video_overlay.c` – core video overlay logic. Sets up **DACs**, timers and comparators to mix the generated OSD pixels with the incoming video signal.

- `src/msp/` – implements the **MultiWii Serial Protocol** (MSP) for interacting with flight controllers.

- `src/render/canvas_char.*` – double‑buffered character canvas used for text rendering onto the video overlay.

- `python/` – scripts to convert fonts or bitmaps into C arrays for embedding. For example, `convert_logo.py` reads an image, maps colors to 2‑bpp pixels, and outputs a header file. `font_updater.py` sends font data over serial using **MSP** commands.

#### Build and Continuous Integration

The GitHub workflow in `.github/workflows/build.yml` checks out the repo, installs the **Arm GNU toolchain**, builds the firmware for both MCU variants, and uploads the `.hex` and `.bin` artifacts.

Build with CMake 3.22+, Ninja and Arm GNU Toolchain on PATH:

```sh
cmake -S . -B build -G Ninja -DTARGET_MCU=STM32G431 -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
python python/check_includes.py build/compile_commands.json
```

Use a separate build directory with `-DTARGET_MCU=STM32G474` for the larger MCU.
CMake selects the matching startup file, linker script and `LOW_RAM`/`HIGH_RAM`
defines. Each build produces HEX, BIN, ELF, MAP and a ZIP archive.
The include checker also verifies exact capitalization on Windows; CI runs it
for both MCU targets. Project includes use paths relative to `src/` when crossing
module directories, such as `hardware/usb.h` and `render/canvas_char.h`.
VS Code reads include paths and defines from `build/compile_commands.json`.

The main-loop switches are in `src/main.h`: `USE_MSP` is enabled,
`DEBUG_LED_BLINK` is enabled, and `PAUSE_ON_FAILED_INIT` is set to 1.

#### What to Explore Next

* **Hardware setup** – Review `README.md` and the diagrams in `doc/` to understand signal routing and timing requirements.

* **Build system** – Read `CMakeLists.txt` and `cmake/` scripts to learn how the project is compiled and linked for different microcontrollers.

* **Video overlay internals** – Study `video_overlay.c` and `video_gen.c` to see how DMA and timers generate the analog video waveform.

* **MSP protocol** – Examine `src/msp/` to understand communication with flight controllers.

* **Python utilities** – The scripts in `python/` show how fonts and logos are converted to C data for flashing onto the device.

The project is licensed under GPL‑2.0, as noted in the LICENSE file. With these parts in mind, a newcomer can navigate the firmware, experiment with modifying fonts/logo data, and extend the OSD functionality.

*Important configuration options live in the main `CMakeLists.txt`. The `TARGET_MCU` variable switches between `STM32G431` and `STM32G474` builds.*

## How It Works

### Hardware Architecture

- The **STM32G431CBUx/G474 microcontroller** handles control and pixel data generation.
- The input video signal is fed into the **OPAMP1 multiplexer input - PA7**, operating in **follower mode**, and to the comparator positive input `vin+` **COMP3 - PA0** for synchronization.
- **DAC1 Channel 1** is used as the negative reference `vref-` for the comparator.
- If no input video signal is present, **TIM17** generates a PWM signal on pin **PB5** to create the video signal.
- To simplify the video signal detection logic and the generation of the internal video signal **TIM17** generates **reference video** signal by producing **PWM** on pin **PB5**, supporting interlaced or progressive scanning.
- Due to the lack of internal synchronization between **TIM17**, **DMA1 Channel 6**, and **TIM1** for precise line start synchronization of the generated video signal, **COMP4** is used, connected via a **1:10** resistive divider to the `vin+` input of **COMP4** and to the **PA3** input of the **OPAMP1** multiplexer.
- Comparators **COMP3** and **COMP4** are used for video signal "parsing" and line start detection.
- The output video signal is formed by mixing (fast switching) signals from the internal **DAC3 Channel 1** and the input video **PA7** via the built-in multiplexer.
- **TIM2** and **TIM3** provide precise synchronization for line and frame start:
  - TIM2 and TIM3 track line start and trigger pixel rendering accordingly.
  - TIM1 handles pixel rendering in the line by transferring two buffers via **DMA1 Channel 1** and **DMA1 Channel 2**.
  - DMA1 Channel 1 transfers the buffer containing precise timing information for switching the multiplexer connected to the OPAMP1 input.
  - DMA1 Channel 2 transfers the buffer containing brightness values for each pixel to **DAC3 Channel 1** for pixel formation in the line.

### Block Diagram
![OpenPixelOSD Block Diagram](doc/pic/internal-block-diagram.png)

### Wiring Diagram
Wiring diagram for quick project launch on WeActStudio:
- https://github.com/WeActStudio/WeActStudio.STM32G431CoreBoard
- https://github.com/WeActStudio/WeActStudio.STM32G474CoreBoard

![OpenPixelOSD Wiring Diagram](doc/pic/openpixelosd-wiring.png)

### Operating Principle

1. **Video Synchronization:** Comparators detect the start of a video line, enabling synchronization of rendering with the input signal.
2. **Timing:** Timers generate precise timing for the start of each pixel display in the line and manage DMA transactions for data transfer.
3. **Rendering:** The processor generates pixel data as a monochrome image with grayscale shades in buffers, which DMA then transfers to the DAC and OPAMP.
4. **Signal Formation:** Using the DAC and internal multiplexer OPAMP, the output video signal with the overlaid **OSD**.

### Reference for PAL/NTSC Standards

For detailed information on PAL and NTSC video standards, please refer to the comprehensive article by Martin Hinner:
https://martin.hinner.info/vga/pal.html

This resource provides authoritative timing diagrams, signal structures, and technical explanations essential for accurate video signal generation and processing.

# Key Features

- No external chips required for video signal detection and generation.
- Utilizes hardware timers and DMA to minimize CPU load.
- CCMRAM is used for fast access to critical data and code.
- Precise synchronization is supported via hardware comparators.
- Software scalability: real-time pixel rendering.
- Using the DAC to control pixel brightness.

## Connection and Setup

### Dual-camera fork

Connect camera 1 video to PA3, camera 2 video to PA7, and take the mixed
video/OSD output from PA2. Both cameras must share ground with the board.
The default input is PA7, matching the Telekatz generic firmware.
This fork uses COMP2 for sync on the selected camera; the original PA0
sync connection and PA3 fallback-generator connection shown above do not apply.

After `video_overlay_init()`, select the input with
`set_video_input(VIDEO_INPUT_1)` or `set_video_input(VIDEO_INPUT_2)`.
The switch stops pixel DMA, clears both line buffers and waits for field
sync from the new camera while preserving the DisplayPort canvas. The new
camera passes through while the overlay reacquires sync. An input without
video cannot display OSD because the fallback generator is disabled.

The current `led_blink()` demo switches cameras every five seconds. Remove
the camera-selection call there when controlling the function elsewhere.
MSP DisplayPort uses 115200 baud, PA10 RX (connect FC TX) and PA9 TX
(connect FC RX), matching Telekatz. `INVERT_UART` is disabled by default;
this option swaps the physical TX/RX pins, not signal polarity.
The startup logo stays visible until the flight controller submits a
DisplayPort frame. Messages are processed immediately, including while
the logo is visible.

The logo is copied from its dedicated flash region into a packed RAM cache
before video interrupts start. Its renderer runs in CCMRAM and skips empty
margins; transparent glyph rows under the logo also skip pixel writes.
This reduces the extra work per logo scanline without changing the camera
switch or synchronization settings. Clipping never overwrites the final
camera-passthrough word in the DMA buffer.

The pixel start uses the TIM2 -> TIM15 -> TIM1 hardware timing path from
[Telekatz/OpenPixelOSD](https://github.com/Telekatz/OpenPixelOSD/blob/6da54b9/src/tim.c).
The 360-pixel line fits inside 50 us; TIM2 CH3 stops pixel output at 57 us
to avoid overwriting the next sync pulse. The DAC's first pixel is preloaded
so its timer trigger and subsequent DMA transfers stay aligned.

Sync acquisition also follows Telekatz's voltage-search approach: each camera
starts at 300 mV and scans 25..800 mV in 25 mV steps when no field is detected.
Each step lasts 40 ms; a locked input retains its reference until field sync
has been missing for 100 ms. Switching back restores that camera's last
reference. Four or more half-line pulses followed by a full line are required
to acquire field sync; isolated noise pulses do not lock the overlay.

After acquisition, ADC1 samples the selected video on PA2 during broad VSYNC
and during the black porch, using TIM2 CH1 (6 us and 3.3 us respectively).
The comparator reference moves gradually towards the midpoint of those levels,
so it does not remain at the noisy edge of the first successful scan threshold.
Samples with missing completion flags or implausible amplitude are ignored.
Only the injected ADC sequence is used for video; regular temperature/VREF
measurements remain available. This follows Telekatz's sync-level measurement
approach and requires no additional wiring.

The loss-of-sync check treats a field timestamp newer than its main-loop clock
snapshot as fresh. It rereads the clock with interrupts masked before starting
a rescan, preventing an interrupt race from unnecessarily blanking the overlay.

For hardware diagnostics, connect USB and run `python tools/video_status.py COM3`
(replace the port; requires `pyserial`). This sends read-only MSP_DEBUG requests.
`edges` counts comparator captures; `armed` counts queued pixel lines;
`completed` counts actual OPAMP DMA completions. No increasing `edges` means
sync is not reaching TIM2; increasing `armed` with no `completed` points to
timer/DMA output. `threshold`, `locked`, and `dma_error` help distinguish them.
Counters wrap at 65536 in the debug response. The full counters are also
available through `video_get_diagnostics()` for a debugger.

Hardware check: display persistent text from the flight controller, switch
PA3 to PA7 and back repeatedly, and confirm the same text returns aligned
on each camera. Also check switching from a disconnected camera back to
a live PAL/NTSC source. Firmware builds alone cannot verify analog timing.

TODO:
Create Custom flightController that supports OpenPixelOSD.


### YouTube Video

[![YouTube](doc/pic/screenshot.png)](https://youtu.be/GXBrZya5-nY)

## License

Open source software — see LICENSE in the repository.

---
