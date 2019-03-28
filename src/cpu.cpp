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

#include <stdlib.h>

#include "cpu.hpp"
#include "nesbrasa.hpp"
#include "util.hpp"
#include "memoria.hpp"
#include "instrucao.hpp"

Cpu::Cpu()
{
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

void Cpu::ciclo(Nes* nes)
{
  if (this->esperar > 0)
  {
    this->esperar -= 1;
    return;
  }

  uint32_t ciclos_qtd_anterior = this->ciclos;

  uint8_t opcode = ler_memoria(nes, this->pc);
  if (!this->instrucoes[opcode].has_value())
    return;

  Instrucao& instrucao = this->instrucoes[opcode].value();
  uint16_t endereco = instrucao.buscar_endereco(nes);

  this->pc += instrucao.bytes;
  instrucao.executar(nes, endereco);
  
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

void Cpu::set_z(uint8_t valor)
{
  // checa se um valor é '0'
  if (valor == 0)
    this->z = false;
  else
    this->z = true;
}

void Cpu::set_n(uint8_t valor)
{
  // o valor é negativo se o bit mais significativo não for '0'
  if ((valor & 0b10000000) != 0)
    this->n = true;
  else
    this->n = false;
}

void Cpu::stack_empurrar(Nes *nes, uint8_t valor)
{
  uint16_t endereco = 0x0100 | this->sp;
  escrever_memoria(nes, endereco, valor);

  this->sp -= 1;
}

void Cpu::stack_empurrar_16_bits(Nes *nes, uint16_t valor)
{
  uint8_t menor = ler_memoria(nes, valor & 0x00FF);
  uint8_t maior = ler_memoria(nes, (valor & 0xFF00) >> 8);

  this->stack_empurrar(nes, maior);
  this->stack_empurrar(nes, menor);
}

uint8_t Cpu::stack_puxar(Nes *nes)
{
  this->sp += 1;
  uint16_t endereco = 0x0100 | this->sp;
  return ler_memoria(nes, endereco);
}

uint16_t Cpu::stack_puxar_16_bits(Nes *nes)
{
  uint8_t menor = this->stack_puxar(nes);
  uint8_t maior = this->stack_puxar(nes);

  return (maior << 8) | menor;
}
