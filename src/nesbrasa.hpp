/* nesbrasa.hpp
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
#include <memory>
#include <array>

#include "cpu.hpp"
#include "ppu.hpp"
#include "cartucho.hpp"

using std::unique_ptr;
using std::array;

class Nes
{
public:
        array<uint8_t, 0x800> ram;

        unique_ptr<Cpu>      cpu;
        unique_ptr<Ppu>      ppu;
        unique_ptr<Cartucho> cartucho;

        Nes();
};
