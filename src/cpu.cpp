/* cpu.cpp
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

#include "cpu.hpp"
#include "nesbrasa.hpp"
#include "util.hpp"
#include "memoria.hpp"
#include "instrucao.hpp"

using std::stringstream;

namespace nesbrasa::nucleo
{
    Cpu::Cpu(Memoria* memoria)
    {
        this->memoria = memoria;
        this->ciclos = 0;
        this->pc = 0;
        this->sp = 0;
        this->a = 0;
        this->x = 0;
        this->y = 0;
        this->c = false;
        this->z = false;
        this->i = false;
        this->d = false;
        this->b = false;
        this->v = false;
        this->n = false;
        this->esperar = 0;
        this->pag_alterada = false;
        this->instrucoes = carregar_instrucoes();
    }

    void Cpu::ciclo()
    {
        if (this->esperar > 0)
        {
            this->esperar -= 1;
            return;
        }

        uint32_t ciclos_qtd_anterior = this->ciclos;

        uint8_t opcode = this->memoria->ler(this->pc);
        if (!this->instrucoes.at(opcode).has_value())
        {
            printf("Erro: Uso de opcode inválido - %02X", opcode);
            return;
        }

        Instrucao& instrucao = this->instrucoes.at(opcode).value();
        this->executar(&instrucao);
        
        if (this->pag_alterada) 
        {
            this->ciclos += instrucao.ciclos;
            this->ciclos += instrucao.ciclos_pag_alterada;
        }
        else
        {
            this->ciclos += instrucao.ciclos;
        }

        // calcula a diferença da quantidade atual de ciclos com a 
        // quantidade anterior e a adiciona ao tempo de espera
        this->esperar += this->ciclos - ciclos_qtd_anterior;
    }

    void Cpu::executar(Instrucao* instrucao)
    {
        auto endereco = instrucao->buscar_endereco(this);
        
        this->pc += instrucao->bytes;
        instrucao->implementacao(instrucao, this, endereco);
    }

    void Cpu::resetar()
    {
        this->sp = 0xFD;
        this->pc = this->memoria->ler_16_bits(0xFFFC);
        this->set_estado(0b00100100);
    }

    void Cpu::branch_somar_ciclos(uint16_t endereco)
    {
        // somar 1 se os 2 endereços forem da mesma pagina,
        // somar 2 se forem de paginas diferentes
        if (comparar_paginas(this->pc, endereco))
            this->ciclos += 1;
        else
            this->ciclos += 2;
    }

    uint8_t Cpu::get_estado()
    {
        uint8_t flags = 0;

        const uint8_t c = this->c;
        const uint8_t z = this->z << 1;
        const uint8_t i = this->i << 2;
        const uint8_t d = this->d << 3;
        const uint8_t b = this->b << 4;
        const uint8_t v = this->v << 6;
        const uint8_t n = this->n << 7;
        // o bit na posiçao 5 sempre está ativo
        const uint8_t bit_5 = 1 << 5;

        return flags | c | z | i | d | b | bit_5 | v | n;
    }

    void Cpu::set_estado(uint8_t valor)
    {
        this->c = buscar_bit(valor, 0);
        this->z = buscar_bit(valor, 1);
        this->i = buscar_bit(valor, 2);
        this->d = buscar_bit(valor, 3);
        this->b = buscar_bit(valor, 4);
        this->v = buscar_bit(valor, 6);
        this->n = buscar_bit(valor, 7);
    }

    void Cpu::stack_empurrar(uint8_t valor)
    {
        uint16_t endereco = 0x0100 | this->sp;
        this->memoria->escrever(endereco, valor);

        this->sp -= 1;
    }

    void Cpu::stack_empurrar_16_bits(uint16_t valor)
    {
        uint8_t menor = valor & 0x00FF;
        uint8_t maior = (valor & 0xFF00) >> 8;

        this->stack_empurrar(maior);
        this->stack_empurrar(menor);
    }

    uint8_t Cpu::stack_puxar()
    {
        this->sp += 1;
        uint16_t endereco = 0x0100 | this->sp;
        return this->memoria->ler(endereco);
    }

    uint16_t Cpu::stack_puxar_16_bits()
    {
        uint8_t menor = this->stack_puxar();
        uint8_t maior = this->stack_puxar();

        return (maior << 8) | menor;
    }

    void Cpu::esperar_adicionar(uint16_t esperar)
    {
        this->esperar += esperar;
    }

    void Cpu::set_z(uint8_t valor)
    {
        // checa se um valor é '0'
        if (valor == 0)
            this->z = true;
        else
            this->z = false;
    }

    void Cpu::set_n(uint8_t valor)
    {
        // o valor é negativo se o bit mais significativo não for '0'
        if ((valor & 0b10000000) != 0)
            this->n = true;
        else
            this->n = false;
    }
    
    uint32_t Cpu::get_ciclos()
    {
        return this->ciclos;
    }

    uint16_t Cpu::get_esperar()
    {
        return this->esperar;
    }

    Memoria* Cpu::get_memoria()
    {
        return this->memoria;
    }

    string Cpu::instrucao_para_asm(uint8_t opcode)
    {
        auto instrucao = &this->instrucoes.at(opcode).value();

        switch (instrucao->modo)
        {
            case InstrucaoModo::ACM:
            {   
                stringstream ss;
                
                ss << instrucao->nome << " $" << std::uppercase << std::hex << (int)this->a;

                return ss.str();
            }

            case InstrucaoModo::ABS:
            {
                stringstream ss;

                uint16_t endereco = instrucao->buscar_endereco(this).value();
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco;
            
                return ss.str();
            }

            case InstrucaoModo::ABS_X:
            {
                stringstream ss;

                uint16_t endereco = instrucao->buscar_endereco(this).value();
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco << ", X";

                return ss.str();
            }

            case InstrucaoModo::ABS_Y:
            {
                stringstream ss;

                uint16_t valor = this->memoria->ler_16_bits(this->pc + 1);
                ss << instrucao->nome << " $" << std::uppercase << std::hex << valor << ", Y";

                return ss.str();
            }

            case InstrucaoModo::IMED:
            {
                int valor = this->memoria->ler(this->pc + 1);

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
                ss << instrucao->nome << " ($" << std::uppercase << std::hex << this->pc+1 << ")";

                return ss.str();
            }

            case InstrucaoModo::IND_X:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " ($" << std::uppercase << std::hex << valor << ", X)";

                return ss.str();
            }

            case InstrucaoModo::IND_Y:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " ($" << std::uppercase << std::hex << valor << "), Y";

                return ss.str();
            }

            case InstrucaoModo::REL:
            {
                int endereco = instrucao->buscar_endereco(this).value();

                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << endereco << "";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << valor;

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_X:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << valor << ", X";

                return ss.str();
            }

            case InstrucaoModo::P_ZERO_Y:
            {
                int valor = this->memoria->ler(this->pc + 1);

                stringstream ss;
                ss << instrucao->nome << " $" << std::uppercase << std::hex << valor << ", Y";

                return ss.str();
            }

            default:
                return "???";
        }
    }
}