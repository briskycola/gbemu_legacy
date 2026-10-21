#pragma once

#include "common.h"

u8 busRead(u16 address);
void busWrite(u16 address, u8 value);

u16 busRead16(u16 address);
void busWrite16(u16 address, u16 value);
