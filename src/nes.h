#pragma once

#include <stdint.h>

typedef struct _Nes Nes;

struct _Nes {
        uint8_t ram[0x800];
};
