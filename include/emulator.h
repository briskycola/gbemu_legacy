#pragma once

#include "common.h"

typedef struct
{
    bool paused;
    bool running;
    bool die;
    u64 ticks;
} EmuContext;

int emuRun(int argc, char **argv);

EmuContext *emuGetContext();

void emuCycles(int cpuCycles);
