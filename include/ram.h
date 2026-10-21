#pragma once

#include "common.h"

uint8_t wramRead(uint16_t address);
void wramWrite(uint16_t address, uint8_t value);

uint8_t hramRead(uint16_t address);
void hramWrite(uint16_t address, uint8_t value);
