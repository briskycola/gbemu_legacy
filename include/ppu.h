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
    u32 value; // 32 bit color value.
} FifoEntry;

typedef struct
{
    FifoEntry *head;
    FifoEntry *tail;
    u32 size;
} fifo;

typedef struct
{
    FetchState curFetchState;
    fifo pixelFifo;
    u8 lineX;
    u8 pushedX;
    u8 fetchX;
    u8 bgwFetchData[3];
    u8 fetchEntryData[6]; // oam data..
    u8 mapY;
    u8 mapX;
    u8 tileY;
    u8 fifoX;
} PixelFifoContext;

typedef struct
{
    u8 y;
    u8 x;
    u8 tile;

    u8 fCgbPn : 3;
    u8 fCgbVramBank : 1;
    u8 fPn : 1;
    u8 fXFlip : 1;
    u8 fYFlip : 1;
    u8 fBgp : 1;

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
    u8 vram[0x2000];

    PixelFifoContext pfc;

    u8 lineSpriteCount;              // 0 to 10 sprites.
    OamLineEntry *lineSprites;       // linked list of current sprites on line.
    OamLineEntry lineEntryArray[10]; // memory to use for list.

    u8 fetchedEntryCount;
    OamEntry fetchedEntries[3]; // entries fetched during pipeline.
    u8 windowLine;

    u32 currentFrame;
    u32 lineTicks;
    u32 *videoBuffer;
} PpuContext;

void ppuInit();
void ppuTick();

void ppuOamWrite(u16 address, u8 value);
u8 ppuOamRead(u16 address);

void ppuVramWrite(u16 address, u8 value);
u8 ppuVramRead(u16 address);

PpuContext *ppuGetContext();
