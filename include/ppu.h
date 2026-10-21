#pragma once

#include "common.h"

static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE = 456;
static const int YRES = 144;
static const int XRES = 160;

typedef enum
{
    FS_TILE,
    FS_DATA0,
    FS_DATA1,
    FS_IDLE,
    FS_PUSH
} FetchState;

typedef struct _fifo_entry
{
    struct _fifo_entry *next;
    uint32_t value; // 32 bit color value.
} FifoEntry;

typedef struct
{
    FifoEntry *head;
    FifoEntry *tail;
    uint32_t size;
} fifo;

typedef struct
{
    FetchState curFetchState;
    fifo pixelFifo;
    uint8_t lineX;
    uint8_t pushedX;
    uint8_t fetchX;
    uint8_t bgwFetchData[3];
    uint8_t fetchEntryData[6]; // oam data..
    uint8_t mapY;
    uint8_t mapX;
    uint8_t tileY;
    uint8_t fifoX;
} PixelFifoContext;

typedef struct
{
    uint8_t y;
    uint8_t x;
    uint8_t tile;

    uint8_t fCgbPn : 3;
    uint8_t fCgbVramBank : 1;
    uint8_t fPn : 1;
    uint8_t fXFlip : 1;
    uint8_t fYFlip : 1;
    uint8_t fBgp : 1;

} OamEntry;

/*
 Bit7   BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
 Bit6   Y flip          (0=Normal, 1=Vertically mirrored)
 Bit5   X flip          (0=Normal, 1=Horizontally mirrored)
 Bit4   Palette number  **Non CGB Mode Only** (0=OBP0, 1=OBP1)
 Bit3   Tile VRAM-Bank  **CGB Mode Only**     (0=Bank 0, 1=Bank 1)
 Bit2-0 Palette number  **CGB Mode Only**     (OBP0-7)
 */

typedef struct _oam_line_entry
{
    OamEntry entry;
    struct _oam_line_entry *next;
} OamLineEntry;

typedef struct
{
    OamEntry oamRam[40];
    uint8_t vram[0x2000];

    PixelFifoContext pfc;

    uint8_t lineSpriteCount;              // 0 to 10 sprites.
    OamLineEntry *lineSprites;       // linked list of current sprites on line.
    OamLineEntry lineEntryArray[10]; // memory to use for list.

    uint8_t fetchedEntryCount;
    OamEntry fetchedEntries[3]; // entries fetched during pipeline.
    uint8_t windowLine;

    uint32_t currentFrame;
    uint32_t lineTicks;
    uint32_t *videoBuffer;
} PpuContext;

void ppuInit();
void ppuTick();

void ppuOamWrite(uint16_t address, uint8_t value);
uint8_t ppuOamRead(uint16_t address);

void ppuVramWrite(uint16_t address, uint8_t value);
uint8_t ppuVramRead(uint16_t address);

PpuContext *ppuGetContext();
