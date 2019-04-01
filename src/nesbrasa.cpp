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

#include "nesbrasa.hpp"
#include "cartucho.hpp"
#include "util.hpp"

using std::make_unique;

namespace nesbrasa::nucleo
{

    Nes::Nes()
    {
        this->memoria = make_unique<Memoria>(this);
        
        this->cpu = make_unique<Cpu>(this->memoria.get());
        this->ppu = make_unique<Ppu>(this->memoria.get());
        this->cartucho = make_unique<Cartucho>();
    }

    void Nes::carregar_rom(vector<uint8_t> rom)
    {
        int resultado = this->cartucho->carregar_rom(rom);
        
        if (resultado == -1)
        {
            throw "Erro: formato não reconhecido";
        }
        
        if (resultado == -2)
        {
            throw "Erro: mapeador não reconhecido";
        }
        
        if (resultado != 0)
        {
            throw "Erro";
        }

        this->cpu->resetar();
    }

    void Nes::ciclo()
    {
        if (!this->cartucho->rom_carregada)
        {
            return;
        }
    }
}