/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#include <stddef.h>
#define GBR_VERSION 1
#define GBR_MACRO 1
#define GBR_DANCE 2
#define GBR_RGB 3
#define GBR_SLOTS 8
#define GBR_STEPS 128
#define GBR_MACRO_SIZE (292 + GBR_STEPS * 8)
#define GBR_DANCE_SIZE 68
#define GBR_RGB_SIZE 5
#define GBR_MAX_SIZE GBR_MACRO_SIZE
uint16_t gbr_u16(const uint8_t *p);
uint32_t gbr_u32(const uint8_t *p);
void gbr_w16(uint8_t *p, uint16_t v);
void gbr_w32(uint8_t *p, uint32_t v);
size_t gbr_size(uint8_t kind);
int gbr_validate(uint8_t kind, const uint8_t *data, size_t len);
int gbr_hid_valid(uint32_t code);
