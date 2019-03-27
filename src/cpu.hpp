/* cpu.h
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
#include <cstdbool>

#include "nesbrasa.hpp"
#include "instrucao.hpp"
#include "memoria.hpp"

// referencias utilizadas:
// http://www.obelisk.me.uk/6502/registers.html

class Cpu
{
public:
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

        uint16_t esperar;
        uint32_t ciclos;
        bool     pag_alterada;

        Instrucao **instrucoes;
};

Cpu* cpu_new(void);

void cpu_free(Cpu *cpu);

void cpu_ciclo(Cpu* cpu, Nes *nes);

/*! Calcula a quantidade de ciclos em um branch e a soma em 'cpu->ciclos'.
 \param endereco O endereço em que o branch sera realizado
 */
void cpu_branch_somar_ciclos(Cpu *cpu, uint16_t endereco);

uint8_t cpu_estado_ler(Cpu *cpu);

void cpu_estado_escrever(Cpu *cpu, uint8_t valor);

//! Ativa a flag de zero caso seja necessario
void cpu_z_escrever(Cpu *cpu, uint8_t valor);

//! Ativa a flag de valor negativo caso seja necessario
void cpu_n_escrever(Cpu *cpu, uint8_t valor);

//! Empurra um valor na stack
void stack_empurrar(Nes *nes, uint8_t valor);

//! Empurra um valor na stack
void stack_empurrar_16_bits(Nes *nes, uint16_t valor);

//! Puxa um valor da stack
uint8_t stack_puxar(Nes *nes);

//! Puxa um valor da stack
uint16_t stack_puxar_16_bits(Nes *nes);
