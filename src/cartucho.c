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

#include "cartucho.h"
#include "mapeadores/nrom.h"
#include "util.h"

Cartucho* cartucho_new(void)
{
  Cartucho *cartucho = malloc(sizeof(Cartucho));
  cartucho->espelhamento = ESPELHAMENTO_VERTICAL;
  cartucho->mapeador_tipo = MAPEADOR_DESCONHECIDO;
  cartucho->prg = NULL;
  cartucho->chr = NULL;
  cartucho->prg_quantidade = 0;
  cartucho->chr_quantidade = 0;
  cartucho->rom_carregada = false;
  cartucho->possui_sram = false;

  for (int i = 0; i < TAMANHO(cartucho->sram); i++)
  {
    cartucho->sram[i] = 0;
  }

  return cartucho;
}

void cartucho_free(Cartucho *cartucho)
{
  if (cartucho->prg != NULL)
    free(cartucho->prg);

  if (cartucho->chr != NULL)
    free(cartucho->chr);

  free(cartucho);
}

int cartucho_carregar_rom(Cartucho *cartucho, uint8_t *rom, size_t rom_tam)
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

  cartucho->possui_sram = buscar_bit(rom[6], 1);

  cartucho->prg_quantidade = rom[4];
  cartucho->chr_quantidade = rom[5];

  const uint32_t prg_tamanho = cartucho->prg_quantidade * 0x4000;
  const uint32_t chr_tamanho = cartucho->chr_quantidade * 0x2000;

  cartucho->prg = malloc(prg_tamanho);
  cartucho->chr = malloc(chr_tamanho);

  for (int i = 0; i < prg_tamanho; i++) {
    cartucho->prg[i] = 0;
  }

  for (int i = 0; i < chr_tamanho; i++) {
    cartucho->chr[i] = 0;
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
  memcpy(cartucho->prg, &rom[offset], prg_tamanho);

  // Copia os dados referentes à ROM CHR do arquivo para o array
  memcpy(cartucho->chr, &rom[offset+prg_tamanho], chr_tamanho);

  uint8_t mapeador_byte_menor = (rom[6] & 0xFF00) >> 8;
  uint8_t mapeador_byte_maior = (rom[7] & 0xFF00) >> 8;
  uint8_t mapeador_codigo = (mapeador_byte_maior << 8) | mapeador_byte_menor;

  switch ((MapeadorTipo)mapeador_codigo)
  {
    case MAPEADOR_NROM:
      cartucho->mapeador_tipo = MAPEADOR_NROM;
      break;

    case MAPEADOR_MMC1:
      cartucho->mapeador_tipo = MAPEADOR_MMC1;
      break;

    default:
      cartucho->mapeador_tipo = MAPEADOR_DESCONHECIDO;
      break;
  }

  if (buscar_bit(rom[6], 3) == true)
  {
    cartucho->espelhamento = ESPELHAMENTO_4_TELAS;
  }
  else
  {
    if (buscar_bit(rom[6], 0) == false)
    {
      cartucho->espelhamento = ESPELHAMENTO_VERTICAL;
    }
    else
    {
      cartucho->espelhamento = ESPELHAMENTO_HORIZONTAL;
    }
  }


  //TODO: Completar suporte a ROMs no formato NES 2.0
  return 0;
}

uint8_t cartucho_mapeador_ler(Cartucho *cartucho, uint16_t endereco)
{
  switch (cartucho->mapeador_tipo)
  {
    case MAPEADOR_NROM:
      return nrom_ler(cartucho, endereco);

    default:
      return 0;
  }
}

void cartucho_mapeador_escrever(Cartucho *cartucho, uint16_t endereco, uint8_t valor)
{
  switch (cartucho->mapeador_tipo)
  {
    case MAPEADOR_NROM:
      nrom_escrever(cartucho, endereco, valor);
      break;

    default:
      break;
  }
}
