#pragma once

#include "cpu.h"

typedef struct _Memoria Memoria;

struct _Memoria {
        uint8_t     *ram;
        struct _Cpu *cpu;
};

uint8_t  memoria_ler             (Memoria  *memoria,
                                  uint16_t endereco);

uint16_t memoria_ler_16_bits     (Memoria  *memoria,
                                  uint16_t endereco);

// implementa o bug do 6502 que ocorre quando o modo indireto utilizado
uint16_t memoria_ler_16_bits_bug (Memoria  *memoria,
                                  uint16_t endereco);
