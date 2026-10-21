#include "../include/emulator.h"
#include "../include/cartridge.h"
#include "../include/cpu.h"
#include "../include/dma.h"
#include "../include/ppu.h"
#include "../include/timer.h"
#include "../include/ui.h"
#include <omp.h>
#include <stdio.h>
#include <unistd.h>

/*
  Emu components:

  |Cart|
  |CPU|
  |Address Bus|
  |PPU|
  |Timer|

*/

static EmuContext ctx;

EmuContext *emuGetContext()
{
    return &ctx;
}

void cpuRun()
{
    timerInit();
    cpuInit();
    ppuInit();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while (ctx.running)
    {
        if (ctx.paused)
        {
            delay(10);
            continue;
        }

        if (!cpuStep())
        {
            printf("CPU Stopped\n");
#pragma omp critical
            {
                ctx.running = false;
            }
            break;
        }
    }
}

void uiRun()
{
    uint32_t prevFrame = 0;
    uiInit();

    while (!ctx.die)
    {
        delay(1);
        uiHandleEvents();

        if (prevFrame != ppuGetContext()->currentFrame)
        {
            uiUpdate();
        }

        prevFrame = ppuGetContext()->currentFrame;
    }

#pragma omp critical
    {
        ctx.running = false;
    }
}

int emuRun(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    if (!cartLoad(argv[1]))
    {
        printf("Failed to load ROM file: %s\n", argv[1]);
        return -2;
    }

    printf("Cart loaded..\n");

#pragma omp parallel sections
    {
#pragma omp section
        {
            cpuRun();
        }

#pragma omp section
        {
            uiRun();
        }
    }

    return 0;
}

void emuCycles(int cpuCycles)
{

    for (int i = 0; i < cpuCycles; i++)
    {
        for (int n = 0; n < 4; n++)
        {
            ctx.ticks++;
            timerTick();
            ppuTick();
        }

        dmaTick();
    }
}
