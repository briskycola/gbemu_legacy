#pragma once

#include "common.h"
#include "instructions.h"

typedef struct
{
    uint8_t a;
    uint8_t f;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t pc;
    uint16_t sp;
} CpuRegisters;

typedef struct
{
    CpuRegisters regs;

    // current fetch...
    uint16_t fetchedData;
    uint16_t memDest;
    bool destIsMem;
    uint8_t curOpcode;
    instruction *curInst;

    bool halted;
    bool haltBug;

    bool intMasterEnabled;
    bool enablingIme;
    uint8_t ieRegister;
    uint8_t intFlags;

} CpuContext;

CpuRegisters *cpuGetRegs();

void cpuInit();
bool cpuStep();
void cpuTraceInstruction(uint16_t pc, uint8_t opcode, uint8_t romBank);
void cpuDumpTrace();
void cpuNoteHaltBug(uint16_t pc);

typedef void (*IN_PROC)(CpuContext *);

IN_PROC instGetProcessor(InType type);

#define CPU_FLAG_Z BIT(ctx->regs.f, 7)
#define CPU_FLAG_N BIT(ctx->regs.f, 6)
#define CPU_FLAG_H BIT(ctx->regs.f, 5)
#define CPU_FLAG_C BIT(ctx->regs.f, 4)

uint16_t cpuReadReg(RegType rt);
void cpuSetReg(RegType rt, uint16_t val);

uint8_t cpuGetIeRegister();
void cpuSetIeRegister(uint8_t n);

uint8_t cpuReadReg8(RegType rt);
void cpuSetReg8(RegType rt, uint8_t val);

uint8_t cpuGetIntFlags();
void cpuSetIntFlags(uint8_t value);

void instToStr(CpuContext *ctx, char *str);
