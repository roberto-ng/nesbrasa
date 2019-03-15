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
  cpu->c = 0;
  cpu->z = 0;
  cpu->d = 0;
  cpu->v = 0;
  cpu->n = 0;
  cpu->i = 0;
  cpu->b = 0;
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

void
cpu_set_z (Cpu     *cpu,
           uint8_t  valor)
{
  // checa se um valor é '0'
  if (valor == 0)
    cpu->z = 0;
  else
    cpu->z = 1;
}

void
cpu_set_n (Cpu     *cpu,
           uint8_t  valor)
{
  // o valor é negativo se o bit mais significativo não for '0'
  if ((valor & 0b10000000) != 0)
    cpu->n = 1;
  else
    cpu->n = 0;
}

void
stack_push (Nes    *nes,
            uint8_t valor)
{
  uint16_t endereco = 0x0100 | nes->cpu->sp;
  escrever_memoria (nes, endereco, valor);

  nes->cpu->sp -= 1;
}

void
stack_push_16_bits (Nes      *nes,
                    uint16_t  valor)
{
  uint8_t menor = ler_memoria (nes, valor & 0x00FF);
  uint8_t maior = ler_memoria (nes, (valor & 0xFF00) >> 8);

  stack_push (nes, maior);
  stack_push (nes, menor);
}

uint8_t
stack_pull (Nes *nes)
{
  nes->cpu->sp += 1;
  uint16_t endereco = 0x0100 | nes->cpu->sp;
  return ler_memoria (nes, endereco);
}

uint16_t
stack_pull_16_bits (Nes *nes)
{
  uint8_t menor = stack_pull (nes);
  uint8_t maior = stack_pull (nes);

  return (maior << 8) | menor;
}
