#include <stdlib.h>

#include "nesbrasa.hpp"

int main()
{
  // testa a implementação de um bug

  Nes nes;
  escrever_memoria(&nes, 0x0FFF, 0x05);
  escrever_memoria(&nes, 0x0F00, 0xF3);

  if (ler_memoria_16_bits_bug(&nes, 0x0FFF) != 0xF305)
  {
    // retornar erro
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
