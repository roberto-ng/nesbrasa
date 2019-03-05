#include "cpu.h"

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
