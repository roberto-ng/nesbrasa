#pragma once

#include "nesbrasa.h"

typedef struct _Nes Nes;

//! Lê um valor de 8 bits na memoria
uint8_t  ler_memoria             (Nes      *memoria,
                                  uint16_t endereco);

//! Lê um valor de 16 bits na memoria
/*!
 \return O valor 16 bits do enrereço lido no formato little-endian
 */
uint16_t ler_memoria_16_bits     (Nes      *memoria,
                                  uint16_t endereco);

//! Lê um valor de 16 bits na memoria com o bug do no modo indireto da cpu
/*!
 \return O valor 16 bits do enrereço lido no formato little-endian
 */
uint16_t ler_memoria_16_bits_bug (Nes      *memoria,
                                  uint16_t endereco);
