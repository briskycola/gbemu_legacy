#include "../include/io.h"
#include "../include/cpu.h"
#include "../include/dma.h"
#include "../include/gamepad.h"
#include "../include/lcd.h"
#include "../include/timer.h"

static char serialData[2];

uint8_t ioRead(uint16_t address)
{
    if (address == 0xFF00)
    {
        return gamepadGetOutput();
    }

    if (address == 0xFF01)
    {
        return serialData[0];
    }

    if (address == 0xFF02)
    {
        return serialData[1];
    }

    if (BETWEEN(address, 0xFF04, 0xFF07))
    {
        return timerRead(address);
    }

    if (address == 0xFF0F)
    {
        return cpuGetIntFlags();
    }

    if (BETWEEN(address, 0xFF10, 0xFF3F))
    {
        // ignore sound
        return 0;
    }

    if (BETWEEN(address, 0xFF40, 0xFF4B))
    {
        return lcdRead(address);
    }

    if (address == 0xFF7F)
    {
        return 0xFF;
    }

    printf("UNSUPPORTED bus_read(%04X)\n", address);
    return 0;
}

void ioWrite(uint16_t address, uint8_t value)
{
    if (address == 0xFF00)
    {
        gamepadSetSel(value);
        return;
    }

    if (address == 0xFF01)
    {
        serialData[0] = value;
        return;
    }

    if (address == 0xFF02)
    {
        serialData[1] = value;
        return;
    }

    if (BETWEEN(address, 0xFF04, 0xFF07))
    {
        timerWrite(address, value);
        return;
    }

    if (address == 0xFF0F)
    {
        cpuSetIntFlags(value);
        return;
    }

    if (BETWEEN(address, 0xFF10, 0xFF3F))
    {
        // ignore sound
        return;
    }

    if (BETWEEN(address, 0xFF40, 0xFF4B))
    {
        lcdWrite(address, value);
        return;
    }

    if (address == 0xFF7F)
    {
        return;
    }

    printf("UNSUPPORTED bus_write(%04X)\n", address);
}
