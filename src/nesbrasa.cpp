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
        this->cartucho->carregar_rom(rom);
        this->cpu->resetar();
    }

    void Nes::ciclo()
    {
        if (!this->cartucho->rom_carregada)
        {
            throw string("Erro: nenhuma ROM foi carregada");
        }

        this->cpu->ciclo();
    }

    string Nes::instrucao_para_asm(Instrucao* instrucao)
    {

        switch (instrucao->modo)
        {
            case InstrucaoModo::ACM:
            {   
                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << (int)this->cpu->a;

                return ss.str();
            }

            case InstrucaoModo::ABS:
            {
                stringstream ss;
                uint16_t endereco = instrucao->buscar_endereco(this->cpu.get());
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco;
            
                return ss.str();
            }

            case InstrucaoModo::ABS_X:
            {
                stringstream ss;
                uint16_t endereco = instrucao->buscar_endereco(this->cpu.get());
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco << ", X";

                return ss.str();
            }

            case InstrucaoModo::ABS_Y:
            {
                stringstream ss;
                uint16_t endereco = instrucao->buscar_endereco(this->cpu.get());
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco << ", Y";

                return ss.str();
            }

            case InstrucaoModo::IMED:
            {
                int valor = this->memoria->ler(this->cpu->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " #$" << std::uppercase << std::hex << valor;

                return ss.str();
            }

            case InstrucaoModo::IMPL:
            {
                stringstream ss;
                ss << instrucao->nome;

                return ss.str();
            }

            case InstrucaoModo::IND:
            {
                stringstream ss;
                ss << instrucao->nome << " ($" << std::uppercase << std::hex << cpu->pc+1 << ")";

                return ss.str();
            }

            case InstrucaoModo::IND_X:
            {
                int valor = cpu->memoria->ler(cpu->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " ($" << std::uppercase << std::hex << valor << ", X)";

                return ss.str();
            }

            case InstrucaoModo::IND_Y:
            {
                int valor = cpu->memoria->ler(cpu->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " ($" << std::uppercase << std::hex << valor << "), Y";

                return ss.str();
            }

            case InstrucaoModo::REL:
            {
                int endereco = instrucao->buscar_endereco(this->cpu.get());

                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco << "";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO:
            {
                int valor = this->memoria->ler(this->cpu->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << valor;

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_X:
            {
                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << cpu->pc + 1 << ", X";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_Y:
            {
                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << cpu->pc + 1 << ", Y";

                return ss.str();
            }

            default:
                return "???";
        }
    }
}