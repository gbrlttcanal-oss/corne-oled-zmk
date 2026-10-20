/* SPDX-License-Identifier: MIT */
#pragma once
#include <gbrtzt/format.h>
#include <zmk/behavior.h>
int gbr_get(uint8_t kind, uint8_t slot, uint8_t *out, uint32_t *revision);
int gbr_put(uint8_t kind, uint8_t slot, uint32_t expected, const uint8_t *data, size_t len, uint32_t *revision);
int gbr_macro_start(uint8_t slot, struct zmk_behavior_binding_event event);
int gbr_dance_event(uint8_t slot, struct zmk_behavior_binding_event event, bool pressed);
int gbr_reset(void);
bool gbr_rgb_supported(void);
