/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_GEN_H
#define VIDEO_GEN_H

extern volatile bool video_gen_enabled;

void video_gen_start(void);
void video_gen_stop(void);

#endif //VIDEO_GEN_H
