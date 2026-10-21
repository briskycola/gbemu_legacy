#pragma once

#include "common.h"

typedef struct
{
    bool start;
    bool select;
    bool a;
    bool b;
    bool up;
    bool down;
    bool left;
    bool right;
} GamepadState;

void gamepadInit();
bool gamepadButtonSel();
bool gamepadDirSel();
void gamepadSetSel(uint8_t value);

GamepadState *gamepadGetState();
uint8_t gamepadGetOutput();
