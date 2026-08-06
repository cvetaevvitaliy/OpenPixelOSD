/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/target.h — selects the active board's pin/resource definitions.
 *
 * Set via CMake: -DTARGET_BOARD_VTX_PA_RTC76401 (see CMakeLists.txt's
 * TARGET_BOARD cache variable). Defaults to the baseline board if no
 * target define is set, so existing builds are unaffected.
 */
#ifndef TARGETS_TARGET_H
#define TARGETS_TARGET_H

#if defined(TARGET_BOARD_VTX_PA_RTC76401)
#include "targets/generic_vtx_pa_rtc76401.h"
#else
#include "generic.h"
#endif

#endif //TARGETS_TARGET_H
