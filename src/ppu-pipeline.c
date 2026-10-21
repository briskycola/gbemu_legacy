#include "../include/bus.h"
#include "../include/lcd.h"
#include "../include/ppu.h"

bool windowVisible()
{
    return LCDC_WIN_ENABLE && lcdGetContext()->winX >= 0 &&
           lcdGetContext()->winX <= 166 && lcdGetContext()->winY >= 0 &&
           lcdGetContext()->winY < YRES;
}

void pixelFifoPush(u32 value)
{
    FifoEntry *next = malloc(sizeof(FifoEntry));
    next->next = NULL;
    next->value = value;

    if (!ppuGetContext()->pfc.pixelFifo.head)
    {
        // first entry...
        ppuGetContext()->pfc.pixelFifo.head = ppuGetContext()->pfc.pixelFifo.tail = next;
    }
    else
    {
        ppuGetContext()->pfc.pixelFifo.tail->next = next;
        ppuGetContext()->pfc.pixelFifo.tail = next;
    }

    ppuGetContext()->pfc.pixelFifo.size++;
}

u32 pixelFifoPop()
{
    if (ppuGetContext()->pfc.pixelFifo.size <= 0)
    {
        fprintf(stderr, "ERR IN PIXEL FIFO!\n");
        exit(-8);
    }

    FifoEntry *popped = ppuGetContext()->pfc.pixelFifo.head;
    ppuGetContext()->pfc.pixelFifo.head = popped->next;
    ppuGetContext()->pfc.pixelFifo.size--;

    u32 val = popped->value;
    free(popped);

    return val;
}

u32 fetchSpritePixels(int bit, u32 color, u8 bgColor)
{
    for (int i = 0; i < ppuGetContext()->fetchedEntryCount; i++)
    {
        int spX = (ppuGetContext()->fetchedEntries[i].x - 8) +
                  ((lcdGetContext()->scrollX % 8));

        if (spX + 8 < ppuGetContext()->pfc.fifoX)
        {
            // past pixel point already...
            continue;
        }

        int offset = ppuGetContext()->pfc.fifoX - spX;

        if (offset < 0 || offset > 7)
        {
            // out of bounds..
            continue;
        }

        bit = (7 - offset);

        if (ppuGetContext()->fetchedEntries[i].fXFlip)
        {
            bit = offset;
        }

        u8 hi = !!(ppuGetContext()->pfc.fetchEntryData[i * 2] & (1 << bit));
        u8 lo = !!(ppuGetContext()->pfc.fetchEntryData[(i * 2) + 1] & (1 << bit)) << 1;

        bool bgPriority = ppuGetContext()->fetchedEntries[i].fBgp;

        if (!(hi | lo))
        {
            // transparent
            continue;
        }

        if (!bgPriority || bgColor == 0)
        {
            color = (ppuGetContext()->fetchedEntries[i].fPn) ? lcdGetContext()->sp2Colors[hi | lo] : lcdGetContext()->sp1Colors[hi | lo];

            if (hi | lo)
            {
                break;
            }
        }
    }

    return color;
}

bool pipelineFifoAdd()
{
    if (ppuGetContext()->pfc.pixelFifo.size > 8)
    {
        // fifo is full!
        return false;
    }

    int x = ppuGetContext()->pfc.fetchX - (8 - (lcdGetContext()->scrollX % 8));

    for (int i = 0; i < 8; i++)
    {
        int bit = 7 - i;
        u8 hi = !!(ppuGetContext()->pfc.bgwFetchData[1] & (1 << bit));
        u8 lo = !!(ppuGetContext()->pfc.bgwFetchData[2] & (1 << bit)) << 1;
        u32 color = lcdGetContext()->bgColors[hi | lo];

        if (!LCDC_BGW_ENABLE)
        {
            color = lcdGetContext()->bgColors[0];
        }

        if (LCDC_OBJ_ENABLE)
        {
            color = fetchSpritePixels(bit, color, hi | lo);
        }

        if (x >= 0)
        {
            pixelFifoPush(color);
            ppuGetContext()->pfc.fifoX++;
        }
    }

    return true;
}

void pipelineLoadSpriteTile()
{
    OamLineEntry *le = ppuGetContext()->lineSprites;

    while (le)
    {
        int spX = (le->entry.x - 8) + (lcdGetContext()->scrollX % 8);

        if ((spX >= ppuGetContext()->pfc.fetchX && spX < ppuGetContext()->pfc.fetchX + 8) ||
            ((spX + 8) >= ppuGetContext()->pfc.fetchX && (spX + 8) < ppuGetContext()->pfc.fetchX + 8))
        {
            // need to add entry
            ppuGetContext()->fetchedEntries[ppuGetContext()->fetchedEntryCount++] = le->entry;
        }

        le = le->next;

        if (!le || ppuGetContext()->fetchedEntryCount >= 3)
        {
            // max checking 3 sprites on pixels
            break;
        }
    }
}

void pipelineLoadSpriteData(u8 offset)
{
    int curY = lcdGetContext()->ly;
    u8 spriteHeight = LCDC_OBJ_HEIGHT;

    for (int i = 0; i < ppuGetContext()->fetchedEntryCount; i++)
    {
        u8 ty = ((curY + 16) - ppuGetContext()->fetchedEntries[i].y) * 2;

        if (ppuGetContext()->fetchedEntries[i].fYFlip)
        {
            // flipped upside down...
            ty = ((spriteHeight * 2) - 2) - ty;
        }

        u8 tileIndex = ppuGetContext()->fetchedEntries[i].tile;

        if (spriteHeight == 16)
        {
            tileIndex &= ~(1); // remove last bit...
        }

        ppuGetContext()->pfc.fetchEntryData[(i * 2) + offset] =
            busRead(0x8000 + (tileIndex * 16) + ty + offset);
    }
}

void pipelineLoadWindowTile()
{
    if (!windowVisible())
    {
        return;
    }

    u8 windowY = lcdGetContext()->winY;

    if (ppuGetContext()->pfc.fetchX + 7 >= lcdGetContext()->winX &&
        ppuGetContext()->pfc.fetchX + 7 < lcdGetContext()->winX + YRES + 14)
    {
        if (lcdGetContext()->ly >= windowY && lcdGetContext()->ly < windowY + XRES)
        {
            u8 wTileY = ppuGetContext()->windowLine / 8;

            ppuGetContext()->pfc.bgwFetchData[0] = busRead(LCDC_WIN_MAP_AREA +
                                                           ((ppuGetContext()->pfc.fetchX + 7 - lcdGetContext()->winX) / 8) +
                                                           (wTileY * 32));

            if (LCDC_BGW_DATA_AREA == 0x8800)
            {
                ppuGetContext()->pfc.bgwFetchData[0] += 128;
            }
        }
    }
}

void pipelineFetch()
{
    switch (ppuGetContext()->pfc.curFetchState)
    {
    case FS_TILE:
    {
        ppuGetContext()->fetchedEntryCount = 0;

        if (LCDC_BGW_ENABLE)
        {
            ppuGetContext()->pfc.bgwFetchData[0] = busRead(LCDC_BG_MAP_AREA +
                                                           (ppuGetContext()->pfc.mapX / 8) +
                                                           (((ppuGetContext()->pfc.mapY / 8)) * 32));

            if (LCDC_BGW_DATA_AREA == 0x8800)
            {
                ppuGetContext()->pfc.bgwFetchData[0] += 128;
            }

            pipelineLoadWindowTile();
        }

        if (LCDC_OBJ_ENABLE && ppuGetContext()->lineSprites)
        {
            pipelineLoadSpriteTile();
        }

        ppuGetContext()->pfc.curFetchState = FS_DATA0;
        ppuGetContext()->pfc.fetchX += 8;
    }
    break;

    case FS_DATA0:
    {
        ppuGetContext()->pfc.bgwFetchData[1] = busRead(LCDC_BGW_DATA_AREA +
                                                       (ppuGetContext()->pfc.bgwFetchData[0] * 16) +
                                                       ppuGetContext()->pfc.tileY);

        pipelineLoadSpriteData(0);

        ppuGetContext()->pfc.curFetchState = FS_DATA1;
    }
    break;

    case FS_DATA1:
    {
        ppuGetContext()->pfc.bgwFetchData[2] = busRead(LCDC_BGW_DATA_AREA +
                                                       (ppuGetContext()->pfc.bgwFetchData[0] * 16) +
                                                       ppuGetContext()->pfc.tileY + 1);

        pipelineLoadSpriteData(1);

        ppuGetContext()->pfc.curFetchState = FS_IDLE;
    }
    break;

    case FS_IDLE:
    {
        ppuGetContext()->pfc.curFetchState = FS_PUSH;
    }
    break;

    case FS_PUSH:
    {
        if (pipelineFifoAdd())
        {
            ppuGetContext()->pfc.curFetchState = FS_TILE;
        }
    }
    break;
    }
}

void pipelinePushPixel()
{
    if (ppuGetContext()->pfc.pixelFifo.size > 8)
    {
        u32 pixelData = pixelFifoPop();

        if (ppuGetContext()->pfc.lineX >= (lcdGetContext()->scrollX % 8))
        {
            ppuGetContext()->videoBuffer[ppuGetContext()->pfc.pushedX +
                                         (lcdGetContext()->ly * XRES)] = pixelData;

            ppuGetContext()->pfc.pushedX++;
        }

        ppuGetContext()->pfc.lineX++;
    }
}

void pipelineProcess()
{
    ppuGetContext()->pfc.mapY = (lcdGetContext()->ly + lcdGetContext()->scrollY);
    ppuGetContext()->pfc.mapX = (ppuGetContext()->pfc.fetchX + lcdGetContext()->scrollX);
    ppuGetContext()->pfc.tileY = ((lcdGetContext()->ly + lcdGetContext()->scrollY) % 8) * 2;

    if (!(ppuGetContext()->lineTicks & 1))
    {
        pipelineFetch();
    }

    pipelinePushPixel();
}

void pipelineFifoReset()
{
    while (ppuGetContext()->pfc.pixelFifo.size)
    {
        pixelFifoPop();
    }

    ppuGetContext()->pfc.pixelFifo.head = 0;
}
