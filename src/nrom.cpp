/* nrom.cpp
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

#include "nrom.hpp"

namespace nesbrasa::nucleo
{
    NRom::NRom(Cartucho *cartucho)
    {
        if (cartucho->possui_chr_ram())
        {
            // aloca a memória a ser útilizada pelo emulador
            cartucho->chr_ram.resize(0x2000);
            for (auto& valor : cartucho->chr_ram)
            {
                valor = 0;
            }
        }
    }

    uint8_t NRom::ler(Cartucho *cartucho, uint16_t endereco)
    {
        if (endereco < 0x2000)
        {
            // ler a rom CHR
            if (cartucho->possui_chr_ram())
            {
                return cartucho->chr_ram.at(endereco);
            }
            else
            {
                return cartucho->chr.at(endereco);   
            }
        }
        else if (endereco >= 0x8000)
        {
            // os bancos da rom PRG começam a partir do endereço 0x8000
            uint16_t endereco_mapeado = endereco - 0x8000;

            // espelhar o endereço caso a rom PRG só possua 1 banco
            if (cartucho->get_prg_quantidade() == 1)
            {
                return cartucho->prg.at(endereco_mapeado % 0x4000);
            }
            else
            {
                return cartucho->prg.at(endereco_mapeado);
            }
        }

        return 0;
    }

    void NRom::escrever(Cartucho *cartucho, uint16_t endereco, uint8_t valor)
    {
        if (!cartucho->possui_chr_ram())
        {
            throw string("CHR RAM inexistente");
        }

        if (endereco < 0x2000)
        {
            // escrever na rom CHR
            cartucho->chr.at(endereco) = valor;
        }
    }

    string NRom::get_nome()
    {
        return "NROM";
    }
}