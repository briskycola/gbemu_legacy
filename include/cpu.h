#pragma once

#include "common.h"
#include "instructions.h"

typedef struct
{
    u8 a;
    u8 f;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u16 pc;
    u16 sp;
} CpuRegisters;

typedef struct
{
    CpuRegisters regs;

    // current fetch...
    u16 fetchedData;
    u16 memDest;
    bool destIsMem;
    u8 curOpcode;
    instruction *curInst;

    bool halted;
    bool haltBug;

    bool intMasterEnabled;
    bool enablingIme;
    u8 ieRegister;
    u8 intFlags;

} CpuContext;

CpuRegisters *cpuGetRegs();

void cpuInit();
bool cpuStep();
void cpuTraceInstruction(u16 pc, u8 opcode, u8 romBank);
void cpuDumpTrace();
void cpuNoteHaltBug(u16 pc);

typedef void (*IN_PROC)(CpuContext *);

IN_PROC instGetProcessor(InType type);

#define CPU_FLAG_Z BIT(ctx->regs.f, 7)
#define CPU_FLAG_N BIT(ctx->regs.f, 6)
#define CPU_FLAG_H BIT(ctx->regs.f, 5)
#define CPU_FLAG_C BIT(ctx->regs.f, 4)

u16 cpuReadReg(RegType rt);
void cpuSetReg(RegType rt, u16 val);

u8 cpuGetIeRegister();
void cpuSetIeRegister(u8 n);

u8 cpuReadReg8(RegType rt);
void cpuSetReg8(RegType rt, u8 val);

u8 cpuGetIntFlags();
void cpuSetIntFlags(u8 value);

void instToStr(CpuContext *ctx, char *str);
