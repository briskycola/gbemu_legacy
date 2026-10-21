#include "../include/cpu.h"
#include "../include/bus.h"
#include "../include/cartridge.h"
#include "../include/debug.h"
#include "../include/emulator.h"
#include "../include/interrupts.h"
#include "../include/timer.h"

CpuContext ctx = {0};

#define CPU_DEBUG 0

#define CPU_TRACE_LENGTH 128
static u16 tracePc[CPU_TRACE_LENGTH];
static u8 traceOpcode[CPU_TRACE_LENGTH];
static u8 traceBank[CPU_TRACE_LENGTH];
static unsigned traceNext;
static unsigned traceCount;
static unsigned haltBugCount;
static u16 lastHaltBugPc;

void cpuTraceInstruction(u16 pc, u8 opcode, u8 romBank)
{
    // The ROM uses the fixed 0x0038 vector as a one-byte call trampoline.
    // Omitting its four instructions leaves more room for the actual caller.
    if (pc >= 0x0038 && pc <= 0x003B)
    {
        return;
    }

    tracePc[traceNext] = pc;
    traceOpcode[traceNext] = opcode;
    traceBank[traceNext] = romBank;
    traceNext = (traceNext + 1) % CPU_TRACE_LENGTH;
    if (traceCount < CPU_TRACE_LENGTH)
    {
        traceCount++;
    }
}

void cpuNoteHaltBug(u16 pc)
{
    haltBugCount++;
    lastHaltBugPc = pc;
}

void cpuDumpTrace()
{
    fprintf(stderr, "HALT bug triggers=%u last_after_pc=%04X\n",
            haltBugCount, lastHaltBugPc);
    fprintf(stderr, "Recent CPU instructions (oldest to newest):\n");
    unsigned first = (traceNext + CPU_TRACE_LENGTH - traceCount) % CPU_TRACE_LENGTH;
    for (unsigned i = 0; i < traceCount; i++)
    {
        unsigned at = (first + i) % CPU_TRACE_LENGTH;
        fprintf(stderr, "  %04X: %02X [ROM bank %u]\n",
                tracePc[at], traceOpcode[at], traceBank[at]);
    }
}

void cpuInit()
{
    ctx.regs.pc = 0x100;
    ctx.regs.sp = 0xFFFE;
    *((short *)&ctx.regs.a) = 0xB001;
    *((short *)&ctx.regs.b) = 0x1300;
    *((short *)&ctx.regs.d) = 0xD800;
    *((short *)&ctx.regs.h) = 0x4D01;
    ctx.ieRegister = 0;
    ctx.intFlags = 0;
    ctx.intMasterEnabled = false;
    ctx.enablingIme = false;
    ctx.halted = false;
    ctx.haltBug = false;

    timerGetContext()->div = 0xABCC;
}

static void fetchInstruction()
{
    u16 pc = ctx.regs.pc;
    ctx.curOpcode = busRead(pc);
    u8 romBank = pc < 0x4000 ? 0 : cartCurrentRomBank();
    cpuTraceInstruction(pc, ctx.curOpcode, romBank);
    if (ctx.haltBug)
    {
        ctx.haltBug = false;
    }
    else
    {
        ctx.regs.pc++;
    }
    ctx.curInst = instructionByOpcode(ctx.curOpcode);
}

void fetchData();

static void execute()
{
    IN_PROC proc = instGetProcessor(ctx.curInst->type);

    if (!proc)
    {
        NO_IMPL
    }

    proc(&ctx);
}

bool cpuStep()
{

    if (!ctx.halted)
    {
        // u16 pc = ctx.regs.pc;

        fetchInstruction();
        emuCycles(1);
        fetchData();

#if CPU_DEBUG == 1
        char flags[16];
        sprintf(flags, "%c%c%c%c",
                ctx.regs.f & (1 << 7) ? 'Z' : '-',
                ctx.regs.f & (1 << 6) ? 'N' : '-',
                ctx.regs.f & (1 << 5) ? 'H' : '-',
                ctx.regs.f & (1 << 4) ? 'C' : '-');

        char inst[16];
        instToStr(&ctx, inst);

        printf("%08lX - %04X: %-12s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n",
               emuGetContext()->ticks,
               pc, inst, ctx.curOpcode,
               busRead(pc + 1), busRead(pc + 2), ctx.regs.a, flags, ctx.regs.b, ctx.regs.c,
               ctx.regs.d, ctx.regs.e, ctx.regs.h, ctx.regs.l);
#endif

        if (ctx.curInst == NULL)
        {
            printf("Unknown Instruction! %02X\n", ctx.curOpcode);
            exit(-7);
        }

        dbgUpdate();
        dbgPrint();

        execute();
    }
    else
    {
        // is halted...
        emuCycles(1);

        if (ctx.intFlags & ctx.ieRegister)
        {
            ctx.halted = false;
        }
    }

    if (ctx.intMasterEnabled)
    {
        cpuHandleInterrupts(&ctx);
        ctx.enablingIme = false;
    }

    if (ctx.enablingIme)
    {
        ctx.intMasterEnabled = true;
    }

    return true;
}

u8 cpuGetIeRegister()
{
    return ctx.ieRegister;
}

void cpuSetIeRegister(u8 n)
{
    ctx.ieRegister = n;
}

void cpuRequestInterrupt(InterruptType t)
{
    ctx.intFlags |= t;
}
