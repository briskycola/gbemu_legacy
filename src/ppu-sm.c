#include "../include/cartridge.h"
#include "../include/cpu.h"
#include "../include/interrupts.h"
#include "../include/lcd.h"
#include "../include/ppu.h"
#include <string.h>

void pipelineFifoReset();
void pipelineProcess();
bool windowVisible();

void incrementLy()
{
    if (windowVisible() && lcdGetContext()->ly >= lcdGetContext()->winY &&
        lcdGetContext()->ly < lcdGetContext()->winY + YRES)
    {
        ppuGetContext()->windowLine++;
    }

    lcdGetContext()->ly++;

    if (lcdGetContext()->ly == lcdGetContext()->lyCompare)
    {
        LCDS_LYC_SET(1);

        if (LCDS_STAT_INT(SS_LYC))
        {
            cpuRequestInterrupt(IT_LCD_STAT);
        }
    }
    else
    {
        LCDS_LYC_SET(0);
    }
}

void loadLineSprites()
{
    int curY = lcdGetContext()->ly;

    u8 spriteHeight = LCDC_OBJ_HEIGHT;
    memset(ppuGetContext()->lineEntryArray, 0,
           sizeof(ppuGetContext()->lineEntryArray));

    for (int i = 0; i < 40; i++)
    {
        OamEntry e = ppuGetContext()->oamRam[i];

        if (!e.x)
        {
            // x = 0 means not visible...
            continue;
        }

        if (ppuGetContext()->lineSpriteCount >= 10)
        {
            // max 10 sprites per line...
            break;
        }

        if (e.y <= curY + 16 && e.y + spriteHeight > curY + 16)
        {
            // this sprite is on the current line.

            OamLineEntry *entry = &ppuGetContext()->lineEntryArray[ppuGetContext()->lineSpriteCount++];

            entry->entry = e;
            entry->next = NULL;

            if (!ppuGetContext()->lineSprites ||
                ppuGetContext()->lineSprites->entry.x > e.x)
            {
                entry->next = ppuGetContext()->lineSprites;
                ppuGetContext()->lineSprites = entry;
                continue;
            }

            // do some sorting...

            OamLineEntry *le = ppuGetContext()->lineSprites;
            OamLineEntry *prev = le;

            while (le)
            {
                if (le->entry.x > e.x)
                {
                    prev->next = entry;
                    entry->next = le;
                    break;
                }

                if (!le->next)
                {
                    le->next = entry;
                    break;
                }

                prev = le;
                le = le->next;
            }
        }
    }
}

void ppuModeOam()
{
    if (ppuGetContext()->lineTicks >= 80)
    {
        LCDS_MODE_SET(MODE_XFER);

        ppuGetContext()->pfc.curFetchState = FS_TILE;
        ppuGetContext()->pfc.lineX = 0;
        ppuGetContext()->pfc.fetchX = 0;
        ppuGetContext()->pfc.pushedX = 0;
        ppuGetContext()->pfc.fifoX = 0;
    }

    if (ppuGetContext()->lineTicks == 1)
    {
        // read oam on the first tick only...
        ppuGetContext()->lineSprites = 0;
        ppuGetContext()->lineSpriteCount = 0;

        loadLineSprites();
    }
}

void ppuModeXfer()
{
    pipelineProcess();

    if (ppuGetContext()->pfc.pushedX >= XRES)
    {
        pipelineFifoReset();

        LCDS_MODE_SET(MODE_HBLANK);

        if (LCDS_STAT_INT(SS_HBLANK))
        {
            cpuRequestInterrupt(IT_LCD_STAT);
        }
    }
}

void ppuModeVblank()
{
    if (ppuGetContext()->lineTicks >= TICKS_PER_LINE)
    {
        incrementLy();

        if (lcdGetContext()->ly >= LINES_PER_FRAME)
        {
            LCDS_MODE_SET(MODE_OAM);
            lcdGetContext()->ly = 0;
            ppuGetContext()->windowLine = 0;
        }

        ppuGetContext()->lineTicks = 0;
    }
}

static u32 targetFrameTime = 1000 / 60;
static long prevFrameTime = 0;
static long startTimer = 0;
static long frameCount = 0;

void ppuModeHblank()
{
    if (ppuGetContext()->lineTicks >= TICKS_PER_LINE)
    {
        incrementLy();

        if (lcdGetContext()->ly >= YRES)
        {
            LCDS_MODE_SET(MODE_VBLANK);

            cpuRequestInterrupt(IT_VBLANK);

            if (LCDS_STAT_INT(SS_VBLANK))
            {
                cpuRequestInterrupt(IT_LCD_STAT);
            }

            ppuGetContext()->currentFrame++;

            // calc FPS...
            u32 end = getTicks();
            u32 frameTime = end - prevFrameTime;

            if (frameTime < targetFrameTime)
            {
                delay((targetFrameTime - frameTime));
            }

            if (end - startTimer >= 1000)
            {
                u32 fps = frameCount;
                startTimer = end;
                frameCount = 0;

                printf("FPS: %d\n", fps);

                if (cartNeedSave())
                {
                    cartBatterySave();
                }
            }

            frameCount++;
            prevFrameTime = getTicks();
        }
        else
        {
            LCDS_MODE_SET(MODE_OAM);
        }

        ppuGetContext()->lineTicks = 0;
    }
}
