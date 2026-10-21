#include "../include/cartridge.h"
#include "../include/cpu.h"
#include <string.h>

#define BANK_TRACE_LENGTH 16

typedef struct
{
    u16 pc;
    u8 value;
    u8 bank;
} BankTraceEntry;

static BankTraceEntry bankTrace[BANK_TRACE_LENGTH];
static unsigned bankTraceNext;
static unsigned bankTraceCount;

typedef struct
{
    char filename[1024];
    u32 romSize;
    u8 *romData;
    RomHeader *header;

    // mbc1 related data
    bool ramEnabled;
    bool ramBanking;

    u8 *romBankX;
    u8 bankingMode;

    u8 romBankValue;
    u8 ramBankValue;

    u8 *ramBank;      // current selected ram bank
    u8 *ramBanks[16]; // all ram banks

    // for battery
    bool battery;  // has battery
    bool needSave; // should save battery backup.
} CartContext;

static CartContext ctx;

bool cartNeedSave()
{
    return ctx.needSave;
}

bool cartMbc1()
{
    return BETWEEN(ctx.header->type, 1, 3);
}

bool cartBattery()
{
    // mbc1 only for now...
    return ctx.header->type == 3;
}

static const char *ROM_TYPES[] = {
    "ROM ONLY",
    "MBC1",
    "MBC1+RAM",
    "MBC1+RAM+BATTERY",
    "0x04 ???",
    "MBC2",
    "MBC2+BATTERY",
    "0x07 ???",
    "ROM+RAM 1",
    "ROM+RAM+BATTERY 1",
    "0x0A ???",
    "MMM01",
    "MMM01+RAM",
    "MMM01+RAM+BATTERY",
    "0x0E ???",
    "MBC3+TIMER+BATTERY",
    "MBC3+TIMER+RAM+BATTERY 2",
    "MBC3",
    "MBC3+RAM 2",
    "MBC3+RAM+BATTERY 2",
    "0x14 ???",
    "0x15 ???",
    "0x16 ???",
    "0x17 ???",
    "0x18 ???",
    "MBC5",
    "MBC5+RAM",
    "MBC5+RAM+BATTERY",
    "MBC5+RUMBLE",
    "MBC5+RUMBLE+RAM",
    "MBC5+RUMBLE+RAM+BATTERY",
    "0x1F ???",
    "MBC6",
    "0x21 ???",
    "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
};

static const char *LIC_CODE[0xA5] = {
    [0x00] = "None",
    [0x01] = "Nintendo R&D1",
    [0x08] = "Capcom",
    [0x13] = "Electronic Arts",
    [0x18] = "Hudson Soft",
    [0x19] = "b-ai",
    [0x20] = "kss",
    [0x22] = "pow",
    [0x24] = "PCM Complete",
    [0x25] = "san-x",
    [0x28] = "Kemco Japan",
    [0x29] = "seta",
    [0x30] = "Viacom",
    [0x31] = "Nintendo",
    [0x32] = "Bandai",
    [0x33] = "Ocean/Acclaim",
    [0x34] = "Konami",
    [0x35] = "Hector",
    [0x37] = "Taito",
    [0x38] = "Hudson",
    [0x39] = "Banpresto",
    [0x41] = "Ubi Soft",
    [0x42] = "Atlus",
    [0x44] = "Malibu",
    [0x46] = "angel",
    [0x47] = "Bullet-Proof",
    [0x49] = "irem",
    [0x50] = "Absolute",
    [0x51] = "Acclaim",
    [0x52] = "Activision",
    [0x53] = "American sammy",
    [0x54] = "Konami",
    [0x55] = "Hi tech entertainment",
    [0x56] = "LJN",
    [0x57] = "Matchbox",
    [0x58] = "Mattel",
    [0x59] = "Milton Bradley",
    [0x60] = "Titus",
    [0x61] = "Virgin",
    [0x64] = "LucasArts",
    [0x67] = "Ocean",
    [0x69] = "Electronic Arts",
    [0x70] = "Infogrames",
    [0x71] = "Interplay",
    [0x72] = "Broderbund",
    [0x73] = "sculptured",
    [0x75] = "sci",
    [0x78] = "THQ",
    [0x79] = "Accolade",
    [0x80] = "misawa",
    [0x83] = "lozc",
    [0x86] = "Tokuma Shoten Intermedia",
    [0x87] = "Tsukuda Original",
    [0x91] = "Chunsoft",
    [0x92] = "Video system",
    [0x93] = "Ocean/Acclaim",
    [0x95] = "Varie",
    [0x96] = "Yonezawa/s’pal",
    [0x97] = "Kaneko",
    [0x99] = "Pack in soft",
    [0xA4] = "Konami (Yu-Gi-Oh!)"};

const char *cartLicName()
{
    if (ctx.header->newLicCode <= 0xA4)
    {
        return LIC_CODE[ctx.header->licCode];
    }

    return "UNKNOWN";
}

const char *cartTypeName()
{
    if (ctx.header->type <= 0x22)
    {
        return ROM_TYPES[ctx.header->type];
    }

    return "UNKNOWN";
}

void cartSetupBanking()
{
    for (int i = 0; i < 16; i++)
    {
        ctx.ramBanks[i] = 0;

        if ((ctx.header->ramSize == 2 && i == 0) ||
            (ctx.header->ramSize == 3 && i < 4) ||
            (ctx.header->ramSize == 4 && i < 16) ||
            (ctx.header->ramSize == 5 && i < 8))
        {
            ctx.ramBanks[i] = malloc(0x2000);
            memset(ctx.ramBanks[i], 0, 0x2000);
        }
    }

    ctx.ramBank = ctx.ramBanks[0];
    ctx.romBankX = ctx.romData + 0x4000; // rom bank 1
}

bool cartLoad(char *cart)
{
    snprintf(ctx.filename, sizeof(ctx.filename), "%s", cart);

    FILE *fp = fopen(cart, "r");

    if (!fp)
    {
        printf("Failed to open: %s\n", cart);
        return false;
    }

    printf("Opened: %s\n", ctx.filename);

    fseek(fp, 0, SEEK_END);
    ctx.romSize = ftell(fp);

    rewind(fp);

    ctx.romData = malloc(ctx.romSize);
    fread(ctx.romData, ctx.romSize, 1, fp);
    fclose(fp);

    ctx.header = (RomHeader *)(ctx.romData + 0x100);
    ctx.header->title[15] = 0;
    ctx.battery = cartBattery();
    ctx.needSave = false;

    printf("Cartridge Loaded:\n");
    printf("\t Title    : %s\n", ctx.header->title);
    printf("\t Type     : %2.2X (%s)\n", ctx.header->type, cartTypeName());
    printf("\t ROM Size : %d KB\n", 32 << ctx.header->romSize);
    printf("\t RAM Size : %2.2X\n", ctx.header->ramSize);
    printf("\t LIC Code : %2.2X (%s)\n", ctx.header->licCode, cartLicName());
    printf("\t ROM Vers : %2.2X\n", ctx.header->version);

    cartSetupBanking();

    u16 x = 0;
    for (u16 i = 0x0134; i <= 0x014C; i++)
    {
        x = x - ctx.romData[i] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", ctx.header->checksum, (x & 0xFF) ? "PASSED" : "FAILED");

    if (ctx.battery)
    {
        cartBatteryLoad();
    }

    return true;
}

void cartBatteryLoad()
{
    if (!ctx.ramBank)
    {
        return;
    }

    char fn[1048];
    sprintf(fn, "%s.battery", ctx.filename);
    FILE *fp = fopen(fn, "rb");

    if (!fp)
    {
        fprintf(stderr, "FAILED TO OPEN: %s\n", fn);
        return;
    }

    fread(ctx.ramBank, 0x2000, 1, fp);
    fclose(fp);
}

void cartBatterySave()
{
    if (!ctx.ramBank)
    {
        return;
    }

    char fn[1048];
    sprintf(fn, "%s.battery", ctx.filename);
    FILE *fp = fopen(fn, "wb");

    if (!fp)
    {
        fprintf(stderr, "FAILED TO OPEN: %s\n", fn);
        return;
    }

    fwrite(ctx.ramBank, 0x2000, 1, fp);
    fclose(fp);
}

u8 cartRead(u16 address)
{
    if (!cartMbc1() || address < 0x4000)
    {
        return ctx.romData[address];
    }

    if ((address & 0xE000) == 0xA000)
    {
        if (!ctx.ramEnabled)
        {
            return 0xFF;
        }

        if (!ctx.ramBank)
        {
            return 0xFF;
        }

        return ctx.ramBank[address - 0xA000];
    }

    return ctx.romBankX[address - 0x4000];
}

void cartWrite(u16 address, u8 value)
{
    if (!cartMbc1())
    {
        return;
    }

    if (address < 0x2000)
    {
        ctx.ramEnabled = ((value & 0xF) == 0xA);
    }

    if ((address & 0xE000) == 0x2000)
    {
        // rom bank number
        if (value == 0)
        {
            value = 1;
        }

        value &= 0b11111;

        ctx.romBankValue = value;
        ctx.romBankX = ctx.romData + (0x4000 * ctx.romBankValue);

        bankTrace[bankTraceNext] = (BankTraceEntry){
            .pc = cpuGetRegs()->pc,
            .value = value,
            .bank = cartCurrentRomBank(),
        };
        bankTraceNext = (bankTraceNext + 1) % BANK_TRACE_LENGTH;
        if (bankTraceCount < BANK_TRACE_LENGTH)
        {
            bankTraceCount++;
        }
    }

    if ((address & 0xE000) == 0x4000)
    {
        // ram bank number
        ctx.ramBankValue = value & 0b11;

        if (ctx.ramBanking)
        {
            if (cartNeedSave())
            {
                cartBatterySave();
            }

            ctx.ramBank = ctx.ramBanks[ctx.ramBankValue];
        }
    }

    if ((address & 0xE000) == 0x6000)
    {
        // banking mode select
        ctx.bankingMode = value & 1;

        ctx.ramBanking = ctx.bankingMode;

        if (ctx.ramBanking)
        {
            if (cartNeedSave())
            {
                cartBatterySave();
            }

            ctx.ramBank = ctx.ramBanks[ctx.ramBankValue];
        }
    }

    if ((address & 0xE000) == 0xA000)
    {
        if (!ctx.ramEnabled)
        {
            return;
        }

        if (!ctx.ramBank)
        {
            return;
        }

        ctx.ramBank[address - 0xA000] = value;

        if (ctx.battery)
        {
            ctx.needSave = true;
        }
    }
}

u8 cartCurrentRomBank()
{
    return (u8)((ctx.romBankX - ctx.romData) / 0x4000);
}

void cartDumpBankTrace()
{
    fprintf(stderr, "Recent MBC1 ROM bank writes (oldest to newest):\n");
    unsigned first = (bankTraceNext + BANK_TRACE_LENGTH - bankTraceCount) % BANK_TRACE_LENGTH;
    for (unsigned i = 0; i < bankTraceCount; i++)
    {
        unsigned at = (first + i) % BANK_TRACE_LENGTH;
        fprintf(stderr, "  pc=%04X value=%02X -> bank=%u\n",
                bankTrace[at].pc, bankTrace[at].value, bankTrace[at].bank);
    }
}
