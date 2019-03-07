/* nesbrasa.cpp
 *
 * Copyright 2019 Roberto Nazareth
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

#include "nesbrasa.h"
#include "util.h"

Nes*
nes_new (void)
{
  Nes *nes = malloc (sizeof (Nes));
  nes->cpu = cpu_new ();
  nes->ppu = ppu_new ();

  for (int i = 0; i < TAMANHO (nes->ram); i++) {
    nes->ram[i] = 0;
  }

  return nes;
}

void nes_free (Nes *nes)
{
  cpu_free (nes->cpu);
  ppu_free (nes->ppu);
  free (nes);
}
