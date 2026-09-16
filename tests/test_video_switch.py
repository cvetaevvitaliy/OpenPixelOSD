"""Run the actual switch/sync C functions with mocked peripherals using host GCC.

Usage: python tests/test_video_switch.py --cc gcc
This checks state transitions, not analog video timing.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    match = re.search(r"^[^\n]*\b" + name + r"\([^;\n]*\)\n\{", source, re.M)
    if not match:
        raise ValueError(name)
    start = match.start()
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = (ROOT / "src/render/video_overlay.c").read_text(encoding="utf-8")
    functions = "\n".join(function(source, name) for name in
                          ("update_sync_reference", "init_buffers", "stop_pixel_output", "set_video_input",
                           "get_video_input", "pars_video_signal"))
    functions += "\n" + function(source, "video_sync_loop")
    config = (ROOT / "src/main.h").read_text(encoding="utf-8")
    timing = "\n".join(line for line in config.splitlines()
                       if line.startswith(("#define VIDEO_SYNC_", "#define VIDEO_BLACK_SAMPLE_TICKS ", "#define VIDEO_PIXEL_TICKS ",
                                           "#define VIDEO_LINE_START_TICKS ",
                                           "#define VIDEO_LINE_END_TICKS ")))
    harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "render/video_overlay.h"
#define EXEC_RAM
#define CCMRAM_BSS
#define LINE_BUF_SZ 413
#define DAC_GRAY 806
#define MAX_RENDER_LINE 305
#define TIM2_TICK_MS (1e6f / 170000000)
#define TIM1 1
#define TIM2 2
#define TIM15 15
#define LL_TIM_SLAVEMODE_DISABLED 0
#define LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER 1
#define DMA1 1
#define COMP2 2
#define TIM2_IRQn 2
#define LL_DMA_CHANNEL_1 1
#define LL_DMA_CHANNEL_2 2
#define LL_TIM_CHANNEL_CH2 2
#define DAC3 3
#define LL_DAC_CHANNEL_2 2
#define DAC12BIT_FROM_MV(mv) ((mv) * 4095U / 3300U)
#define DAC12BIT_TO_MV(raw) ((raw) * 3300U / 4095U)
#define ADC1 1
#define LL_ADC_INJ_RANK_1 1
static uint16_t sync_low_raw, adc_sample;
static bool sync_low_valid, sync_sample_pending, adc_ready;
static unsigned sample_compare;
static void LL_ADC_ClearFlag_JEOC(int a) { adc_ready = false; }
static bool LL_ADC_IsActiveFlag_JEOC(int a) { return adc_ready; }
static uint16_t LL_ADC_INJ_ReadConversionData12(int a, int rank) { return adc_sample; }
static void LL_TIM_OC_SetCompareCH1(int t, unsigned value) { sample_compare = value; }
static video_diagnostics_t video_diagnostics;
static uint8_t half_lines;
static uint16_t sync_threshold_mv[2] = { VIDEO_SYNC_START_MV, VIDEO_SYNC_START_MV };
static uint32_t last_scan_ms, now_ms;
static uint32_t HAL_GetTick(void) { return now_ms; }
static void LL_DAC_ConvertData12RightAligned(int d, int c, unsigned value) {}
static unsigned LL_DMA_IsActiveFlag_TC1(int d) { return 0; }
static unsigned LL_DMA_IsActiveFlag_TE1(int d) { return 0; }
static unsigned LL_DMA_IsActiveFlag_TE2(int d) { return 0; }
static struct { uint32_t CSR; } opamp;
#define OPAMP1 (&opamp)
static struct { uint32_t comp_input, opamp_input; } video_inputs[] = {
    {2, 0x108000E5}, {1, 0x108000E9}
};
static uint16_t dac_buff[2][LINE_BUF_SZ], video_line;
static uint32_t opamp_buff[2][LINE_BUF_SZ], video_source;
static video_input_t active_video_input;
static bool buf_idx, field_synced, discard_capture, new_field;
static unsigned mask, capture, running, dma[3], requests, comp, rendered;
static unsigned slave[16];
static unsigned __get_PRIMASK(void) { return mask; }
static void __disable_irq(void) { mask = 1; }
static void __set_PRIMASK(unsigned value) { mask = value; }
static unsigned LL_TIM_CC_IsEnabledChannel(int t, int c) { return capture; }
static void LL_TIM_CC_DisableChannel(int t, int c) { capture = 0; }
static void LL_TIM_CC_EnableChannel(int t, int c) { capture = 1; }
static void LL_TIM_DisableDMAReq_UPDATE(int t) { requests &= ~1u; }
static void LL_TIM_DisableDMAReq_CC2(int t) { requests &= ~2u; }
static void LL_TIM_DisableCounter(int t) { running = 0; }
static void LL_TIM_SetSlaveMode(int t, int mode) { slave[t] = mode; }
static void LL_DMA_DisableChannel(int d, int c) { dma[c] = 0; }
static void LL_DMA_ClearFlag_GI1(int d) {}
static void LL_DMA_ClearFlag_GI2(int d) {}
static void LL_COMP_SetInputPlus(int c, unsigned input) {
    assert(mask && !capture && !running && !dma[1] && !dma[2] && !requests);
    assert(!slave[TIM1] && !slave[TIM15]);
    comp = input;
}
static void LL_TIM_SetCounter(int t, unsigned count) {}
static void LL_TIM_ClearFlag_CC2(int t) {}
static void LL_TIM_ClearFlag_CC2OVR(int t) {}
static void LL_TIM_ClearFlag_CC3(int t) {}
static void NVIC_ClearPendingIRQ(int irq) {}
static void push_line_to_dma(uint16_t line) { rendered++; }
'''
    checks = r'''
int main(void) {
    assert(360 * VIDEO_PIXEL_TICKS <= 8500); /* 50 us maximum pixel window */
    assert(VIDEO_LINE_START_TICKS + 361 * VIDEO_PIXEL_TICKS < VIDEO_LINE_END_TICKS);
    assert(VIDEO_LINE_END_TICKS < 9996); /* next NTSC sync starts at ~58.8 us */
    for (unsigned iteration = 0; iteration < 100; iteration++) {
        video_input_t input = iteration & 1;
        mask = iteration & 1;
        capture = (iteration >> 1) & 1;
        unsigned old_capture = capture, old_mask = mask;
        running = dma[1] = dma[2] = 1; requests = 3;
        video_line = 200; new_field = field_synced = true;
        set_video_input(input);
        assert(get_video_input() == input);
        assert(comp == video_inputs[input].comp_input);
        assert(opamp.CSR == video_inputs[input].opamp_input);
        assert(mask == old_mask && capture == old_capture);
        assert(slave[TIM1] && slave[TIM15]);
        assert(!video_line && !new_field && !field_synced && discard_capture);
        for (unsigned b = 0; b < 2; b++)
            for (unsigned i = 0; i < LINE_BUF_SZ; i++)
                assert(opamp_buff[b][i] == opamp.CSR);
        unsigned before = rendered;
        pars_video_signal(5440); /* first capture is partial; ignore it */
        pars_video_signal(10880); /* PAL full line before field sync */
        assert(rendered == before && !field_synced);
        for (unsigned i = 0; i < 4; i++) pars_video_signal(5440);
        pars_video_signal(9690); /* 57 us broad-sync transition */
        pars_video_signal(1190); /* 7 us broad-sync transition */
        pars_video_signal(10880);
        assert(field_synced && new_field && rendered == before + 1);
        for (unsigned i = 0; i < 4; i++) pars_video_signal(5403);
        pars_video_signal(10807);
        assert(video_line == 1 && rendered == before + 2);
        for (unsigned i = 0; i < 70000; i++) pars_video_signal(10880);
        assert(video_line == MAX_RENDER_LINE + 1); /* no counter wrap */
        before = rendered;
        pars_video_signal(100); /* noise must not render */
        assert(rendered == before);
        set_video_input((video_input_t)99);
        assert(get_video_input() == input && video_line == MAX_RENDER_LINE + 1);
    }
    /* Missing comparator edges: scan through the whole supported voltage range. */
    set_video_input(VIDEO_INPUT_1);
    for (unsigned i = 0; i < 40; i++) {
        uint16_t previous = sync_threshold_mv[0];
        now_ms += VIDEO_SYNC_SCAN_MS;
        video_sync_loop();
        assert(sync_threshold_mv[0] == (previous >= VIDEO_SYNC_MAX_MV ? VIDEO_SYNC_MIN_MV : previous + VIDEO_SYNC_STEP_MV));
    }
    /* Acquire at a non-default level and hold it while fields keep arriving. */
    uint16_t acquired = sync_threshold_mv[0];
    for (unsigned frame = 0; frame < 20; frame++) {
        now_ms += 20;
        pars_video_signal(5440); /* also consumes the first partial capture */
        for (unsigned i = 0; i < 4; i++) pars_video_signal(5440);
        pars_video_signal(10880);
        video_sync_loop();
        assert(video_diagnostics.locked && sync_threshold_mv[0] == acquired);
    }
    now_ms += VIDEO_SYNC_LOST_MS;
    video_sync_loop();
    assert(!video_diagnostics.locked && sync_threshold_mv[0] != acquired);
    uint16_t camera1 = sync_threshold_mv[0];
    set_video_input(VIDEO_INPUT_2);
    now_ms += VIDEO_SYNC_SCAN_MS;
    video_sync_loop();
    assert(sync_threshold_mv[0] == camera1); /* independent camera references */
    /* A capture published just after the main loop read its clock is fresh. */
    video_diagnostics.locked = true;
    now_ms += 100;
    video_diagnostics.last_field_ms = now_ms + 1;
    uint16_t before_race = sync_threshold_mv[1];
    video_sync_loop();
    assert(video_diagnostics.locked && sync_threshold_mv[1] == before_race);

    /* Repeated PAL/NTSC VSYNC samples move an edge lock towards mid amplitude. */
    set_video_input(VIDEO_INPUT_1);
    sync_threshold_mv[0] = 300;
    discard_capture = false;
    for (unsigned frame = 0; frame < 24; frame++) {
        for (unsigned i = 0; i < 4; i++) pars_video_signal(5440);
        pars_video_signal(9690);
        assert(sample_compare == VIDEO_SYNC_SAMPLE_TICKS);
        adc_sample = DAC12BIT_FROM_MV(300); adc_ready = true;
        pars_video_signal(5440);
        assert(sync_low_valid && sample_compare == VIDEO_BLACK_SAMPLE_TICKS);
        while (half_lines < (frame & 1 ? 9 : 8)) pars_video_signal(5440);
        pars_video_signal(1190);
        adc_sample = DAC12BIT_FROM_MV(600); adc_ready = true;
        while (half_lines < 11) pars_video_signal(5440);
        pars_video_signal(10880);
    }
    assert(sync_threshold_mv[0] >= 445 && sync_threshold_mv[0] <= 450);
    assert(sync_threshold_mv[1] == before_race);
    uint16_t centred = sync_threshold_mv[0];
    update_sync_reference(1000, 999); /* reversed */
    update_sync_reference(100, 110); /* noise, no sync amplitude */
    update_sync_reference(0, 4000); /* implausible amplitude */
    assert(sync_threshold_mv[0] == centred);
    set_video_input(VIDEO_INPUT_2);
    assert(!sync_low_valid && !sync_sample_pending && !adc_ready);
    return 0;
}
'''
    with tempfile.TemporaryDirectory() as temp:
        c_file = Path(temp) / "switch_test.c"
        exe = Path(temp) / "switch_test.exe"
        c_file.write_text(timing + "\n" + harness + functions + checks, encoding="utf-8")
        subprocess.run([args.cc, "-std=c11", "-O2", "-I", str(ROOT / "src"), str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("PASS: 100 switches, DMA quiescence, both buffers, IRQ preservation, PAL/NTSC sync, invalid input")


if __name__ == "__main__":
    main()
