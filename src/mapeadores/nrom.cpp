/* nrom.cpp
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

#include "nrom.hpp"

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
