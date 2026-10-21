#pragma once

#include "common.h"

void dmaStart(u8 start);
void dmaTick();

bool dmaTransferring();
