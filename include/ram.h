#pragma once

#include "common.h"

u8 wramRead(u16 address);
void wramWrite(u16 address, u8 value);

u8 hramRead(u16 address);
void hramWrite(u16 address, u8 value);
