#include "nesbrasa.h"

int
main ()
{
  // testa a implementação de um bug

  Nes *nes = nes_new ();
  escrever_memoria (nes, 0x0FFF, 0x05);
  escrever_memoria (nes, 0x0F00, 0xF3);

  uint16_t valor = ler_memoria_16_bits_bug (nes, 0x0FFF);
  nes_free (nes);

  if (valor != 0xF305) {
    return 1;
  }

  return 0;
}
