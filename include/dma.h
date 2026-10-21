#pragma once

#include "common.h"

void dmaStart(uint8_t start);
void dmaTick();

bool dmaTransferring();
