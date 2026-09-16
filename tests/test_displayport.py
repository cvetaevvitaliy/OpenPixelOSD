"""Exercise the actual DisplayPort callback and canvas functions with host GCC."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from test_video_switch import ROOT, function


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cc", default="gcc")
    args = parser.parse_args()
    dp = (ROOT / "src/render/msp_displayport.c").read_text(encoding="utf-8")
    canvas = (ROOT / "src/render/canvas_char.c").read_text(encoding="utf-8")
    # Use the production enum, callback and renderer API, mocking transport only.
    enum = dp[dp.index("typedef enum"):dp.index("} msp_displayport_cmd_t;") + len("} msp_displayport_cmd_t;")]
    functions = "\n".join(function(canvas, name) for name in
                          ("canvas_char_flush_map", "canvas_char_clean",
                           "canvas_char_write", "canvas_char_draw_complete"))
    functions += "\n" + function(dp, "msp_callback")
    fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "render/video_overlay.h"
#define EXEC_RAM
#define COLUMN_SIZE 30
#define ROW_SIZE 16
#define MSP_DISPLAYPORT 182
#define MSP_SET_OSD_CANVAS 188
#define MSP_OSD_CHAR_WRITE 87
#define MSP_DEBUG 254
#define MSP_OUTBOUND 0
#define MSP_OWNER_UART 0
#define MSP_OWNER_USB 1
typedef enum { MSP_V1, MSP_V2_OVER_V1, MSP_V2_NATIVE } msp_version_t;
char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
uint8_t active_buffer;
bool canvas_write_next_buff, show_logo = true;
static unsigned replies;
static bool debug_reply;
static void video_get_diagnostics_stub(video_diagnostics_t *status) {
    memset(status, 0, sizeof(*status));
    status->threshold_mv = 525;
    status->input = 1;
    status->locked = 1;
    status->captures = 42;
    status->armed_lines = 12;
    status->completed_lines = 11;
}
#define video_get_diagnostics video_get_diagnostics_stub
static uint16_t construct_msp_command_v1(uint8_t *tx, int cmd, uint8_t *data, int size, int dir) {
    if (cmd == MSP_DEBUG) {
        assert(size == 8 && data[0] == 13 && data[1] == 194);
        assert(data[2] == 42 && data[4] == 12 && data[6] == 11);
        debug_reply = true;
        return 0;
    }
    assert(cmd == MSP_SET_OSD_CANVAS && size == 2);
    assert(data[0] == COLUMN_SIZE && data[1] == ROW_SIZE);
    return 0;
}
static void uart1_tx_dma(uint8_t *data, uint16_t size) { replies++; }
static void usb_uart_write_bytes(const char *data, uint16_t size) { replies++; }
static void update_font_symbol_write(uint8_t index, const uint8_t *data, int len) {}
'''
    checks = r'''
static void send(uint8_t *data, uint16_t len) {
    msp_callback(MSP_OWNER_UART, MSP_V1, MSP_DISPLAYPORT, len, data);
}
int main(void) {
    canvas_char_flush_map();
    canvas_char_write(0, 0, "BOOT", 4);
    canvas_char_draw_complete();
    send(NULL, 0); /* empty packet must not read payload[0] */
    uint8_t keepalive[] = {0};
    send(keepalive, sizeof keepalive);
    assert(show_logo && replies == 1); /* connection alone is not a frame */
    uint8_t clear[] = {2}, draw[] = {4};
    uint8_t text[] = {3, 4, 5, 0, 'O', 'S', 'D'};
    send(clear, sizeof clear);
    send(text, sizeof text);
    assert(show_logo);
    send(draw, sizeof draw);
    assert(!show_logo);
    assert(memcmp(&canvas_char_map[!active_buffer][4][5], "OSD", 3) == 0);
    /* FC layouts wider than this canvas must not overwrite the next row. */
    send(clear, sizeof clear);
    uint8_t long_text[] = {3, 0, 29, 0, 'A', 'B', 'C'};
    send(long_text, sizeof long_text);
    send(draw, sizeof draw);
    assert(canvas_char_map[!active_buffer][0][29] == 'A');
    assert(canvas_char_map[!active_buffer][1][0] == ' ');
    uint8_t release[] = {1};
    send(release, sizeof release);
    assert(show_logo);
    send(clear, sizeof clear);
    send(text, sizeof text);
    send(draw, sizeof draw);
    assert(!show_logo);
    msp_callback(MSP_OWNER_USB, MSP_V1, MSP_DEBUG, 0, NULL);
    assert(debug_reply && !show_logo);
    return 0;
}
'''
    with tempfile.TemporaryDirectory() as temp:
        c_file = Path(temp) / "displayport_test.c"
        exe = Path(temp) / "displayport_test.exe"
        c_file.write_text(fixture + enum + functions + checks, encoding="utf-8")
        subprocess.run([args.cc, "-std=c11", "-O2", "-I", str(ROOT / "src"), str(c_file), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("PASS: logo/frame handoff, canvas negotiation, text rendering, clipping, empty packets, reconnect")


if __name__ == "__main__":
    main()
