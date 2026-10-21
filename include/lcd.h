#pragma once

#include "common.h"

typedef struct
{
    // registers...
    uint8_t lcdc;
    uint8_t lcds;
    uint8_t scrollY;
    uint8_t scrollX;
    uint8_t ly;
    uint8_t lyCompare;
    uint8_t dma;
    uint8_t bgPalette;
    uint8_t objPalette[2];
    uint8_t winY;
    uint8_t winX;

    // other data...
    uint32_t bgColors[4];
    uint32_t sp1Colors[4];
    uint32_t sp2Colors[4];

} LcdContext;

typedef enum
{
    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_XFER
} LcdMode;

LcdContext *lcdGetContext();

#define LCDC_BGW_ENABLE (BIT(lcdGetContext()->lcdc, 0))
#define LCDC_OBJ_ENABLE (BIT(lcdGetContext()->lcdc, 1))
#define LCDC_OBJ_HEIGHT (BIT(lcdGetContext()->lcdc, 2) ? 16 : 8)
#define LCDC_BG_MAP_AREA (BIT(lcdGetContext()->lcdc, 3) ? 0x9C00 : 0x9800)
#define LCDC_BGW_DATA_AREA (BIT(lcdGetContext()->lcdc, 4) ? 0x8000 : 0x8800)
#define LCDC_WIN_ENABLE (BIT(lcdGetContext()->lcdc, 5))
#define LCDC_WIN_MAP_AREA (BIT(lcdGetContext()->lcdc, 6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_ENABLE (BIT(lcdGetContext()->lcdc, 7))

#define LCDS_MODE ((LcdMode)(lcdGetContext()->lcds & 0b11))
#define LCDS_MODE_SET(mode)             \
    {                                   \
        lcdGetContext()->lcds &= ~0b11; \
        lcdGetContext()->lcds |= mode;  \
    }

#define LCDS_LYC (BIT(lcdGetContext()->lcds, 2))
#define LCDS_LYC_SET(b) (BIT_SET(lcdGetContext()->lcds, 2, b))

typedef enum
{
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM = (1 << 5),
    SS_LYC = (1 << 6),
} StatSrc;

#define LCDS_STAT_INT(src) (lcdGetContext()->lcds & src)

void lcdInit();

uint8_t lcdRead(uint16_t address);
void lcdWrite(uint16_t address, uint8_t value);
