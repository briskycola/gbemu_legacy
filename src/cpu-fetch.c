#include "../include/bus.h"
#include "../include/cpu.h"
#include "../include/emulator.h"

extern CpuContext ctx;

void fetchData()
{
    ctx.memDest = 0;
    ctx.destIsMem = false;

    if (ctx.curInst == NULL)
    {
        return;
    }

    switch (ctx.curInst->mode)
    {
    case AM_IMP:
        return;

    case AM_R:
        ctx.fetchedData = cpuReadReg(ctx.curInst->reg1);
        return;

    case AM_R_R:
        ctx.fetchedData = cpuReadReg(ctx.curInst->reg2);
        return;

    case AM_R_D8:
        ctx.fetchedData = busRead(ctx.regs.pc);
        emuCycles(1);
        ctx.regs.pc++;
        return;

    case AM_R_D16:
    case AM_D16:
    {
        uint16_t lo = busRead(ctx.regs.pc);
        emuCycles(1);

        uint16_t hi = busRead(ctx.regs.pc + 1);
        emuCycles(1);

        ctx.fetchedData = lo | (hi << 8);

        ctx.regs.pc += 2;

        return;
    }

    case AM_MR_R:
        ctx.fetchedData = cpuReadReg(ctx.curInst->reg2);
        ctx.memDest = cpuReadReg(ctx.curInst->reg1);
        ctx.destIsMem = true;

        if (ctx.curInst->reg1 == RT_C)
        {
            ctx.memDest |= 0xFF00;
        }

        return;

    case AM_R_MR:
    {
        uint16_t addr = cpuReadReg(ctx.curInst->reg2);

        if (ctx.curInst->reg2 == RT_C)
        {
            addr |= 0xFF00;
        }

        ctx.fetchedData = busRead(addr);
        emuCycles(1);
    }
        return;

    case AM_R_HLI:
        ctx.fetchedData = busRead(cpuReadReg(ctx.curInst->reg2));
        emuCycles(1);
        cpuSetReg(RT_HL, cpuReadReg(RT_HL) + 1);
        return;

    case AM_R_HLD:
        ctx.fetchedData = busRead(cpuReadReg(ctx.curInst->reg2));
        emuCycles(1);
        cpuSetReg(RT_HL, cpuReadReg(RT_HL) - 1);
        return;

    case AM_HLI_R:
        ctx.fetchedData = cpuReadReg(ctx.curInst->reg2);
        ctx.memDest = cpuReadReg(ctx.curInst->reg1);
        ctx.destIsMem = true;
        cpuSetReg(RT_HL, cpuReadReg(RT_HL) + 1);
        return;

    case AM_HLD_R:
        ctx.fetchedData = cpuReadReg(ctx.curInst->reg2);
        ctx.memDest = cpuReadReg(ctx.curInst->reg1);
        ctx.destIsMem = true;
        cpuSetReg(RT_HL, cpuReadReg(RT_HL) - 1);
        return;

    case AM_R_A8:
        ctx.fetchedData = busRead(ctx.regs.pc);
        emuCycles(1);
        ctx.regs.pc++;
        return;

    case AM_A8_R:
        ctx.memDest = busRead(ctx.regs.pc) | 0xFF00;
        ctx.destIsMem = true;
        emuCycles(1);
        ctx.regs.pc++;
        return;

    case AM_HL_SPR:
        ctx.fetchedData = busRead(ctx.regs.pc);
        emuCycles(1);
        ctx.regs.pc++;
        return;

    case AM_D8:
        ctx.fetchedData = busRead(ctx.regs.pc);
        emuCycles(1);
        ctx.regs.pc++;
        return;

    case AM_A16_R:
    case AM_D16_R:
    {
        uint16_t lo = busRead(ctx.regs.pc);
        emuCycles(1);

        uint16_t hi = busRead(ctx.regs.pc + 1);
        emuCycles(1);

        ctx.memDest = lo | (hi << 8);
        ctx.destIsMem = true;

        ctx.regs.pc += 2;
        ctx.fetchedData = cpuReadReg(ctx.curInst->reg2);
    }
        return;

    case AM_MR_D8:
        ctx.fetchedData = busRead(ctx.regs.pc);
        emuCycles(1);
        ctx.regs.pc++;
        ctx.memDest = cpuReadReg(ctx.curInst->reg1);
        ctx.destIsMem = true;
        return;

    case AM_MR:
        ctx.memDest = cpuReadReg(ctx.curInst->reg1);
        ctx.destIsMem = true;
        ctx.fetchedData = busRead(cpuReadReg(ctx.curInst->reg1));
        emuCycles(1);
        return;

    case AM_R_A16:
    {
        uint16_t lo = busRead(ctx.regs.pc);
        emuCycles(1);

        uint16_t hi = busRead(ctx.regs.pc + 1);
        emuCycles(1);

        uint16_t addr = lo | (hi << 8);

        ctx.regs.pc += 2;
        ctx.fetchedData = busRead(addr);
        emuCycles(1);

        return;
    }

    default:
        printf("Unknown Addressing Mode! %d (%02X)\n", ctx.curInst->mode, ctx.curOpcode);
        exit(-7);
        return;
    }
}
