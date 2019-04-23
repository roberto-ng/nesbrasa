/* memoria.cpp
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

#include <sstream>

#include "memoria.hpp"
#include "nesbrasa.hpp"

using std::stringstream;

namespace nesbrasa::nucleo
{
    Memoria::Memoria(Nes *nes)
    {
        this->nes = nes;
    
        for (auto& valor : this->ram)
        {
            valor = 0;
        }
    }

    uint8_t Memoria::ler(uint16_t endereco)
    {
        if (endereco <= 0x07FF)
        {
            return this->ram.at(endereco);
        }
        else if (endereco >= 0x0800 && endereco <=0x1FFF)
        {
            // endereços nesta area são espelhos dos endereços
            // localizados entre 0x0000 e 0x07FF
            return this->ram.at(endereco % 0x0800);
        }
        else if (endereco >= 0x2000 && endereco <= 0x2007)
        {
            return this->nes->ppu.registrador_ler(nes, endereco);
        }
        else if (endereco >= 0x2008 && endereco <= 0x3FFF)
        {
            // endereço espelhado do registrador
            uint16_t ender_espelhado = (endereco%0x8) + 0x2000;
            return this->nes->ppu.registrador_ler(nes, ender_espelhado);
        }
        else if (endereco >= 0x4000 && endereco <= 0x4017)
        {
            // TODO: registradores da APU e de input/output
            return 0;
        }
        else if (endereco >= 0x4018 && endereco <= 0x401F)
        {
            // originalmente usado apenas em modo de testes da CPU
            return 0;
        }
        else if (endereco >= 0x4020 && endereco <= 0xFFFF)
        {
            return this->nes->cartucho.ler(endereco);
        }

        // endereço não existe, jogar erro
        stringstream ss;
        ss << "Tentativa de leitura em um endereço não existente na memória";
        ss << " (" << std::hex << endereco << ") ";
        ss << endereco;

        throw ss.str();
    }

    uint16_t Memoria::ler_16_bits(uint16_t endereco)
    {
        uint16_t menor = this->ler(endereco);
        uint16_t maior = this->ler(endereco + 1);

        return (maior << 8) | menor;
    }

    uint16_t Memoria::ler_16_bits_bug(uint16_t endereco)
    {
        uint16_t menor = this->ler(endereco);
        uint16_t maior = 0;

        if ((endereco & 0x00FF) == 0x00FF)
        {
            maior = this->ler(endereco & 0xFF00);
        }
        else
        {
            maior = this->ler(endereco + 1);
        }

        return (maior << 8) | menor;
    }

    void Memoria::escrever(uint16_t endereco, uint8_t valor)
    {
        if (endereco <= 0x07FF)
        {
            this->ram.at(endereco) = valor;
        }
        else if (endereco >= 0x0800 && endereco <=0x1FFF)
        {
            // endereços nesta area são espelhos dos endereços
            // localizados entre 0x0000 e 0x07FF
            this->ram.at(endereco % 0x0800) = valor;
        }
        else if (endereco >= 0x2000 && endereco <= 0x2007)
        {
            this->nes->ppu.registrador_escrever(nes, endereco, valor);
        }
        else if (endereco >= 0x2008 && endereco <= 0x3FFF)
        {
            // endereço espelhado do registrador
            uint16_t ender_espelhado = (endereco%0x8) + 0x2000;
            this->nes->ppu.registrador_escrever(nes, ender_espelhado, valor);
        }
        else if (endereco >= 0x4000 && endereco <= 0x4017)
        {
            // TODO: registradores da APU e de input/output
        }
        else if (endereco >= 0x4018 && endereco <= 0x401F)
        {
            // originalmente usado apenas no modo de testes da CPU
            return;
        }
        else if (endereco >= 0x4020 && endereco <= 0xFFFF)
        {
            this->nes->cartucho.escrever(endereco, valor);
        }
        else
        {
            // endereço não existe, jogar erro
            stringstream ss;
            ss << "Tentativa de escrita do valor " << std::hex << valor;
            ss << " em um endereço não existente na memória";
            ss << " (" << std::hex << endereco << ")";

            throw ss.str();
        }
    }
}