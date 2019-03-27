/* memoria.c
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

#include "nesbrasa.hpp"
#include "memoria.hpp"

uint8_t ler_memoria(Nes *nes, uint16_t  endereco)
{
  if (endereco <= 0x07FF)
  {
    return nes->ram[endereco];
  }
  else if (endereco >= 0x0800 && endereco <=0x1FFF)
  {
    // endereços nesta area são espelhos dos endereços
    // localizados entre 0x0000 e 0x07FF
    return nes->ram[endereco % 0x0800];
  }
  else if (endereco >= 0x2000 && endereco <= 0x2007)
  {
    return ppu_registrador_ler(nes, endereco);
  }
  else if (endereco >= 0x2008 && endereco <= 0x3FFF)
  {
    // endereço espelhado do registrador
    uint16_t ender_espelhado = (endereco%0x8) + 0x2000;
    return ppu_registrador_ler(nes, ender_espelhado);
  }
  else if (endereco >= 0x4000 && endereco <= 0x4017)
  {
    // TODO: registradores da APU e de input/output
  }
  else if (endereco >= 0x4018 && endereco <= 0x401F)
  {
    // originalmente usado apenas em modo de testes da CPU
    return 0;
  }
  else if (endereco >= 0x4020 && endereco <= 0xFFFF)
  {
    return cartucho_mapeador_ler(nes->cartucho, endereco);
  }

  return 0;
}

uint16_t ler_memoria_16_bits(Nes *nes, uint16_t endereco)
{
  uint16_t menor = ler_memoria(nes, endereco);
  uint16_t maior = ler_memoria(nes, endereco + 1);

  return (maior << 8) | menor;
}

uint16_t ler_memoria_16_bits_bug(Nes *nes, uint16_t endereco)
{
  uint16_t menor = ler_memoria(nes, endereco);
  uint16_t maior = 0;

  if ((endereco & 0x00FF) == 0x00FF)
  {
    maior = ler_memoria(nes, endereco & 0xFF00);
  }
  else
  {
    maior = ler_memoria(nes, endereco + 1);
  }

  return (maior << 8) | menor;
}

void escrever_memoria(Nes *nes, uint16_t endereco, uint8_t valor)
{
  if (endereco <= 0x07FF)
  {
    nes->ram[endereco] = valor;
  }
  else if (endereco >= 0x0800 && endereco <=0x1FFF)
  {
    // endereços nesta area são espelhos dos endereços
    // localizados entre 0x0000 e 0x07FF
    nes->ram[endereco % 0x0800] = valor;
  }
  else if (endereco >= 0x2000 && endereco <= 0x2007)
  {
    ppu_registrador_escrever(nes, endereco, valor);
  }
  else if (endereco >= 0x2008 && endereco <= 0x3FFF)
  {
    // endereço espelhado do registrador
    uint16_t ender_espelhado = (endereco%0x8) + 0x2000;
    ppu_registrador_escrever(nes, ender_espelhado, valor);
  }
  else if (endereco >= 0x4000 && endereco <= 0x4017)
  {
    // TODO: registradores da APU e de input/output
  }
  else if (endereco >= 0x4018 && endereco <= 0x401F)
  {
    // originalmente usado apenas no modo de testes da CPU
    return;
  }
  else if (endereco >= 0x4020 && endereco <= 0xFFFF)
  {
    cartucho_mapeador_escrever(nes->cartucho, endereco, valor);
  }
}
