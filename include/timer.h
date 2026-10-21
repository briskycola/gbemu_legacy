#pragma once

#include "common.h"

typedef struct
{
    u16 div;
    u8 tima;
    u8 tma;
    u8 tac;
} TimerContext;

void timerInit();
void timerTick();

void timerWrite(u16 address, u8 value);
u8 timerRead(u16 address);

TimerContext *timerGetContext();
