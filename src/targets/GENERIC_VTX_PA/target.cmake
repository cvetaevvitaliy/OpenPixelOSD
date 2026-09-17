# SPDX-License-Identifier: GPL-2.0-only
# GENERIC_VTX_PA build flags -- see CMakeLists.txt "Board target selection".
# RTC6705 VTX + baseline DAC-biased PA (no separate enable GPIO). TARGET_USE_PA implies VTX.
set(TARGET_USE_PA TRUE)
