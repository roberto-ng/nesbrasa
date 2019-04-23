/* cpu.hpp
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
#include <array>
#include <memory>
#include <optional>

#include "instrucao.hpp"
#include "memoria.hpp"

using std::array;
using std::shared_ptr;
using std::optional;

// referencias utilizadas:
// http://www.obelisk.me.uk/6502/registers.html

namespace nesbrasa::nucleo
{
    class Cpu
    {
    private:
        uint16_t esperar;
        uint32_t ciclos;

        // tabela de instruções
        array< optional<Instrucao>, 256 > instrucoes;
    
    public:
        Memoria* memoria;

        bool pag_alterada;

        uint16_t pc; // contador de programa
        uint8_t  sp; // ponteiro da stack

        uint8_t a; // registrador acumulador
        uint8_t x; // registrador de indice x
        uint8_t y; // registrador de indice y

        bool c; // flag de carregamento (carry flag)
        bool z; // flag zero
        bool i; // flag de desabilitar interrupções
        bool d; // flag decimal
        bool b; // flag da instrução break (break command flag)
        bool v; // flag de transbordamento (overflow flag)
        bool n; // flag de negativo

        Cpu(Memoria* memoria);
            
        void ciclo();

        void resetar();

        /*! Calcula a quantidade de ciclos em um branch e a soma em 'cpu->ciclos'.
            \param endereco O endereço em que o branch sera realizado
        */
        void branch_somar_ciclos(uint16_t endereco);

        uint8_t get_estado();

        void set_estado(uint8_t valor);

        //! Empurra um valor na stack
        void stack_empurrar(uint8_t valor);

        //! Empurra um valor na stack
        void stack_empurrar_16_bits(uint16_t valor);

        //! Puxa um valor da stack
        uint8_t stack_puxar();

        //! Puxa um valor da stack
        uint16_t stack_puxar_16_bits();

        void esperar_adicionar(uint16_t esperar);

        string instrucao_para_asm(uint8_t opcode);

        //! Ativa a flag de zero caso seja necessario
        void set_z(uint8_t valor);

        //! Ativa a flag de valor negativo caso seja necessario
        void set_n(uint8_t valor);
        
        uint32_t get_ciclos();

        uint16_t get_esperar();

        optional<Instrucao> get_instrucao(uint8_t opcode);

    private:
        void executar(Instrucao* instrucao);
    };
}