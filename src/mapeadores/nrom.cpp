#include "nrom.h"

uint8_t nrom_ler(Cartucho *cartucho, uint16_t endereco)
{
  if (endereco < 0x2000)
  {
    // ler a rom CHR
    return cartucho->chr[endereco];
  }
  else if (endereco >= 0x8000)
  {
    // os bancos da rom PRG começam a partir do endereço 0x8000
    uint16_t endereco_mapeado = endereco - 0x8000;

    // espelhar o endereço caso a rom PRG só possua 1 banco
    if (cartucho->prg_quantidade == 1)
    {
      return endereco_mapeado % 0x4000;
    }
    else
    {
      return endereco_mapeado;
    }
  }

  return 0;
}

void nrom_escrever(Cartucho *cartucho, uint16_t endereco, uint8_t valor)
{
  if (endereco < 0x2000)
  {
    // escrever na rom CHR
    cartucho->chr[endereco] = valor;
  }
}
