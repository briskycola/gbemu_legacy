#pragma once

#include "common.h"

void stackPush(uint8_t data);
void stackPush16(uint16_t data);

uint8_t stackPop();
uint16_t stackPop16();
