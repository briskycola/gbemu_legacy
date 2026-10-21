#pragma once

#include "common.h"

void stackPush(u8 data);
void stackPush16(u16 data);

u8 stackPop();
u16 stackPop16();
