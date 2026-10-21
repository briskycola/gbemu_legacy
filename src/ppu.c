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
    memset(ctx.videoBuffer, 0, YRES * XRES * sizeof(uint32_t));
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

void ppuOamWrite(uint16_t address, uint8_t value)
{
    if (address >= 0xFE00)
    {
        address -= 0xFE00;
    }

    uint8_t *p = (uint8_t *)ctx.oamRam;
    p[address] = value;
}

uint8_t ppuOamRead(uint16_t address)
{
    if (address >= 0xFE00)
    {
        address -= 0xFE00;
    }

    uint8_t *p = (uint8_t *)ctx.oamRam;
    return p[address];
}

void ppuVramWrite(uint16_t address, uint8_t value)
{
    ctx.vram[address - 0x8000] = value;
}

uint8_t ppuVramRead(uint16_t address)
{
    return ctx.vram[address - 0x8000];
}
