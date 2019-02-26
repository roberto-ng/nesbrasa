#include <stdlib.h>

#include "instrucao.h"
#include "memoria.h"
#include "cpu.h"

Instrucao*
instrucao_new (uint8_t       codigo,
               uint8_t       bytes,
               int           ciclos,
               InstrucaoModo modo,
               InstrucaoFunc funcao)
{
  Instrucao *instr = malloc (sizeof (Instrucao));
  instr->codigo = codigo;
  instr->bytes = bytes;
  instr->ciclos = ciclos;
  instr->modo = modo;
  instr->funcao = funcao;

  return instr;
}

void
instrucao_free (Instrucao *instr)
{
  free (instr);
}

static uint16_t
buscar_endereco (Instrucao *instrucao,
                 Cpu       *cpu)
{
  switch (instrucao->modo)
  {
    case MODO_ENDER_ACM:
      return 0;
    case MODO_ENDER_IMPL:
      return 0;
    case MODO_ENDER_IMED:
      return cpu->pc + 1;
    case MODO_ENDER_P_ZERO:
      return memoria_ler (cpu->memoria, (cpu->pc + 1)%0xFF);
    case MODO_ENDER_P_ZERO_X:
      return memoria_ler (cpu->memoria, (cpu->pc + 1 + cpu->x)%0xFF);
    case MODO_ENDER_P_ZERO_Y:
      return memoria_ler (cpu->memoria, (cpu->pc + 1 + cpu->y)%0xFF);
    case MODO_ENDER_ABS:
      return memoria_ler_16_bits (cpu->memoria, cpu->pc + 1);
    case MODO_ENDER_ABS_X:
      return memoria_ler_16_bits (cpu->memoria, cpu->pc + 1 + cpu->x);
    case MODO_ENDER_ABS_Y:
      return memoria_ler_16_bits (cpu->memoria, cpu->pc + 1 + cpu->y);
    case MODO_ENDER_IND: {
      const uint16_t valor = memoria_ler_16_bits (cpu->memoria, cpu->pc+1);
      return memoria_ler_16_bits_bug (cpu->memoria, valor);
    }
    case MODO_ENDER_INDEX_IND: {
      const uint16_t valor = memoria_ler (cpu->memoria, cpu->pc + 1);
      return memoria_ler_16_bits_bug (cpu->memoria, valor + cpu->x);
    }
    case MODO_ENDER_IND_INDEX: {
      const uint16_t valor = memoria_ler (cpu->memoria, cpu->pc + 1);
      return memoria_ler_16_bits_bug (cpu->memoria, valor + cpu->y);
    }
    case MODO_ENDER_REL: {
      const uint16_t valor = memoria_ler (cpu->memoria, cpu->pc + 1);
      return cpu->pc + 2 + valor - ((valor < 0x80) ? 0 : 0x100);
    }
  }

  return 0;
}

static void
adc(Instrucao *instrucao,
    Cpu       *cpu)
{
}
