#pragma once

#include "nesbrasa.h"

typedef struct _Nes Nes;

uint8_t  ler_memoria             (Nes      *memoria,
                                  uint16_t endereco);

uint16_t ler_memoria_16_bits     (Nes      *memoria,
                                  uint16_t endereco);

// implementa o bug do 6502 que ocorre quando o modo indireto utilizado
uint16_t ler_memoria_16_bits_bug (Nes      *memoria,
                                  uint16_t endereco);
