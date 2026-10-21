#include "../include/lcd.h"
#include "../include/dma.h"
#include "../include/ppu.h"

static LcdContext ctx;

static unsigned long colorsDefault[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void lcdInit()
{
    ctx.lcdc = 0x91;
    ctx.scrollX = 0;
    ctx.scrollY = 0;
    ctx.ly = 0;
    ctx.lyCompare = 0;
    ctx.bgPalette = 0xFC;
    ctx.objPalette[0] = 0xFF;
    ctx.objPalette[1] = 0xFF;
    ctx.winY = 0;
    ctx.winX = 0;

    for (int i = 0; i < 4; i++)
    {
        ctx.bgColors[i] = colorsDefault[i];
        ctx.sp1Colors[i] = colorsDefault[i];
        ctx.sp2Colors[i] = colorsDefault[i];
    }
}

LcdContext *lcdGetContext()
{
    return &ctx;
}

uint8_t lcdRead(uint16_t address)
{
    uint8_t offset = (address - 0xFF40);
    uint8_t *p = (uint8_t *)&ctx;

    return p[offset];
}

void updatePalette(uint8_t paletteData, uint8_t pal)
{
    uint32_t *pColors = ctx.bgColors;

    switch (pal)
    {
    case 1:
        pColors = ctx.sp1Colors;
        break;
    case 2:
        pColors = ctx.sp2Colors;
        break;
    }

    pColors[0] = colorsDefault[paletteData & 0b11];
    pColors[1] = colorsDefault[(paletteData >> 2) & 0b11];
    pColors[2] = colorsDefault[(paletteData >> 4) & 0b11];
    pColors[3] = colorsDefault[(paletteData >> 6) & 0b11];
}

void lcdWrite(uint16_t address, uint8_t value)
{

    uint8_t offset = (address - 0xFF40);
    uint8_t *p = (uint8_t *)&ctx;
    p[offset] = value;

    if (offset == 6)
    {
        // 0xFF46 = DMA
        dmaStart(value);
    }

    if (address == 0xFF47)
    {
        updatePalette(value, 0);
    }
    else if (address == 0xFF48)
    {
        updatePalette(value & 0b11111100, 1);
    }
    else if (address == 0xFF49)
    {
        updatePalette(value & 0b11111100, 2);
    }
}
