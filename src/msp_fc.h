#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
  bool armed : 1;
} fc_status_t;

typedef struct {
  fc_status_t status;
} fc_t;

extern fc_t fc;

bool msp_fc_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);


