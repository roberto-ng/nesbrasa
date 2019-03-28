/* cartucho.c
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
  this->espelhamento = ESPELHAMENTO_VERTICAL;
  this->mapeador_tipo = MAPEADOR_DESCONHECIDO;
  this->prg = NULL;
  this->chr = NULL;
  this->prg_quantidade = 0;
  this->chr_quantidade = 0;
  this->rom_carregada = false;
  this->possui_sram = false;

  for (int i = 0; i < TAMANHO(this->sram); i++)
  {
    this->sram[i] = 0;
  }
}

Cartucho::~Cartucho()
{
  if (this->prg != NULL)
    free(this->prg);

  if (this->chr != NULL)
    free(this->chr);
}

int Cartucho::carregar_rom(uint8_t *rom, size_t rom_tam)
{
  if (rom_tam < 16)
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

  this->prg_quantidade = rom[4];
  this->chr_quantidade = rom[5];

  const uint32_t prg_tamanho = this->prg_quantidade * 0x4000;
  const uint32_t chr_tamanho = this->chr_quantidade * 0x2000;

  this->prg = (uint8_t *)malloc(prg_tamanho);
  this->chr = (uint8_t *)malloc(chr_tamanho);

  for (int i = 0; i < prg_tamanho; i++) {
    this->prg[i] = 0;
  }

  for (int i = 0; i < chr_tamanho; i++) {
    this->chr[i] = 0;
  }

  bool contem_trainer = buscar_bit(rom[6], 2);
  int offset = 16 + ((contem_trainer) ? 512 : 0);

  // checa o tamanho do arquivo
  if (rom_tam < (offset + prg_tamanho + chr_tamanho))
  {
    // formato inválido
    return -1;
  }

  // Copia os dados referentes à ROM PRG do arquivo para o array
  memcpy(this->prg, &rom[offset], prg_tamanho);

  // Copia os dados referentes à ROM CHR do arquivo para o array
  memcpy(this->chr, &rom[offset+prg_tamanho], chr_tamanho);

  uint8_t mapeador_byte_menor = (rom[6] & 0xFF00) >> 8;
  uint8_t mapeador_byte_maior = (rom[7] & 0xFF00) >> 8;
  uint8_t mapeador_codigo = (mapeador_byte_maior << 8) | mapeador_byte_menor;

  switch ((MapeadorTipo)mapeador_codigo)
  {
    case MAPEADOR_NROM:
      this->mapeador_tipo = MAPEADOR_NROM;
      break;

    case MAPEADOR_MMC1:
      this->mapeador_tipo = MAPEADOR_MMC1;
      break;

    default:
      this->mapeador_tipo = MAPEADOR_DESCONHECIDO;
      break;
  }

  if (buscar_bit(rom[6], 3) == true)
  {
    this->espelhamento = ESPELHAMENTO_4_TELAS;
  }
  else
  {
    if (buscar_bit(rom[6], 0) == false)
    {
      this->espelhamento = ESPELHAMENTO_VERTICAL;
    }
    else
    {
      this->espelhamento = ESPELHAMENTO_HORIZONTAL;
    }
  }

  //TODO: Completar suporte a ROMs no formato NES 2.0
  return 0;
}

uint8_t Cartucho::mapeador_ler(uint16_t endereco)
{
  switch (this->mapeador_tipo)
  {
    case MAPEADOR_NROM:
      return nrom_ler(this, endereco);

    default:
      return 0;
  }
}

void Cartucho::mapeador_escrever(uint16_t endereco, uint8_t valor)
{
  switch (this->mapeador_tipo)
  {
    case MAPEADOR_NROM:
      nrom_escrever(this, endereco, valor);
      break;

    default:
      break;
  }
}
