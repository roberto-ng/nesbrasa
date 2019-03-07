#pragma once

#include <stdint.h>
#include <stdbool.h>

//! Busca uma posição de 0 a 7 em um byte e returna o seu valor
bool buscar_bit(uint8_t byte, uint8_t pos);
