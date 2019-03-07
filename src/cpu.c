#include <stdlib.h>

#include "cpu.h"

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

  return cpu;
}

void
cpu_free (Cpu *cpu)
{
  free (cpu);
}

void
cpu_set_z (Cpu     *cpu,
           uint8_t  valor)
{
  // checa se um valor é '0'
  if (valor == 0) {
    cpu->z = 0;
  }
  else {
    cpu->z = 1;
  }
}

void
cpu_set_n (Cpu     *cpu,
           uint8_t  valor)
{
  // o valor é negativo se o bit mais significativo não for '0'
  if ((valor & 0b10000000) != 0) {
    cpu->n = 1;
  }
  else {
    cpu->n = 0;
  }
}

void
cpu_set_c (Cpu     *cpu,
           int32_t  valor)
{
  if (valor > 0xFF) {
    cpu->c = 1;
  }
  else {
    cpu->c = 0;
  }
}
