#include <stdlib.h>

#include "nesbrasa.hpp"
#include "memoria.hpp"

using namespace nesbrasa::nucleo;

int main()
{
    // testa a implementação de um bug

    Nes nes;
    nes.memoria->escrever(0x0FFF, 0x05);
    nes.memoria->escrever(0x0F00, 0xF3);

    uint16_t valor = nes.memoria->ler_16_bits_bug(0x0FFF);

    if (valor == 0xF305)
    {
        // retornar erro
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }

}
