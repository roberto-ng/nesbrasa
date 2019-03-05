#include "memoria.h"
#include "nesbrasa.h"

typedef struct _Nes Nes;

uint8_t
ler_memoria (Nes      *nes,
             uint16_t  endereco)
{
  if (endereco <= 0x07FF) {
    return nes->ram[endereco];
  }
  else if (endereco >= 0x0800 && endereco <=0x1FFF) {
    // endereços nesta area são espelhos dos endereços
    // localizados entre 0x0000 e 0x07FF
    return nes->ram[endereco % 0x0800];
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
ler_memoria_16_bits (Nes      *nes,
                     uint16_t  endereco)
{
  uint16_t menor = ler_memoria (nes, endereco);
  uint16_t maior = ler_memoria (nes, endereco + 1);

  return (maior << 8) | menor;
}

uint16_t
ler_memoria_16_bits_bug (Nes      *nes,
                         uint16_t  endereco)
{
  uint16_t menor = ler_memoria (nes, endereco);
  uint16_t maior = 0;

  if ((endereco & 0x00FF) == 0x00FF) {
    maior = ler_memoria (nes, endereco & 0xFF00);
  }
  else {
    maior = ler_memoria (nes, endereco + 1);
  }

  return (maior << 8) | menor;
}

void
escrever_memoria (Nes      *nes,
                  uint16_t  endereco,
                  uint8_t   valor)
{
  //TODO: implementar
}
