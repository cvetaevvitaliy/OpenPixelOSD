"""Compare actual logo/glyph rendering against a pixel reference on host GCC."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from test_video_switch import ROOT, function


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    source = (ROOT / "src/render/video_overlay.c").read_text(encoding="utf-8")
    functions = "\n".join(function(source, name) for name in
                          ("init_logo_cache", "render_overlay_logo_line", "squash_canvas_raw_pixel_buff"))
    fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "logo/logo.h"
#include "fonts/font_bf_default.h"
#define EXEC_RAM
#define PEXELS_PER_LINE 360
#define LINE_BUF_SZ 361
#define MAX_RENDER_LINE 305
#define LOGO_OFFSET_Y 50
#define DAC_BLACK 558
#define DAC_WHITE 1116
#define DAC_GRAY 806
#define OPAMP_CONST_DAC 0x108000edU
enum { PX_BLACK, PX_TRANSPARENT, PX_WHITE, PX_GRAY };
static uint16_t dac_buff[2][LINE_BUF_SZ + 1];
static uint32_t opamp_buff[2][LINE_BUF_SZ + 1];
static uint8_t logo_cache[LOGO_HEIGHT][LOGO_ROW_BYTES];
static uint16_t logo_first_pixel[LOGO_HEIGHT], logo_end_pixel[LOGO_HEIGHT];
static const uint16_t logo_levels[] = { DAC_BLACK, DAC_BLACK, DAC_WHITE, DAC_GRAY };
static bool buf_idx;
static uint32_t video_source = 0x108000e5U;
'''
    checks = r'''
int main(void) {
    init_logo_cache();
    for (unsigned buffer = 0; buffer < 2; buffer++) {
        buf_idx = buffer;
        for (unsigned line = 0; line < 320; line++) {
            for (unsigned b = 0; b < 2; b++)
                for (unsigned x = 0; x <= LINE_BUF_SZ; x++) {
                    dac_buff[b][x] = 100 + x;
                    opamp_buff[b][x] = 2000 + x;
                }
            render_overlay_logo_line(line);
            for (unsigned b = 0; b < 2; b++)
                for (unsigned x = 0; x <= LINE_BUF_SZ; x++) {
                    unsigned dac = 100 + x, opamp = 2000 + x;
                    if (b == buffer && line <= MAX_RENDER_LINE && line >= LOGO_OFFSET_Y &&
                        line - LOGO_OFFSET_Y < LOGO_HEIGHT && x >= LOGO_OFFSET_X &&
                        x - LOGO_OFFSET_X < LOGO_WIDTH && x < PEXELS_PER_LINE) {
                        unsigned col = x - LOGO_OFFSET_X;
                        unsigned raw = logo_data[(line - LOGO_OFFSET_Y) * LOGO_ROW_BYTES + col / 4];
                        unsigned pixel = (raw >> (6 - (col % 4) * 2)) & 3;
                        if (pixel != PX_TRANSPARENT) { dac = logo_levels[pixel]; opamp = OPAMP_CONST_DAC; }
                    }
                    assert(dac_buff[b][x] == dac);
                    assert(opamp_buff[b][x] == opamp);
                }
        }
    }
    buf_idx = 0;
    for (unsigned glyph = 0; glyph < 256; glyph++) {
        for (unsigned row = 0; row < FONT_HEIGHT; row++) {
            for (unsigned x = 0; x <= LINE_BUF_SZ; x++) {
                dac_buff[0][x] = DAC_BLACK;
                opamp_buff[0][x] = video_source;
            }
            squash_canvas_raw_pixel_buff((char)glyph, row, 17);
            for (unsigned x = 0; x < FONT_WIDTH; x++) {
                unsigned raw = font_data[glyph * FONT_STRIDE + row * BYTES_PER_ROW + x / 4];
                unsigned pixel = (raw >> (6 - 2 * (x % 4))) & 3;
                assert(dac_buff[0][17 + x] == logo_levels[pixel]);
                assert(opamp_buff[0][17 + x] == (pixel == PX_TRANSPARENT ? video_source : OPAMP_CONST_DAC));
            }
        }
    }
    return 0;
}
'''
    with tempfile.TemporaryDirectory() as temp:
        c_file = Path(temp) / "logo_test.c"
        exe = Path(temp) / "logo_test.exe"
        for offset in (100, 350, 360):
            c_file.write_text(f"#define LOGO_OFFSET_X {offset}\n" + fixture + functions + checks, encoding="utf-8")
            subprocess.run([args.cc, "-std=c11", "-O2", "-I", str(ROOT / "src"), str(c_file), "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
    print("PASS: every logo row, both buffers, transparency, clipping, DMA terminator, all 256 glyphs")


if __name__ == "__main__":
    main()
