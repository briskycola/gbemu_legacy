#include "../include/ram.h"

typedef struct
{
    uint8_t wram[0x2000];
    uint8_t hram[0x80];
} RamContext;

static RamContext ctx;

uint8_t wramRead(uint16_t address)
{
    address -= 0xC000;

    if (address >= 0x2000)
    {
        printf("INVALID WRAM ADDR %08X\n", address + 0xC000);
        exit(-1);
    }

    return ctx.wram[address];
}

void wramWrite(uint16_t address, uint8_t value)
{
    address -= 0xC000;

    ctx.wram[address] = value;
}

uint8_t hramRead(uint16_t address)
{
    address -= 0xFF80;

    return ctx.hram[address];
}

void hramWrite(uint16_t address, uint8_t value)
{
    address -= 0xFF80;

    ctx.hram[address] = value;
}
