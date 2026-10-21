
#pragma once

#include "cpu.h"

typedef enum
{
    IT_VBLANK = 1,
    IT_LCD_STAT = 2,
    IT_TIMER = 4,
    IT_SERIAL = 8,
    IT_JOYPAD = 16
} InterruptType;

void cpuRequestInterrupt(InterruptType t);

void cpuHandleInterrupts(CpuContext *ctx);
