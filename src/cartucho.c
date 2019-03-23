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

#include "cartucho.h"

Cartucho* cartucho_new(void)
{
  Cartucho *cartucho = malloc(sizeof(Cartucho));
  cartucho->espelhamento = ESPELHAMENTO_TELA_UNICA;
  cartucho->mapeador = mapeador_new();
  cartucho->prg = NULL;
  cartucho->chr = NULL;
  cartucho->sram = NULL;
  cartucho->rom_carregada = false;

  return cartucho;
}

void cartucho_free(Cartucho *cartucho)
{
  free(cartucho);
}
