#include "../include/bus.h"
#include "../include/cartridge.h"
#include "../include/cpu.h"
#include "../include/emulator.h"
#include "../include/stack.h"

// processes CPU instructions...

void cpuSetFlags(CpuContext *ctx, int8_t z, int8_t n, int8_t h, int8_t c)
{
    if (z != -1)
    {
        BIT_SET(ctx->regs.f, 7, z);
    }

    if (n != -1)
    {
        BIT_SET(ctx->regs.f, 6, n);
    }

    if (h != -1)
    {
        BIT_SET(ctx->regs.f, 5, h);
    }

    if (c != -1)
    {
        BIT_SET(ctx->regs.f, 4, c);
    }
}

static void procNone(CpuContext *ctx)
{
    fprintf(stderr, "INVALID INSTRUCTION! opcode=%02X pc=%04X\n",
            ctx->curOpcode, ctx->regs.pc);
    fprintf(stderr, "AF=%02X%02X BC=%02X%02X DE=%02X%02X HL=%02X%02X SP=%04X IF=%02X IE=%02X IME=%d HALT_BUG=%d\n",
            ctx->regs.a, ctx->regs.f, ctx->regs.b, ctx->regs.c,
            ctx->regs.d, ctx->regs.e, ctx->regs.h, ctx->regs.l,
            ctx->regs.sp, ctx->intFlags, ctx->ieRegister,
            ctx->intMasterEnabled, ctx->haltBug);
    cpuDumpTrace();
    cartDumpBankTrace();
    exit(-7);
}

static void procNop(CpuContext *ctx)
{
}

RegType rtLookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A};

RegType decodeReg(u8 reg)
{
    if (reg > 0b111)
    {
        return RT_NONE;
    }

    return rtLookup[reg];
}

static void procCb(CpuContext *ctx)
{
    u8 op = ctx->fetchedData;
    RegType reg = decodeReg(op & 0b111);
    u8 bit = (op >> 3) & 0b111;
    u8 bitOp = (op >> 6) & 0b11;
    u8 regVal = cpuReadReg8(reg);

    emuCycles(1);

    if (reg == RT_HL)
    {
        emuCycles(2);
    }

    switch (bitOp)
    {
    case 1:
        // BIT
        cpuSetFlags(ctx, !(regVal & (1 << bit)), 0, 1, -1);
        return;

    case 2:
        // RST
        regVal &= ~(1 << bit);
        cpuSetReg8(reg, regVal);
        return;

    case 3:
        // SET
        regVal |= (1 << bit);
        cpuSetReg8(reg, regVal);
        return;
    }

    bool flagC = CPU_FLAG_C;

    switch (bit)
    {
    case 0:
    {
        // RLC
        bool setC = false;
        u8 result = (regVal << 1) & 0xFF;

        if ((regVal & (1 << 7)) != 0)
        {
            result |= 1;
            setC = true;
        }

        cpuSetReg8(reg, result);
        cpuSetFlags(ctx, result == 0, false, false, setC);
    }
        return;

    case 1:
    {
        // RRC
        u8 old = regVal;
        regVal >>= 1;
        regVal |= (old << 7);

        cpuSetReg8(reg, regVal);
        cpuSetFlags(ctx, !regVal, false, false, old & 1);
    }
        return;

    case 2:
    {
        // RL
        u8 old = regVal;
        regVal <<= 1;
        regVal |= flagC;

        cpuSetReg8(reg, regVal);
        cpuSetFlags(ctx, !regVal, false, false, !!(old & 0x80));
    }
        return;

    case 3:
    {
        // RR
        u8 old = regVal;
        regVal >>= 1;

        regVal |= (flagC << 7);

        cpuSetReg8(reg, regVal);
        cpuSetFlags(ctx, !regVal, false, false, old & 1);
    }
        return;

    case 4:
    {
        // SLA
        u8 old = regVal;
        regVal <<= 1;

        cpuSetReg8(reg, regVal);
        cpuSetFlags(ctx, !regVal, false, false, !!(old & 0x80));
    }
        return;

    case 5:
    {
        // SRA
        u8 u = (int8_t)regVal >> 1;
        cpuSetReg8(reg, u);
        cpuSetFlags(ctx, !u, 0, 0, regVal & 1);
    }
        return;

    case 6:
    {
        // SWAP
        regVal = ((regVal & 0xF0) >> 4) | ((regVal & 0xF) << 4);
        cpuSetReg8(reg, regVal);
        cpuSetFlags(ctx, regVal == 0, false, false, false);
    }
        return;

    case 7:
    {
        // SRL
        u8 u = regVal >> 1;
        cpuSetReg8(reg, u);
        cpuSetFlags(ctx, !u, 0, 0, regVal & 1);
    }
        return;
    }

    fprintf(stderr, "ERROR: INVALID CB: %02X", op);
    NO_IMPL
}

static void procRlca(CpuContext *ctx)
{
    u8 u = ctx->regs.a;
    bool c = (u >> 7) & 1;
    u = (u << 1) | c;
    ctx->regs.a = u;

    cpuSetFlags(ctx, 0, 0, 0, c);
}

static void procRrca(CpuContext *ctx)
{
    u8 b = ctx->regs.a & 1;
    ctx->regs.a >>= 1;
    ctx->regs.a |= (b << 7);

    cpuSetFlags(ctx, 0, 0, 0, b);
}

static void procRla(CpuContext *ctx)
{
    u8 u = ctx->regs.a;
    u8 cf = CPU_FLAG_C;
    u8 c = (u >> 7) & 1;

    ctx->regs.a = (u << 1) | cf;
    cpuSetFlags(ctx, 0, 0, 0, c);
}

static void procStop(CpuContext *ctx)
{
    fprintf(stderr, "STOPPING!\n");
}

static void procDaa(CpuContext *ctx)
{
    u8 u = 0;
    int fc = 0;

    if (CPU_FLAG_H || (!CPU_FLAG_N && (ctx->regs.a & 0xF) > 9))
    {
        u = 6;
    }

    if (CPU_FLAG_C || (!CPU_FLAG_N && ctx->regs.a > 0x99))
    {
        u |= 0x60;
        fc = 1;
    }

    ctx->regs.a += CPU_FLAG_N ? -u : u;

    cpuSetFlags(ctx, ctx->regs.a == 0, -1, 0, fc);
}

static void procCpl(CpuContext *ctx)
{
    ctx->regs.a = ~ctx->regs.a;
    cpuSetFlags(ctx, -1, 1, 1, -1);
}

static void procScf(CpuContext *ctx)
{
    cpuSetFlags(ctx, -1, 0, 0, 1);
}

static void procCcf(CpuContext *ctx)
{
    cpuSetFlags(ctx, -1, 0, 0, CPU_FLAG_C ^ 1);
}

static void procHalt(CpuContext *ctx)
{
    if (!ctx->intMasterEnabled && (ctx->intFlags & ctx->ieRegister))
    {
        ctx->haltBug = true;
        cpuNoteHaltBug(ctx->regs.pc);
    }
    else
    {
        ctx->halted = true;
    }
}

static void procRra(CpuContext *ctx)
{
    u8 carry = CPU_FLAG_C;
    u8 newC = ctx->regs.a & 1;

    ctx->regs.a >>= 1;
    ctx->regs.a |= (carry << 7);

    cpuSetFlags(ctx, 0, 0, 0, newC);
}

static void procAnd(CpuContext *ctx)
{
    ctx->regs.a &= ctx->fetchedData;
    cpuSetFlags(ctx, ctx->regs.a == 0, 0, 1, 0);
}

static void procXor(CpuContext *ctx)
{
    ctx->regs.a ^= ctx->fetchedData & 0xFF;
    cpuSetFlags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void procOr(CpuContext *ctx)
{
    ctx->regs.a |= ctx->fetchedData & 0xFF;
    cpuSetFlags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void procCp(CpuContext *ctx)
{
    int n = (int)ctx->regs.a - (int)ctx->fetchedData;

    cpuSetFlags(ctx, n == 0, 1,
                ((int)ctx->regs.a & 0x0F) - ((int)ctx->fetchedData & 0x0F) < 0, n < 0);
}

static void procDi(CpuContext *ctx)
{
    ctx->intMasterEnabled = false;
}

static void procEi(CpuContext *ctx)
{
    ctx->enablingIme = true;
}

static bool is16Bit(RegType rt)
{
    return rt >= RT_AF;
}

static void procLd(CpuContext *ctx)
{
    if (ctx->destIsMem)
    {
        // LD (BC), A for instance...

        if (is16Bit(ctx->curInst->reg2))
        {
            // if 16 bit register...
            emuCycles(1);
            busWrite16(ctx->memDest, ctx->fetchedData);
        }
        else
        {
            busWrite(ctx->memDest, ctx->fetchedData);
        }

        emuCycles(1);

        return;
    }

    if (ctx->curInst->mode == AM_HL_SPR)
    {
        u8 hflag = (cpuReadReg(ctx->curInst->reg2) & 0xF) +
                       (ctx->fetchedData & 0xF) >=
                   0x10;

        u8 cflag = (cpuReadReg(ctx->curInst->reg2) & 0xFF) +
                       (ctx->fetchedData & 0xFF) >=
                   0x100;

        cpuSetFlags(ctx, 0, 0, hflag, cflag);
        cpuSetReg(ctx->curInst->reg1,
                  cpuReadReg(ctx->curInst->reg2) + (int8_t)ctx->fetchedData);

        return;
    }

    cpuSetReg(ctx->curInst->reg1, ctx->fetchedData);
}

static void procLdh(CpuContext *ctx)
{
    if (ctx->curInst->reg1 == RT_A)
    {
        cpuSetReg(ctx->curInst->reg1, busRead(0xFF00 | ctx->fetchedData));
    }
    else
    {
        busWrite(ctx->memDest, ctx->regs.a);
    }

    emuCycles(1);
}

static bool checkCond(CpuContext *ctx)
{
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch (ctx->curInst->cond)
    {
    case CT_NONE:
        return true;
    case CT_C:
        return c;
    case CT_NC:
        return !c;
    case CT_Z:
        return z;
    case CT_NZ:
        return !z;
    }

    return false;
}

static void gotoAddr(CpuContext *ctx, u16 addr, bool pushpc)
{
    if (checkCond(ctx))
    {
        if (pushpc)
        {
            emuCycles(2);
            stackPush16(ctx->regs.pc);
        }

        ctx->regs.pc = addr;
        emuCycles(1);
    }
}

static void procJp(CpuContext *ctx)
{
    gotoAddr(ctx, ctx->fetchedData, false);
}

static void procJr(CpuContext *ctx)
{
    int8_t rel = (int8_t)(ctx->fetchedData & 0xFF);
    u16 addr = ctx->regs.pc + rel;
    gotoAddr(ctx, addr, false);
}

static void procCall(CpuContext *ctx)
{
    gotoAddr(ctx, ctx->fetchedData, true);
}

static void procRst(CpuContext *ctx)
{
    gotoAddr(ctx, ctx->curInst->param, true);
}

static void procRet(CpuContext *ctx)
{
    if (ctx->curInst->cond != CT_NONE)
    {
        emuCycles(1);
    }

    if (checkCond(ctx))
    {
        u16 lo = stackPop();
        emuCycles(1);
        u16 hi = stackPop();
        emuCycles(1);

        u16 n = (hi << 8) | lo;
        ctx->regs.pc = n;

        emuCycles(1);
    }
}

static void procReti(CpuContext *ctx)
{
    ctx->intMasterEnabled = true;
    procRet(ctx);
}

static void procPop(CpuContext *ctx)
{
    u16 lo = stackPop();
    emuCycles(1);
    u16 hi = stackPop();
    emuCycles(1);

    u16 n = (hi << 8) | lo;

    cpuSetReg(ctx->curInst->reg1, n);

    if (ctx->curInst->reg1 == RT_AF)
    {
        cpuSetReg(ctx->curInst->reg1, n & 0xFFF0);
    }
}

static void procPush(CpuContext *ctx)
{
    u16 hi = (cpuReadReg(ctx->curInst->reg1) >> 8) & 0xFF;
    emuCycles(1);
    stackPush(hi);

    u16 lo = cpuReadReg(ctx->curInst->reg1) & 0xFF;
    emuCycles(1);
    stackPush(lo);

    emuCycles(1);
}

static void procInc(CpuContext *ctx)
{
    u16 val = cpuReadReg(ctx->curInst->reg1) + 1;

    if (is16Bit(ctx->curInst->reg1))
    {
        emuCycles(1);
    }

    if (ctx->curInst->reg1 == RT_HL && ctx->curInst->mode == AM_MR)
    {
        val = busRead(cpuReadReg(RT_HL)) + 1;
        val &= 0xFF;
        busWrite(cpuReadReg(RT_HL), val);
    }
    else
    {
        cpuSetReg(ctx->curInst->reg1, val);
        val = cpuReadReg(ctx->curInst->reg1);
    }

    if ((ctx->curOpcode & 0x03) == 0x03)
    {
        return;
    }

    cpuSetFlags(ctx, val == 0, 0, (val & 0x0F) == 0, -1);
}

static void procDec(CpuContext *ctx)
{
    u16 val = cpuReadReg(ctx->curInst->reg1) - 1;

    if (is16Bit(ctx->curInst->reg1))
    {
        emuCycles(1);
    }

    if (ctx->curInst->reg1 == RT_HL && ctx->curInst->mode == AM_MR)
    {
        val = busRead(cpuReadReg(RT_HL)) - 1;
        busWrite(cpuReadReg(RT_HL), val);
    }
    else
    {
        cpuSetReg(ctx->curInst->reg1, val);
        val = cpuReadReg(ctx->curInst->reg1);
    }

    if ((ctx->curOpcode & 0x0B) == 0x0B)
    {
        return;
    }

    cpuSetFlags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
}

static void procSub(CpuContext *ctx)
{
    u16 val = cpuReadReg(ctx->curInst->reg1) - ctx->fetchedData;

    int z = val == 0;
    int h = ((int)cpuReadReg(ctx->curInst->reg1) & 0xF) - ((int)ctx->fetchedData & 0xF) < 0;
    int c = ((int)cpuReadReg(ctx->curInst->reg1)) - ((int)ctx->fetchedData) < 0;

    cpuSetReg(ctx->curInst->reg1, val);
    cpuSetFlags(ctx, z, 1, h, c);
}

static void procSbc(CpuContext *ctx)
{
    u8 val = ctx->fetchedData + CPU_FLAG_C;

    int z = cpuReadReg(ctx->curInst->reg1) - val == 0;

    int h = ((int)cpuReadReg(ctx->curInst->reg1) & 0xF) - ((int)ctx->fetchedData & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)cpuReadReg(ctx->curInst->reg1)) - ((int)ctx->fetchedData) - ((int)CPU_FLAG_C) < 0;

    cpuSetReg(ctx->curInst->reg1, cpuReadReg(ctx->curInst->reg1) - val);
    cpuSetFlags(ctx, z, 1, h, c);
}

static void procAdc(CpuContext *ctx)
{
    u16 u = ctx->fetchedData;
    u16 a = ctx->regs.a;
    u16 c = CPU_FLAG_C;

    ctx->regs.a = (a + u + c) & 0xFF;

    cpuSetFlags(ctx, ctx->regs.a == 0, 0,
                (a & 0xF) + (u & 0xF) + c > 0xF,
                a + u + c > 0xFF);
}

static void procAdd(CpuContext *ctx)
{
    u32 val = cpuReadReg(ctx->curInst->reg1) + ctx->fetchedData;

    bool is16bit = is16Bit(ctx->curInst->reg1);

    if (is16bit)
    {
        emuCycles(1);
    }

    if (ctx->curInst->reg1 == RT_SP)
    {
        val = cpuReadReg(ctx->curInst->reg1) + (int8_t)ctx->fetchedData;
    }

    int z = (val & 0xFF) == 0;
    int h = (cpuReadReg(ctx->curInst->reg1) & 0xF) + (ctx->fetchedData & 0xF) >= 0x10;
    int c = (int)(cpuReadReg(ctx->curInst->reg1) & 0xFF) + (int)(ctx->fetchedData & 0xFF) >= 0x100;

    if (is16bit)
    {
        z = -1;
        h = (cpuReadReg(ctx->curInst->reg1) & 0xFFF) + (ctx->fetchedData & 0xFFF) >= 0x1000;
        u32 n = ((u32)cpuReadReg(ctx->curInst->reg1)) + ((u32)ctx->fetchedData);
        c = n >= 0x10000;
    }

    if (ctx->curInst->reg1 == RT_SP)
    {
        z = 0;
        h = (cpuReadReg(ctx->curInst->reg1) & 0xF) + (ctx->fetchedData & 0xF) >= 0x10;
        c = (int)(cpuReadReg(ctx->curInst->reg1) & 0xFF) + (int)(ctx->fetchedData & 0xFF) >= 0x100;
    }

    cpuSetReg(ctx->curInst->reg1, val & 0xFFFF);
    cpuSetFlags(ctx, z, 0, h, c);
}

static IN_PROC processors[] = {
    [IN_NONE] = procNone,
    [IN_NOP] = procNop,
    [IN_LD] = procLd,
    [IN_LDH] = procLdh,
    [IN_JP] = procJp,
    [IN_DI] = procDi,
    [IN_POP] = procPop,
    [IN_PUSH] = procPush,
    [IN_JR] = procJr,
    [IN_CALL] = procCall,
    [IN_RET] = procRet,
    [IN_RST] = procRst,
    [IN_DEC] = procDec,
    [IN_INC] = procInc,
    [IN_ADD] = procAdd,
    [IN_ADC] = procAdc,
    [IN_SUB] = procSub,
    [IN_SBC] = procSbc,
    [IN_AND] = procAnd,
    [IN_XOR] = procXor,
    [IN_OR] = procOr,
    [IN_CP] = procCp,
    [IN_CB] = procCb,
    [IN_RRCA] = procRrca,
    [IN_RLCA] = procRlca,
    [IN_RRA] = procRra,
    [IN_RLA] = procRla,
    [IN_STOP] = procStop,
    [IN_HALT] = procHalt,
    [IN_DAA] = procDaa,
    [IN_CPL] = procCpl,
    [IN_SCF] = procScf,
    [IN_CCF] = procCcf,
    [IN_EI] = procEi,
    [IN_RETI] = procReti};

IN_PROC instGetProcessor(InType type)
{
    return processors[type];
}
