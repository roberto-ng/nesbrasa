/* nesbrasa.cpp
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

#include <string>
#include <sstream>

#include "nesbrasa.hpp"
#include "cartucho.hpp"
#include "util.hpp"

using std::make_unique;
using std::stringstream;
using std::runtime_error;

namespace nesbrasa::nucleo
{
    using namespace std::string_literals;

    Nes::Nes(): memoria(this),
                cartucho(),
                cpu(&this->memoria),
                ppu(&this->memoria)
    {}

    void Nes::carregar_rom(vector<uint8_t> rom)
    {
        this->cartucho.carregar_rom(rom);
        this->cpu.resetar();
    }

    void Nes::ciclo()
    {
        if (!this->cartucho.possui_rom_carregada())
        {
            throw runtime_error("Erro: nenhuma ROM foi carregada"s);
        }

        this->cpu.ciclo();
    }
}