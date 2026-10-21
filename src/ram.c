#include "../include/ram.h"

typedef struct
{
    u8 wram[0x2000];
    u8 hram[0x80];
} RamContext;

static RamContext ctx;

u8 wramRead(u16 address)
{
    address -= 0xC000;

    if (address >= 0x2000)
    {
        printf("INVALID WRAM ADDR %08X\n", address + 0xC000);
        exit(-1);
    }

    return ctx.wram[address];
}

void wramWrite(u16 address, u8 value)
{
    address -= 0xC000;

    ctx.wram[address] = value;
}

u8 hramRead(u16 address)
{
    address -= 0xFF80;

    return ctx.hram[address];
}

void hramWrite(u16 address, u8 value)
{
    address -= 0xFF80;

    ctx.hram[address] = value;
}
