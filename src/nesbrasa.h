/* braza-nes.h
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

#pragma once

#include <stdint.h>

#include "cpu.h"
#include "ppu.h"

typedef struct _Cpu Cpu;
typedef struct _Ppu Ppu;
typedef struct _Nes Nes;

struct _Nes {
        uint8_t  ram[0x800];
        Cpu     *cpu;
        Ppu     *ppu;
};

Nes* nes_new  (void);

void nes_free (Nes *nes);
