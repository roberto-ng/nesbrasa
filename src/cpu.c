/* cpu.c
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

#include "cpu.h"
#include "util.h"

Cpu*
cpu_new (void)
{
  Cpu *cpu = malloc (sizeof (Cpu));
  cpu->pc = 0;
  cpu->sp = 0;
  cpu->a = 0;
  cpu->x = 0;
  cpu->y = 0;
  cpu->c = false;
  cpu->z = false;
  cpu->i = false;
  cpu->d = false;
  cpu->b = false;
  cpu->v = false;
  cpu->n = false;
  cpu->esperar = 0;

  return cpu;
}

void
cpu_free (Cpu *cpu)
{
  free (cpu);
}

void
cpu_branch_somar_ciclos (Cpu      *cpu,
                         uint16_t  endereco)
{
  // somar 1 se os 2 endereços forem da mesma pagina,
  // somar 2 se forem de paginas diferentes
  if (comparar_paginas (cpu->pc, endereco))
    cpu->ciclos += 1;
  else
    cpu->ciclos += 2;
}

uint8_t
cpu_estado_ler (Cpu *cpu)
{
  uint8_t flags = 0;

  const uint8_t c = cpu->c;
  const uint8_t z = cpu->z << 1;
  const uint8_t i = cpu->i << 2;
  const uint8_t d = cpu->d << 3;
  const uint8_t b = cpu->b << 4;
  const uint8_t v = cpu->v << 6;
  const uint8_t n = cpu->n << 7;
  // o bit na posiçao 5 sempre está ativo
  const uint8_t bit_5 = 1 << 5;

  return flags | c | z | i | d | b | bit_5 | v | n;
}

void
cpu_estado_escrever (Cpu     *cpu,
                     uint8_t  valor)
{
  cpu->c = buscar_bit (valor, 0);
  cpu->z = buscar_bit (valor, 1);
  cpu->i = buscar_bit (valor, 2);
  cpu->d = buscar_bit (valor, 3);
  cpu->b = buscar_bit (valor, 4);
  cpu->v = buscar_bit (valor, 6);
  cpu->n = buscar_bit (valor, 7);
}

void
cpu_z_escrever (Cpu     *cpu,
                uint8_t  valor)
{
  // checa se um valor é '0'
  if (valor == 0)
    cpu->z = false;
  else
    cpu->z = true;
}

void
cpu_n_escrever (Cpu     *cpu,
                uint8_t  valor)
{
  // o valor é negativo se o bit mais significativo não for '0'
  if ((valor & 0b10000000) != 0)
    cpu->n = true;
  else
    cpu->n = false;
}

void
stack_empurrar (Nes    *nes,
                uint8_t valor)
{
  uint16_t endereco = 0x0100 | nes->cpu->sp;
  escrever_memoria (nes, endereco, valor);

  nes->cpu->sp -= 1;
}

void
stack_empurrar_16_bits (Nes      *nes,
                        uint16_t  valor)
{
  uint8_t menor = ler_memoria (nes, valor & 0x00FF);
  uint8_t maior = ler_memoria (nes, (valor & 0xFF00) >> 8);

  stack_empurrar (nes, maior);
  stack_empurrar (nes, menor);
}

uint8_t
stack_puxar (Nes *nes)
{
  nes->cpu->sp += 1;
  uint16_t endereco = 0x0100 | nes->cpu->sp;
  return ler_memoria (nes, endereco);
}

uint16_t
stack_puxar_16_bits (Nes *nes)
{
  uint8_t menor = stack_puxar (nes);
  uint8_t maior = stack_puxar (nes);

  return (maior << 8) | menor;
}
