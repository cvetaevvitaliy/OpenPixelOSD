/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "rtc6705.h"
#include "main.h"

#include <rf_pa.h>

/** rtc6705.c  — Bit-banged 3-wire interface for RTC6705
 *  Safety notes (important):
 *  - Writing Synth B (0x01) forces VCO mode even if value is unchanged → avoid redundant writes.
 *  - Pre-driver & PA Control (0x07): writing values other than the device defaults can create
 *    strong spurs. Keep 0x07 at its power-on value unless you explicitly allow it.
 *  - Keep external PA disabled until the device reaches a stable/locked state (STATE 0x0F stable),
 *    especially after reset and any synthesizer programming.
 *
 *      Datasheet notes (RTC6705):
 *      - 25-bit frame, LSB-first: A0..A3 (4b) + RW (1=write,0=read) + D0..D19 (20b).
 *      - FRF = 2 * (N*64 + A) * (Fosc / R). Typical Fosc=8 MHz, R=400 => Fref=20 kHz.
 *      - Registers:
 *      0x00: Synth A (R-counter, 15b). Default R=400.
 *      0x01: Synth B (N[12:0], A[6:0]).
 *      0x02: Synth C (charge pump, test, etc).
 *      0x07: Pre-driver & PA control; PA5G_PW[1:0] controls PA power level.
 *      0x0F: STATE readable register.
 * (Refer to the PDF for details.)
 */

#define SDIO_PORT   SPI2_MOSI_GPIO_Port
#define SDIO_PIN    SPI2_MOSI_Pin
#define SCLK_PORT   SPI2_SCK_GPIO_Port
#define SCLK_PIN    SPI2_SCK_Pin
#define LE_PORT     SPI2_CS_GPIO_Port
#define LE_PIN      SPI2_CS_Pin

#ifndef RTC6705_SDIO_INPUT_PULL
#define RTC6705_SDIO_INPUT_PULL   LL_GPIO_PULL_UP
#endif

// Short delay loop iterations between edges (tune to taste).
#ifndef RTC6705_DELAY_ITERS
#define RTC6705_DELAY_ITERS 8
#endif

// Convenience macros
#define SDIO_OUT()          gpio_to_output(SDIO_PORT, SDIO_PIN)
#define SDIO_IN()           gpio_to_input (SDIO_PORT, SDIO_PIN, RTC6705_SDIO_INPUT_PULL)
#define SDIO_WR(b)          gpio_write(SDIO_PORT, SDIO_PIN, (b))
#define SDIO_RD()           gpio_read(SDIO_PORT, SDIO_PIN)

#define SCLK_OUT()          gpio_to_output(SCLK_PORT, SCLK_PIN)
#define SCLK_WR(b)          gpio_write(SCLK_PORT, SCLK_PIN, (b))

#define LE_OUT()            gpio_to_output(LE_PORT, LE_PIN)
#define LE_WR(b)            gpio_write(LE_PORT, LE_PIN, (b))

// Synth ref source
#ifndef RTC6705_FOSC_HZ
#define RTC6705_FOSC_HZ     8000000u  // 8 MHz crystal per datasheet
#endif
#ifndef RTC6705_R_DIV
#define RTC6705_R_DIV       400u      // R-counter default = 400 (-> 20 kHz ref)
#endif

// Register addresses
#define RTC6705_REG_SYN_A   0x00  // R-counter (15 bits effective)
#define RTC6705_REG_SYN_B   0x01  // N[12:0], A[6:0]
#define RTC6705_REG_SYN_C   0x02  // charge pump, etc
#define RTC6705_REG_VCO3    0x07  // PA control (PA5G_PW[1:0] etc)
#define RTC6705_REG_STATE   0x0F  // readable state

// Bitfield helpers for REG 0x01 (N/A)
#define SYNB_N_MASK         0x1FFFu      // 13 bits
#define SYNB_A_MASK         0x007Fu      // 7 bits (use <=63 for /64 prescaler)
#define SYNB_A_SHIFT        0
#define SYNB_N_SHIFT        7

// Bitfield for REG 0x00 (R 15 bits)
#define SYNA_R_MASK         0x7FFFu

// Bitfield for REG 0x07 (PA5G_PW[1:0] at bits [8:7])
#define REG7_PA5G_PW_SHIFT  7
#define REG7_PA5G_PW_MASK   (0x3u << REG7_PA5G_PW_SHIFT)

#ifndef RTC6705_POST_WRITE_DELAY_US
#define RTC6705_POST_WRITE_DELAY_US   50u
#endif
#ifndef RTC6705_LOCK_WAIT_TIMEOUT_US
#define RTC6705_LOCK_WAIT_TIMEOUT_US 5000u
#endif
#ifndef RTC6705_LOCK_STABLE_US
#define RTC6705_LOCK_STABLE_US       200u
#endif

static uint32_t g_reg7_poweron = 0;     // captured initial 0x07 value
static uint32_t g_reg7_cached   = 0;    // last written/readback value
static uint32_t g_regb_cached   = 0;    // last programmed SYN_B payload
static uint32_t g_freq_mhz_last = 0;    // last applied RF in MHz
static bool g_allow_reg7_w  = false;    // guard flag

static void rtc6705_write_reg(uint8_t addr4, uint32_t data20);
static uint32_t rtc6705_read_reg(uint8_t addr4);
static bool rtc6705_detect(void);

__attribute__((weak)) void rtc6705_hook_delay_us(uint32_t us)
{
    uint32_t wait_loop_index = (us * (SystemCoreClock / (1000000u * 2u)));
    while (wait_loop_index != 0) {
        wait_loop_index--;
    }
}

static inline void bb_delay(void)
{
    // ~tens of ns per a few NOPs depending on CPU clock; tweak RTC6705_DELAY_ITERS for coarse control
    for (volatile uint32_t i = 0; i < RTC6705_DELAY_ITERS; ++i) {
        __NOP();
    }
}

// Switch given pins to General Purpose Output (MODER=01)
static inline void gpio_to_output(GPIO_TypeDef *port, uint32_t pinmask)
{
    for (uint32_t bit = 0; bit < 16; bit++) {
        if (pinmask & (1u << bit)) {
            uint32_t pos = bit * 2u;
            port->MODER = (port->MODER & ~(0x3u << pos)) | (0x1u << pos);
        }
    }
}

// Switch given pins to Input (MODER=00) and set PUPDR
static inline void gpio_to_input(GPIO_TypeDef *port, uint32_t pinmask, uint32_t pull)
{
    for (uint32_t bit = 0; bit < 16; bit++) {
        if (pinmask & (1u << bit)) {
            uint32_t pos = bit * 2u;
            port->MODER &= ~(0x3u << pos);
        }
    }
    for (uint32_t bit = 0; bit < 16; bit++) {
        if (pinmask & (1u << bit)) {
            uint32_t pos = bit * 2u;
            port->PUPDR = (port->PUPDR & ~(0x3u << pos)) | ((pull & 0x3u) << pos);
        }
    }
}

// Atomic write: set or reset
static inline void gpio_write(GPIO_TypeDef *port, uint32_t pinmask, uint32_t level)
{
    if (level) port->BSRR = pinmask; else port->BRR = pinmask;
}
static inline uint32_t gpio_read(GPIO_TypeDef *port, uint32_t pinmask)
{
    return (port->IDR & pinmask) ? 1u : 0u;
}

// Shift one bit LSB-first on rising SCLK (mode 0 semantics)
static inline void shift_out_bit_lsbf(uint32_t bit)
{
    SDIO_WR(bit & 1u);
    bb_delay();
    SCLK_WR(1);
    bb_delay();
    SCLK_WR(0);
}

// Read one bit (chip drives on falling; we sample on rising to meet mode 0)
static inline uint32_t shift_in_bit_lsbf(void)
{
    bb_delay();
    SCLK_WR(1);
    uint32_t b = SDIO_RD();
    bb_delay();
    SCLK_WR(0);
    return b;
}

// LSB-first 25-bit frame builder: A0..A3, RW, D0..D19
static inline uint32_t make_frame(uint8_t addr4, uint8_t rw, uint32_t data20)
{
    uint32_t f = 0;
    f |= ((addr4 & 0x1u) << 0);
    f |= (((addr4 >> 1) & 0x1u) << 1);
    f |= (((addr4 >> 2) & 0x1u) << 2);
    f |= (((addr4 >> 3) & 0x1u) << 3);
    f |= ((rw & 0x1u) << 4);
    f |= ((data20 & 0xFFFFFu) << 5);
    return f;
}


/**
 * @brief Initialize GPIO clocks and idle levels for RTC6705 bit-bang interface.
 * Call this once before any read/write. Make sure SPI2 peripheral is disabled on these pins.
 */
bool rtc6705_init(void)
{
    SDIO_OUT();
    SCLK_OUT();
    LE_OUT();

    // Idle levels: SCLK=0, LE=0, SDIO=0
    SCLK_WR(0);
    LE_WR(0);
    SDIO_WR(0);

    if (rtc6705_detect() == false) {
        return false;
    }
    // Program default R (optional but explicit)
    rtc6705_write_reg(RTC6705_REG_SYN_A, (RTC6705_R_DIV & SYNA_R_MASK));

    // Capture REG 0x07 power-on value and cache it (guard writes later).
    g_reg7_poweron = rtc6705_read_reg(RTC6705_REG_VCO3) & 0xFFFFFu;
    g_reg7_cached = g_reg7_poweron;

    return true;
}

/**
 * @brief Write 20-bit value to 4-bit register address.
 * @param addr4  lower 4 bits used
 * @param data20 lower 20 bits used
 */
static void rtc6705_write_reg(uint8_t addr4, uint32_t data20)
{
    uint32_t frame = make_frame(addr4 & 0xFu, 1u, data20);

    SDIO_OUT();
    LE_WR(0);

    // Shift 25 bits LSB-first
    for (int i = 0; i < 25; i++) {
        shift_out_bit_lsbf(frame >> i);
    }

    // Latch on LE rising edge
    bb_delay();
    LE_WR(1);
    bb_delay();
    LE_WR(0);
}

/**
 * @brief Read 20-bit value from 4-bit register address.
 * @param addr4 lower 4 bits used
 * @return 20-bit data value (LSB-aligned)
 */
static uint32_t rtc6705_read_reg(uint8_t addr4)
{
    uint32_t header = make_frame(addr4 & 0xFu, 0u, 0u);

    SDIO_OUT();
    LE_WR(0);

    // Send only first 5 bits (A0-A3 + RW=0)
    for (int i = 0; i < 5; i++) {
        shift_out_bit_lsbf(header >> i);
    }

    // Now chip drives the bus -> switch SDIO to input
    SDIO_IN();
    bb_delay();

    uint32_t data = 0;
    for (int i = 0; i < 20; i++) {
        data |= (shift_in_bit_lsbf() << i);
    }
    return (data & 0xFFFFFu);
}

static uint32_t rtc6705_get_state_raw(void)
{
    return rtc6705_read_reg(RTC6705_REG_STATE) & 0xFFFFFu;
}

__attribute__((unused))
static uint8_t rtc6705_get_state3(void)
{
    return (uint8_t)(rtc6705_get_state_raw() & 0x7u);
}

/** Wait until STATE register readings are stable for a short window.
 *  This is a generic lock/settle heuristic when exact state codes are unknown.
 *  Returns true if stabilized; false on timeout.
 */
static bool rtc6705_wait_state_stable(uint32_t stable_us, uint32_t timeout_us)
{
    uint32_t last = rtc6705_get_state_raw();
    uint32_t stable_elapsed = 0;
    uint32_t elapsed = 0;

    while (elapsed < timeout_us) {
        rtc6705_hook_delay_us(10);
        elapsed += 10;

        uint32_t cur = rtc6705_get_state_raw();
        if (cur == last) {
            stable_elapsed += 10;
            if (stable_elapsed >= stable_us) {
                return true;
            }
        } else {
            stable_elapsed = 0;
            last = cur;
        }
    }
    return false;
}

// Guard for REG 0x07 writes
void rtc6705_allow_power_writes(bool allow)
{
    g_allow_reg7_w = allow;
}

/**
 * @brief Set internal PA output power level via PA5G_PW[1:0] (REG 0x07).
 * @param level one of rtc6705_power_t values (3, 7, 11, 13 dBm).
 * WARNING: By default, writes to 0x07 are blocked to avoid spurious emissions.
 *  Call rtc6705_allow_power_writes(true) if you really know what you are doing.
 */
void rtc6705_set_power(rtc6705_power_t level)
{
    if (!g_allow_reg7_w) {
        // Guard active: keep power-on defaults to avoid spurs.
        return;
    }

    uint32_t reg = rtc6705_read_reg(RTC6705_REG_VCO3) & 0xFFFFFu;
    uint32_t new_reg = (reg & ~REG7_PA5G_PW_MASK) |
                    ((((uint32_t)level) << REG7_PA5G_PW_SHIFT) & REG7_PA5G_PW_MASK);

    if (new_reg == g_reg7_cached) {
        return; // no write if unchanged
    }

    rtc6705_write_reg(RTC6705_REG_VCO3, new_reg);
    g_reg7_cached = new_reg;
}

/**
 * @brief Current PA5G_PW[1:0] level, decoded from the same g_reg7_cached
 * value rtc6705_set_power() already maintains -- no extra register read.
 */
rtc6705_power_t rtc6705_get_power(void)
{
    return (rtc6705_power_t)((g_reg7_cached & REG7_PA5G_PW_MASK) >> REG7_PA5G_PW_SHIFT);
}

/**
 * @brief Compute and program synthesizer for requested frequency.
 * @param freq_mhz desired RF in MHz (e.g., 5865 for 5.865 GHz).
 * @return actually programmed frequency in MHz (quantized to 40 kHz steps).
 *
 * Formula: FRF = 2 * (N*64 + A) * (Fosc / R).
 * With Fosc=8 MHz, R=400 => step = 40 kHz.
 *
 *  - Wait for STATE to stabilize (lock proxy).
 *  - Re-enable external PA.
 */
uint32_t rtc6705_set_frequency(uint32_t freq_mhz)
{
    const uint32_t fosc = RTC6705_FOSC_HZ;     // 8,000,000
    const uint32_t rdiv = RTC6705_R_DIV;       // 400
    const uint32_t fref_hz = fosc / rdiv;      // 20,000 Hz
    const uint32_t step_hz = 2u * fref_hz;     // 40,000 Hz

    uint64_t target_hz = ((uint64_t)freq_mhz) * 1000000ull;

    // total = (N*64 + A)
    uint64_t total = (target_hz + (step_hz/2ull)) / step_hz; // round
    uint32_t N = (uint32_t)(total / 64ull);
    uint32_t A = (uint32_t)(total % 64ull);
    if (A > 63u) { A = 63u; if (N) N--; }   // clamp A

    uint32_t regB = ((N & SYNB_N_MASK) << SYNB_N_SHIFT) |
                    ((A & SYNB_A_MASK) << SYNB_A_SHIFT);

    // Avoid writing if resulting regB equals last programmed (prevents VCO re-entry)
    if ((g_freq_mhz_last == freq_mhz) && (g_regb_cached == regB)) {
        return freq_mhz; // nothing to do
    }

    // Gate external PA while (re)programming to avoid OOB emissions
#if defined(USE_PA)
    rf_pa_disable();
#endif

    // Ensure R is set (write every time is fine, but it's static; ok to re-write)
    rtc6705_write_reg(RTC6705_REG_SYN_A, (RTC6705_R_DIV & SYNA_R_MASK));

    // Only write SYN_B if changed (critical per datasheet/field notes)
    if (g_regb_cached != regB) {
        rtc6705_write_reg(RTC6705_REG_SYN_B, regB);
        g_regb_cached = regB;
    }

    // Short settle before polling
    rtc6705_hook_delay_us(RTC6705_POST_WRITE_DELAY_US);

    // Wait for state stability (proxy for lock/STBY) before enabling external PA
    rtc6705_wait_state_stable(RTC6705_LOCK_STABLE_US, RTC6705_LOCK_WAIT_TIMEOUT_US);

    // Restore external PA to whatever state was last commanded (may be
    // disabled, e.g. before any MSP_SET_VTX_CONFIG has been received) --
    // NOT an unconditional re-enable.
#if defined(USE_PA)
    rf_pa_restore();
#endif

    g_freq_mhz_last = freq_mhz;

    uint64_t actual_hz = (uint64_t)step_hz * (uint64_t)(N * 64u + A);
    return (uint32_t)(actual_hz / 1000000ull); // return in MHz
}

/**
 * @brief Detect presence of RTC6705 by reading the STATE register (0x0F).
 *
 * Heuristics:
 *  - Read the 20-bit STATE register several times.
 *  - Reject if all reads are stuck at 0x00000 or 0xFFFFF.
 *  - Accept if all reads are identical and non-pathological.
 *  - Otherwise allow variation only in the lower 3 bits (STATE[2:0]),
 *    while upper bits [19:3] must remain stable.
 *
 * @return true if the chip is likely present and responding, false otherwise.
 */
static bool rtc6705_detect(void)
{
    // Take 4 consecutive samples of STATE register (20-bit values)
    uint32_t s0 = rtc6705_get_state_raw();
    uint32_t s1 = rtc6705_get_state_raw();
    uint32_t s2 = rtc6705_get_state_raw();
    uint32_t s3 = rtc6705_get_state_raw();

    bool all_zero = (s0|s1|s2|s3) == 0;
    // all reads are all ones (0xFFFFF) - bus floating or invalid
    bool all_ones = (s0==0xFFFFF) && (s1==0xFFFFF) && (s2==0xFFFFF) && (s3==0xFFFFF);
    if (all_zero || all_ones) {
        return false;
    }

    // Accept if high bits stable across samples; low 3 bits may vary
    uint32_t hi_mask = 0xFFFF8u;
    // Verify that high bits are identical across all samples
    bool hi_stable = ((s0 & hi_mask) == (s1 & hi_mask)) &&
                     ((s1 & hi_mask) == (s2 & hi_mask)) &&
                     ((s2 & hi_mask) == (s3 & hi_mask));
    return hi_stable;
}
