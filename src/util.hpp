/* util.hpp
 *
 * Copyright 2019 Roberto Nazareth <nazarethroberto97@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*! Macro que retorna um o tamanho de um array
 *  (Não funciona com ponteiros ou arrays passados como argumentos)
 */
#define TAMANHO(__LISTA)\
  (sizeof (__LISTA)/sizeof (__LISTA[0]))

//! Busca uma posição de 0 a 7 em um byte e returna o seu valor
bool buscar_bit(uint8_t byte, uint8_t pos);

//! Checa se dois endereços da memoria estão na mesma pagina
bool comparar_paginas(uint16_t pagina_1, uint16_t pagina_2);

//! Uma alternativa segura ao 'sprintf'
char* formatar_str(char *fmt, ...);
