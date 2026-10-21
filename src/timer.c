#include "../include/timer.h"
#include "../include/interrupts.h"

static TimerContext ctx = {0};

TimerContext *timerGetContext()
{
    return &ctx;
}

void timerInit()
{
    ctx.div = 0xAC00;
}

void timerTick()
{
    uint16_t prevDiv = ctx.div;

    ctx.div++;

    bool timerUpdate = false;

    switch (ctx.tac & (0b11))
    {
    case 0b00:
        timerUpdate = (prevDiv & (1 << 9)) && (!(ctx.div & (1 << 9)));
        break;
    case 0b01:
        timerUpdate = (prevDiv & (1 << 3)) && (!(ctx.div & (1 << 3)));
        break;
    case 0b10:
        timerUpdate = (prevDiv & (1 << 5)) && (!(ctx.div & (1 << 5)));
        break;
    case 0b11:
        timerUpdate = (prevDiv & (1 << 7)) && (!(ctx.div & (1 << 7)));
        break;
    }

    if (timerUpdate && ctx.tac & (1 << 2))
    {
        ctx.tima++;

        if (ctx.tima == 0xFF)
        {
            ctx.tima = ctx.tma;

            cpuRequestInterrupt(IT_TIMER);
        }
    }
}

void timerWrite(uint16_t address, uint8_t value)
{
    switch (address)
    {
    case 0xFF04:
        // DIV
        ctx.div = 0;
        break;

    case 0xFF05:
        // TIMA
        ctx.tima = value;
        break;

    case 0xFF06:
        // TMA
        ctx.tma = value;
        break;

    case 0xFF07:
        // TAC
        ctx.tac = value;
        break;
    }
}

uint8_t timerRead(uint16_t address)
{
    switch (address)
    {
    case 0xFF04:
        return ctx.div >> 8;
    case 0xFF05:
        return ctx.tima;
    case 0xFF06:
        return ctx.tma;
    case 0xFF07:
        return ctx.tac;
    }
    return 0;
}
