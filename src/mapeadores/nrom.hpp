#include <stdint.h>

#include "../cartucho.hpp"

uint8_t nrom_ler(Cartucho *cartucho, uint16_t endereco);

void nrom_escrever(Cartucho *cartucho, uint16_t endereco, uint8_t valor);
