/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/target.h — selects the active board's pin/resource definitions.
 *
 * Set via CMake's TARGET_BOARD cache variable (see CMakeLists.txt):
 *   GENERIC                 -> targets/generic.h (no PA of any kind)
 *   GENERIC_VTX_PA           -> targets/generic_vtx_pa.h (baseline PA)
 *   GENERIC_VTX_PA_RTC76401  -> targets/generic_vtx_pa_rtc76401.h (RTC76401 external PA)
 * Defaults to the baseline (no-PA) board if no target define is set.
 */
#ifndef TARGETS_TARGET_H
#define TARGETS_TARGET_H

#if defined(TARGET_BOARD_VTX_PA_RTC76401)
#include "targets/generic_vtx_pa_rtc76401.h"
#elif defined(TARGET_BOARD_TSCT_VTX_SKYWORKS_SE5004L_V1)
#include "targets/tsct_vtx_skyworks_se5004l_v1.h"
#elif defined(TARGET_BOARD_TSCT_SURFBOARD_V1)
#include "targets/tsct_surfboard_v1.h"
#elif defined(TARGET_BOARD_VTX_PA)
#include "targets/generic_vtx_pa.h"
#else
#include "targets/generic.h"
#endif

#endif //TARGETS_TARGET_H
