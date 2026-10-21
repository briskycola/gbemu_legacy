#pragma once

#include "common.h"

uint8_t busRead(uint16_t address);
void busWrite(uint16_t address, uint8_t value);

uint16_t busRead16(uint16_t address);
void busWrite16(uint16_t address, uint16_t value);
