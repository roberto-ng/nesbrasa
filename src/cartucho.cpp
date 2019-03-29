/* cartucho.cpp
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

#include <stdlib.h>
#include <string.h>

#include "cartucho.hpp"
#include "mapeadores/nrom.hpp"
#include "util.hpp"

Cartucho::Cartucho()
{
  this->espelhamento = Espelhamento::VERTICAL;
  this->mapeador_tipo = MapeadorTipo::DESCONHECIDO;
  this->prg_quantidade = 0;
  this->chr_quantidade = 0;
  this->rom_carregada = false;
  this->possui_sram = false;
  
  this->sram.reserve(0x2000);
  for (uint32_t i = 0; i < this->sram.capacity(); i++)
  {
    this->sram[i] = 0;
  }
}

void Cartucho::resetar_arrays()
{
  vector<uint8_t>().swap(this->prg);
  vector<uint8_t>().swap(this->chr);
  vector<uint8_t>().swap(this->sram);
}

int Cartucho::carregar_rom(vector<uint8_t> rom)
{
  this->resetar_arrays();
  
  this->sram.reserve(0x2000);
  for (uint32_t i = 0; i < this->sram.capacity(); i++)
  {
    this->sram[i] = 0;
  }

  if (rom.size() < 16)
    return -1;

  bool formato_ines = false;
  bool formato_nes20 = false;

  // arquivos nos formatos iNES e NES 2.0 começam com a string "NES<EOF>"
  if (rom[0] == 'N' && rom[1] == 'E' && rom[2] == 'S' && rom[3] == 0x1A)
  {
    if (buscar_bit(rom[7], 2) == false && buscar_bit(rom[7], 3) == true)
    {
      // o arquivo está no formato NES 2.0
      formato_nes20 = true;
    }
    else
    {
      // o arquivo está no formato iNES
      formato_ines = true;
    }
  }

  if (!formato_ines && !formato_nes20)
  {
    // formato inválido
    return -1;
  }

  this->possui_sram = buscar_bit(rom[6], 1);

  bool contem_trainer = buscar_bit(rom[6], 2);
  int offset = 16 + ((contem_trainer) ? 512 : 0);

  this->prg_quantidade = rom[4];
  this->chr_quantidade = rom[5];

  const uint32_t prg_tamanho = this->prg_quantidade * 0x4000;
  const uint32_t chr_tamanho = this->chr_quantidade * 0x2000;

  this->prg.reserve(prg_tamanho);
  this->chr.reserve(chr_tamanho);

  // checa o tamanho do arquivo
  if ((offset + prg_tamanho + chr_tamanho) > rom.size())
  {
    // formato inválido
    return -1;
  }

  // Copia os dados referentes à ROM PRG do arquivo para o array
  for (uint32_t i = 0; i < this->prg.capacity(); i++)
  {
    this->prg[i] = rom[offset+i];
  }

  // Copia os dados referentes à ROM CHR do arquivo para o array
  for (uint32_t i = 0; i < this->chr.capacity(); i++) {
    this->chr[i] = rom[offset+prg_tamanho+i];
  }

  uint8_t mapeador_byte_menor = (rom[6] & 0xFF00) >> 8;
  uint8_t mapeador_byte_maior = (rom[7] & 0xFF00) >> 8;
  uint8_t mapeador_codigo = (mapeador_byte_maior << 8) | mapeador_byte_menor;

  switch (mapeador_codigo)
  {
    case 0:
      this->mapeador_tipo = MapeadorTipo::NROM;
      break;

    case 1:
      this->mapeador_tipo = MapeadorTipo::MMC1;
      break;

    default:
      this->mapeador_tipo = MapeadorTipo::DESCONHECIDO;
      break;
  }

  if (buscar_bit(rom[6], 3) == true)
  {
    this->espelhamento = Espelhamento::QUATRO_TELAS;
  }
  else
  {
    if (buscar_bit(rom[6], 0) == false)
    {
      this->espelhamento = Espelhamento::VERTICAL;
    }
    else
    {
      this->espelhamento = Espelhamento::HORIZONTAL;
    }
  }

  //TODO: Completar suporte a ROMs no formato NES 2.0
  return 0;
}

uint8_t Cartucho::mapeador_ler(uint16_t endereco)
{
  switch (this->mapeador_tipo)
  {
    case MapeadorTipo::NROM:
      return nrom_ler(this, endereco);

    default:
      return 0;
  }
}

void Cartucho::mapeador_escrever(uint16_t endereco, uint8_t valor)
{
  switch (this->mapeador_tipo)
  {
    case MapeadorTipo::NROM:
      nrom_escrever(this, endereco, valor);
      break;

    default:
      break;
  }
}
