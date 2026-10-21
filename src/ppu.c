#include "../include/ppu.h"
#include "../include/lcd.h"
#include "../include/ppu-sm.h"
#include <string.h>

void pipelineFifoReset();
void pipelineProcess();

static PpuContext ctx;

PpuContext *ppuGetContext()
{
    return &ctx;
}

void ppuInit()
{
    ctx.currentFrame = 0;
    ctx.lineTicks = 0;
    ctx.videoBuffer = malloc(YRES * XRES * sizeof(32));

    ctx.pfc.lineX = 0;
    ctx.pfc.pushedX = 0;
    ctx.pfc.fetchX = 0;
    ctx.pfc.pixelFifo.size = 0;
    ctx.pfc.pixelFifo.head = ctx.pfc.pixelFifo.tail = NULL;
    ctx.pfc.curFetchState = FS_TILE;

    ctx.lineSprites = 0;
    ctx.fetchedEntryCount = 0;
    ctx.windowLine = 0;

    lcdInit();
    LCDS_MODE_SET(MODE_OAM);

    memset(ctx.oamRam, 0, sizeof(ctx.oamRam));
    memset(ctx.videoBuffer, 0, YRES * XRES * sizeof(u32));
}

void ppuTick()
{
    ctx.lineTicks++;

    switch (LCDS_MODE)
    {
    case MODE_OAM:
        ppuModeOam();
        break;
    case MODE_XFER:
        ppuModeXfer();
        break;
    case MODE_VBLANK:
        ppuModeVblank();
        break;
    case MODE_HBLANK:
        ppuModeHblank();
        break;
    }
}

void ppuOamWrite(u16 address, u8 value)
{
    if (address >= 0xFE00)
    {
        address -= 0xFE00;
    }

    u8 *p = (u8 *)ctx.oamRam;
    p[address] = value;
}

u8 ppuOamRead(u16 address)
{
    if (address >= 0xFE00)
    {
        address -= 0xFE00;
    }

    u8 *p = (u8 *)ctx.oamRam;
    return p[address];
}

void ppuVramWrite(u16 address, u8 value)
{
    ctx.vram[address - 0x8000] = value;
}

u8 ppuVramRead(u16 address)
{
    return ctx.vram[address - 0x8000];
}
