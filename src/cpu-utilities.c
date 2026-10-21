#include "../include/bus.h"
#include "../include/cpu.h"

extern CpuContext ctx;

u16 reverse(u16 n)
{
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

u16 cpuReadReg(RegType rt)
{
    switch (rt)
    {
    case RT_A:
        return ctx.regs.a;
    case RT_F:
        return ctx.regs.f;
    case RT_B:
        return ctx.regs.b;
    case RT_C:
        return ctx.regs.c;
    case RT_D:
        return ctx.regs.d;
    case RT_E:
        return ctx.regs.e;
    case RT_H:
        return ctx.regs.h;
    case RT_L:
        return ctx.regs.l;

    case RT_AF:
        return reverse(*((u16 *)&ctx.regs.a));
    case RT_BC:
        return reverse(*((u16 *)&ctx.regs.b));
    case RT_DE:
        return reverse(*((u16 *)&ctx.regs.d));
    case RT_HL:
        return reverse(*((u16 *)&ctx.regs.h));

    case RT_PC:
        return ctx.regs.pc;
    case RT_SP:
        return ctx.regs.sp;
    default:
        return 0;
    }
}

void cpuSetReg(RegType rt, u16 val)
{
    switch (rt)
    {
    case RT_A:
        ctx.regs.a = val & 0xFF;
        break;
    case RT_F:
        ctx.regs.f = val & 0xFF;
        break;
    case RT_B:
        ctx.regs.b = val & 0xFF;
        break;
    case RT_C:
    {
        ctx.regs.c = val & 0xFF;
    }
    break;
    case RT_D:
        ctx.regs.d = val & 0xFF;
        break;
    case RT_E:
        ctx.regs.e = val & 0xFF;
        break;
    case RT_H:
        ctx.regs.h = val & 0xFF;
        break;
    case RT_L:
        ctx.regs.l = val & 0xFF;
        break;

    case RT_AF:
        *((u16 *)&ctx.regs.a) = reverse(val);
        break;
    case RT_BC:
        *((u16 *)&ctx.regs.b) = reverse(val);
        break;
    case RT_DE:
        *((u16 *)&ctx.regs.d) = reverse(val);
        break;
    case RT_HL:
    {
        *((u16 *)&ctx.regs.h) = reverse(val);
        break;
    }

    case RT_PC:
        ctx.regs.pc = val;
        break;
    case RT_SP:
        ctx.regs.sp = val;
        break;
    case RT_NONE:
        break;
    }
}

u8 cpuReadReg8(RegType rt)
{
    switch (rt)
    {
    case RT_A:
        return ctx.regs.a;
    case RT_F:
        return ctx.regs.f;
    case RT_B:
        return ctx.regs.b;
    case RT_C:
        return ctx.regs.c;
    case RT_D:
        return ctx.regs.d;
    case RT_E:
        return ctx.regs.e;
    case RT_H:
        return ctx.regs.h;
    case RT_L:
        return ctx.regs.l;
    case RT_HL:
    {
        return busRead(cpuReadReg(RT_HL));
    }
    default:
        printf("**ERR INVALID REG8: %d\n", rt);
        NO_IMPL
    }
}

void cpuSetReg8(RegType rt, u8 val)
{
    switch (rt)
    {
    case RT_A:
        ctx.regs.a = val & 0xFF;
        break;
    case RT_F:
        ctx.regs.f = val & 0xFF;
        break;
    case RT_B:
        ctx.regs.b = val & 0xFF;
        break;
    case RT_C:
        ctx.regs.c = val & 0xFF;
        break;
    case RT_D:
        ctx.regs.d = val & 0xFF;
        break;
    case RT_E:
        ctx.regs.e = val & 0xFF;
        break;
    case RT_H:
        ctx.regs.h = val & 0xFF;
        break;
    case RT_L:
        ctx.regs.l = val & 0xFF;
        break;
    case RT_HL:
        busWrite(cpuReadReg(RT_HL), val);
        break;
    default:
        printf("**ERR INVALID REG8: %d\n", rt);
        NO_IMPL
    }
}

CpuRegisters *cpuGetRegs()
{
    return &ctx.regs;
}

u8 cpuGetIntFlags()
{
    return ctx.intFlags;
}

void cpuSetIntFlags(u8 value)
{
    ctx.intFlags = value;
}
