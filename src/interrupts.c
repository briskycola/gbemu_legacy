#include "../include/interrupts.h"
#include "../include/cpu.h"
#include "../include/stack.h"

void intHandle(CpuContext *ctx, u16 address)
{
    stackPush16(ctx->regs.pc);
    ctx->regs.pc = address;
}

bool intCheck(CpuContext *ctx, u16 address, InterruptType it)
{
    if (ctx->intFlags & it && ctx->ieRegister & it)
    {
        intHandle(ctx, address);
        ctx->intFlags &= ~it;
        ctx->halted = false;
        ctx->intMasterEnabled = false;

        return true;
    }

    return false;
}

void cpuHandleInterrupts(CpuContext *ctx)
{
    if (intCheck(ctx, 0x40, IT_VBLANK))
    {
    }
    else if (intCheck(ctx, 0x48, IT_LCD_STAT))
    {
    }
    else if (intCheck(ctx, 0x50, IT_TIMER))
    {
    }
    else if (intCheck(ctx, 0x58, IT_SERIAL))
    {
    }
    else if (intCheck(ctx, 0x60, IT_JOYPAD))
    {
    }
}
