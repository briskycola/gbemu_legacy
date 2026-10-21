#pragma once

#include "common.h"

typedef struct
{
    u8 entry[4];
    u8 logo[0x30];

    char title[16];
    u16 newLicCode;
    u8 sgbFlag;
    u8 type;
    u8 romSize;
    u8 ramSize;
    u8 destCode;
    u8 licCode;
    u8 version;
    u8 checksum;
    u16 globalChecksum;
} RomHeader;

bool cartLoad(char *cart);

u8 cartRead(u16 address);
void cartWrite(u16 address, u8 value);
u8 cartCurrentRomBank();
void cartDumpBankTrace();

bool cartNeedSave();
void cartBatteryLoad();
void cartBatterySave();
