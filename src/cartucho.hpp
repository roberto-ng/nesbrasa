/* cartucho.h
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
#include <cstdlib>
#include <cstdbool>

typedef enum
{
  ESPELHAMENTO_HORIZONTAL,
  ESPELHAMENTO_VERTICAL,
  ESPELHAMENTO_TELA_UNICA,
  ESPELHAMENTO_4_TELAS,
} Espelhamento;

typedef enum
{
  MAPEADOR_NROM = 0,
  MAPEADOR_MMC1 = 1,
  MAPEADOR_DESCONHECIDO,
} MapeadorTipo;

class Cartucho
{
public:
        uint8_t *prg;
        uint8_t *chr;
        uint8_t  sram[0x2000];

        uint8_t prg_quantidade;
        uint8_t chr_quantidade;

        MapeadorTipo mapeador_tipo;
        Espelhamento espelhamento;

        bool rom_carregada;
        bool possui_sram;

        Cartucho();
        ~Cartucho();
        
        int carregar_rom(uint8_t *rom, size_t rom_tam);

        uint8_t mapeador_ler(uint16_t endereco);

        void mapeador_escrever(uint16_t endereco, uint8_t valor);
};