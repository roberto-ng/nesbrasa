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
        int resultado = this->cartucho->carregar_rom(rom);
        
        if (resultado == -1)
        {
            throw string("Erro: formato não reconhecido");
        }
        
        if (resultado == -2)
        {
            throw string("Erro: mapeador não reconhecido");
        }
        
        if (resultado != 0)
        {
            throw string("Erro");
        }

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
                ss << instrucao->nome << " $" << std::hex << this->cpu->a;

                return ss.str();
            }

            case InstrucaoModo::ABS:
            {
                stringstream ss;
                uint16_t endereco = instrucao->buscar_endereco(this->cpu.get());
                ss << instrucao->nome << " $" << std::hex << endereco;
            
                return ss.str();
            }

            case InstrucaoModo::ABS_X:
            {
                stringstream ss;
                uint16_t endereco = instrucao->buscar_endereco(this->cpu.get());
                ss << instrucao->nome << " $" << std::hex << endereco << ", X";

                return ss.str();
            }

            case InstrucaoModo::ABS_Y:
            {
                stringstream ss;
                uint16_t endereco = instrucao->buscar_endereco(this->cpu.get());
                ss << instrucao->nome << " $" << std::hex << endereco << ", Y";

                return ss.str();
            }

            case InstrucaoModo::IMED:
            {
                stringstream ss;
                ss << instrucao->nome << " #$" << std::hex << this->cpu->pc + 1;

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
                ss << instrucao->nome << " ($" << std::hex << cpu->pc+1 << ")";

                return ss.str();
            }

            case InstrucaoModo::IND_X:
            {
                stringstream ss;
                ss << instrucao->nome << " ($" << std::hex << cpu->pc+1 << ", X)";

                return ss.str();
            }

            case InstrucaoModo::IND_Y:
            {
                stringstream ss;
                ss << instrucao->nome << " ($" << std::hex << cpu->pc+1 << "), Y";

                return ss.str();
            }

            case InstrucaoModo::REL:
            {
                stringstream ss;
                ss << instrucao->nome << " ##" << std::hex << cpu->pc+1 << "";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO:
            {
                stringstream ss;
                ss << instrucao->nome << " $" << std::hex << cpu->pc + 1;

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_X:
            {
                stringstream ss;
                ss << instrucao->nome << " $" << std::hex << cpu->pc + 1 << ", X";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_Y:
            {
                stringstream ss;
                ss << instrucao->nome << " $" << std::hex << cpu->pc + 1 << ", Y";

                return ss.str();
            }

            default:
                return "???";
        }
    }

}