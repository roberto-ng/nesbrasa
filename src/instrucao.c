#include <stdlib.h>

#include "cpu.h"
#include "instrucao.h"
#include "memoria.h"
#include "nesbrasa.h"

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

/*!
  Busca o endereço que vai ser usado pela instrução de
  acordo com o modo de endereçamento da CPU
 */
static uint16_t
buscar_endereco (Instrucao *instrucao,
                 Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  switch (instrucao->modo) {
  case MODO_ENDER_ACM:
    return 0;

  case MODO_ENDER_IMPL:
    return 0;

  case MODO_ENDER_IMED:
    return nes->cpu->pc + 1;

  case MODO_ENDER_P_ZERO:
    return ler_memoria (nes, (cpu->pc + 1)%0xFF);

  case MODO_ENDER_P_ZERO_X:
    return ler_memoria (nes, (cpu->pc + 1 + cpu->x)%0xFF);

  case MODO_ENDER_P_ZERO_Y:
    return ler_memoria (nes, (cpu->pc + 1 + cpu->y)%0xFF);

  case MODO_ENDER_ABS:
    return ler_memoria_16_bits (nes, cpu->pc + 1);

  case MODO_ENDER_ABS_X:
    return ler_memoria_16_bits (nes, cpu->pc + 1 + cpu->x);

  case MODO_ENDER_ABS_Y:
    return ler_memoria_16_bits (nes, cpu->pc + 1 + cpu->y);

  case MODO_ENDER_IND:
    {
      const uint16_t valor = ler_memoria_16_bits (nes, cpu->pc+1);
      return ler_memoria_16_bits_bug (nes, valor);
    }

  case MODO_ENDER_INDEX_IND:
    {
      const uint16_t valor = ler_memoria (nes, cpu->pc + 1);
      return ler_memoria_16_bits_bug (nes, valor + cpu->x);
    }

  case MODO_ENDER_IND_INDEX:
    {
      const uint16_t valor = ler_memoria (nes, cpu->pc + 1);
      return ler_memoria_16_bits_bug (nes, valor + cpu->y);
    }

  case MODO_ENDER_REL:
    {
      const uint16_t valor = ler_memoria (nes, cpu->pc + 1);
      return cpu->pc + 2 + valor - ((valor < 0x80) ? 0 : 0x100);
    }
  }

  return 0;
}

/*!
  Instrução ADC
  A + M + C -> A, C
 */
static void
adc (Instrucao *instrucao,
     Nes       *nes)
{
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t m = valor;
  const uint8_t c = nes->cpu->c;

  nes->cpu->a = a + m + c;

  if (((int32_t)a + (int32_t)m + (int32_t)c) > 0xFF) {
    nes->cpu->c = 1;
  }
  else {
    nes->cpu->c = 0;
  }

  // checa se houve um overflow/transbordamento
  // solução baseada em: https://stackoverflow.com/a/16861251
  if ((~(a ^ m)) & (a ^ c) & 0x80) {
    nes->cpu->v = 1;
  }
  else {
    nes->cpu->v = 0;
  }

  cpu_set_n (nes->cpu, nes->cpu->a);
  cpu_set_z (nes->cpu, nes->cpu->a);
}

/*!
  Instrução AND com o acumulador
  A AND M -> A
 */
static void
and (Instrucao *instrucao,
     Nes       *nes)
{
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t m = valor;

  nes->cpu->a = a & m;

  cpu_set_n (nes->cpu, nes->cpu->a);
  cpu_set_z (nes->cpu, nes->cpu->a);
}

/*!
  Instrução shift para a esquerda com a memoria ou com o acumulador
 */
static void
asl (Instrucao *instrucao,
     Nes       *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM) {
    // checa se o setimo bit do valor é '1'
    if ((nes->cpu->a & 0b01000000) >> 7 == 1) {
      nes->cpu->c = 1;
    }
    else {
      nes->cpu->c = 0;
    }

    nes->cpu->a <<= 1;

    cpu_set_n (nes->cpu, nes->cpu->a);
    cpu_set_z (nes->cpu, nes->cpu->a);
  }
  else {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    uint8_t valor = ler_memoria (nes, endereco);

    // checa se o setimo bit do valor é '1'
    if ((valor & 0b01000000) >> 7 == 1) {
      nes->cpu->c = 1;
    }
    else {
      nes->cpu->c = 0;
    }

    escrever_memoria (nes, endereco, valor << 1);

    cpu_set_n (nes->cpu, nes->cpu->a);
    cpu_set_z (nes->cpu, nes->cpu->a);
  }
}
