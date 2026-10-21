#pragma once

#include "common.h"

typedef struct
{
    uint8_t entry[4];
    uint8_t logo[0x30];

    char title[16];
    uint16_t newLicCode;
    uint8_t sgbFlag;
    uint8_t type;
    uint8_t romSize;
    uint8_t ramSize;
    uint8_t destCode;
    uint8_t licCode;
    uint8_t version;
    uint8_t checksum;
    uint16_t globalChecksum;
} RomHeader;

bool cartLoad(char *cart);

uint8_t cartRead(uint16_t address);
void cartWrite(uint16_t address, uint8_t value);
uint8_t cartCurrentRomBank();
void cartDumpBankTrace();

bool cartNeedSave();
void cartBatteryLoad();
void cartBatterySave();
