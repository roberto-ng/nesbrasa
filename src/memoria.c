#include "memoria.h"

uint8_t
memoria_ler (Memoria  *memoria,
             uint16_t endereco)
{
  if (endereco <= 0x07FF) {
    return memoria->ram[endereco];
  }
  else if (endereco >= 0x0800 && endereco <=0x1FFF) {
    // endereços nesta area são espelhos dos endereços
    // localizados entre 0x0000 e 0x07FF
    return memoria->ram[endereco % 0x0800];
  }
  else if (endereco >= 0x2000 && endereco <= 0x2007) {
    // TODO: retornar registradores da PPU
    return 0;
  }
  else if (endereco >= 0x2008 && endereco <= 0x3FFF) {
    // TODO: espelhos dos registradores da PPU
    return 0;
  }
  else if (endereco >= 0x4000 && endereco <= 0x4017) {
    // TODO: registradores da APU e de input/output
  }
  else if (endereco >= 0x4020 && endereco <= 0xFFFF) {
    // TODO: espaço do cartucho
    return 0;
  }

  return 0;
}

uint16_t
memoria_ler_16_bits (Memoria  *memoria,
                     uint16_t endereco)
{
  uint16_t baixo = memoria_ler (memoria, endereco);
  uint16_t alto  = memoria_ler (memoria, endereco + 1);

  return (alto << 8) | baixo;
}

// implementa o bug no modo indireto da cpu 6502
uint16_t
memoria_ler_16_bits_bug (Memoria  *memoria,
                         uint16_t endereco)
{
  uint16_t baixo = memoria_ler (memoria, endereco);
  uint16_t alto  = 0;

  if ((baixo & 0x00FF) == 0x00FF) {
    alto = memoria_ler (memoria, - 0xFF);
  }
  else {
    alto = memoria_ler (memoria, endereco + 1);
  }

  return (alto << 8) | baixo;
}
