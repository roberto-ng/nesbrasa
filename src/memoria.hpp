/* memoria.h
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

#include <cstdint>

class Nes;

//! Lê um valor de 8 bits na memoria
uint8_t ler_memoria(Nes* nes, uint16_t endereco);

//! Lê um valor de 16 bits na memoria
/*!
 \return O valor 16 bits do enrereço lido no formato little-endian
 */
uint16_t ler_memoria_16_bits(Nes* nes, uint16_t endereco);

//! Lê um valor de 16 bits na memoria com o bug do no modo indireto da cpu
/*!
 \return O valor 16 bits do enrereço lido no formato little-endian
 */
uint16_t ler_memoria_16_bits_bug(Nes* nes, uint16_t endereco);

//! Escreve um valor na memoria
void escrever_memoria(Nes* nes, uint16_t endereco, uint8_t valor);
