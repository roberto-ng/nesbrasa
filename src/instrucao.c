/* instrucao.c
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
#include "instrucao.h"
#include "memoria.h"
#include "nesbrasa.h"
#include "util.h"

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
  switch (instrucao->modo)
  {
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
instrucao_adc (Instrucao *instrucao,
               Nes       *nes)
{
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t m = valor;
  const uint8_t c = nes->cpu->c;

  nes->cpu->a = a + m + c;

  // atualiza a flag c
  if (((int32_t)a + (int32_t)m + (int32_t)c) > 0xFF)
    nes->cpu->c = 1;
  else
    nes->cpu->c = 0;

  // checa se houve um overflow/transbordamento e atualiza a flag v
  // solução baseada em: https://stackoverflow.com/a/16861251
  if ((~(a ^ m)) & (a ^ c) & 0x80)
    nes->cpu->v = 1;
  else
    nes->cpu->v = 0;

  // atualiza as flags z e n
  cpu_n_escrever (nes->cpu, nes->cpu->a);
  cpu_z_escrever (nes->cpu, nes->cpu->a);
}

/*!
  Instrução AND
  A AND M -> A
 */
static void
instrucao_and (Instrucao *instrucao,
               Nes       *nes)
{
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t m = valor;

  nes->cpu->a = a & m;

  // atualizar flags
  cpu_n_escrever (nes->cpu, nes->cpu->a);
  cpu_z_escrever (nes->cpu, nes->cpu->a);
}

/*!
  Instrução shift para a esquerda.
  Utiliza a memoria ou o acumulador
 */
static void
instrucao_asl (Instrucao *instrucao,
               Nes       *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    // checa se a posição 7 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit (nes->cpu->a, 7);

    nes->cpu->a <<= 1;

    // atualizar flags
    cpu_n_escrever (nes->cpu, nes->cpu->a);
    cpu_z_escrever (nes->cpu, nes->cpu->a);
  }
  else
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    uint8_t valor = ler_memoria (nes, endereco);

    // checa se a posição 7 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit (valor, 7);

    valor <<= 1;

    escrever_memoria (nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever (nes->cpu, valor);
    cpu_z_escrever (nes->cpu, valor);
  }
}

//! Pula para o endereço indicado se a flag 'c' não estiver ativa
static void
instrucao_bcc (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->c == false)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'c' estiver ativa
static void
instrucao_bcs (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->c == true)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'z' estiver ativa
static void
instrucao_beq (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->z == true)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

/*! BIT
  Busca um byte na memoria e depois salva a posição 7 do byte em 'n'
  e a posição 6 do byte em 'v'.
  A flag 'z' tambem é alterada sendo calculada com 'a' AND valor
 */
static void
instrucao_bit (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  cpu->n = buscar_bit (valor, 7);
  cpu->v = buscar_bit (valor, 6);
  cpu->z = valor & cpu->a;
}

//! Pula para o endereço indicado se a flag 'n' estiver ativa
static void
instrucao_bmi (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->n == true)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'z' não estiver ativa
static void
instrucao_bne (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->z == true)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'n' não estiver ativa
static void
instrucao_bpl (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->n == false)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Instrução BRK
static void
instrucao_brk (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  stack_push_16_bits (nes, cpu->pc);
  stack_push (nes, cpu_estado_ler (cpu));

  cpu->b = 1;
  cpu->pc = ler_memoria_16_bits (nes, 0xFFFE);
}

//! Pula para o endereço indicado se a flag 'v' não estiver ativa
static void
instrucao_bvc (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->n == false)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'v' estiver ativa
static void
instrucao_bvs (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  if (cpu->n == true)
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    cpu_branch_somar_ciclos (cpu, endereco);
    cpu->pc = endereco;
  }
}

//! Limpa a flag 'c'
static void
instrucao_clc (Instrucao *instrucao,
               Nes       *nes)
{
  nes->cpu->c = false;
}

//! Limpa a flag 'd'
static void
instrucao_cld (Instrucao *instrucao,
               Nes       *nes)
{
  nes->cpu->d = false;
}

//! Limpa a flag 'i'
static void
instrucao_cli (Instrucao *instrucao,
               Nes       *nes)
{
  nes->cpu->i = false;
}

//! Limpa a flag 'v'
static void
instrucao_clv (Instrucao *instrucao,
               Nes       *nes)
{
  nes->cpu->v = false;
}

//! Compara o acumulador com um valor
static void
instrucao_cmp (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  if (cpu->a >= valor)
    cpu->c = true;
  else
    cpu->c = false;

  uint8_t resultado = cpu->a - valor;

  // atualizar flags
  cpu_n_escrever (cpu, resultado);
  cpu_z_escrever (cpu, resultado);
}

//! Compara o indice X com um valor
static void
instrucao_cpx (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  if (cpu->x >= valor)
    cpu->c = true;
  else
    cpu->c = false;

  uint8_t resultado = cpu->x - valor;

  // atualizar flags
  cpu_n_escrever (cpu, resultado);
  cpu_z_escrever (cpu, resultado);
}

//! Compara o indice Y com um valor
static void
instrucao_cpy (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  if (cpu->y >= valor)
    cpu->c = true;
  else
    cpu->c = false;

  uint8_t resultado = cpu->y - valor;
  cpu_n_escrever (cpu, resultado);
  cpu_z_escrever (cpu, resultado);
}

//! Diminui um valor na memoria por 1
static void
instrucao_dec (Instrucao *instrucao,
               Nes       *nes)
{
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  valor -= 1;

  // atualizar o valor na memoria
  escrever_memoria (nes, endereco, valor);

  // atualizar flags
  cpu_n_escrever (nes->cpu, valor);
  cpu_z_escrever (nes->cpu, valor);
}

//! Diminui o valor do indice X por 1
static void
instrucao_dex (Instrucao *instrucao,
               Nes       *nes)
{
  nes->cpu->x -= 1;

  // atualizar flags
  cpu_n_escrever (nes->cpu, nes->cpu->x);
  cpu_z_escrever (nes->cpu, nes->cpu->x);
}

//! Diminui o valor do indice Y por 1
static void
instrucao_dey (Instrucao *instrucao,
               Nes       *nes)
{
  nes->cpu->y -= 1;

  // atualizar flags
  cpu_n_escrever (nes->cpu, nes->cpu->y);
  cpu_z_escrever (nes->cpu, nes->cpu->y);
}

//! OR exclusivo de um valor na memoria com o acumulador
static void
instrucao_eor (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  cpu->a = cpu->a ^ valor;

  //atualizar flags
  cpu_n_escrever (cpu, cpu->a);
  cpu_z_escrever (cpu, cpu->a);
}

//! Incrementa um valor na memoria por 1
static void
instrucao_inc (Instrucao *instrucao,
               Nes       *nes)
{
  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  valor += 1;

  // atualizar o valor na memoria
  escrever_memoria (nes, endereco, valor);

  // atualizar flags
  cpu_n_escrever (nes->cpu, valor);
  cpu_z_escrever (nes->cpu, valor);
}

//! Incrementa o valor do indice X por 1
static void
instrucao_inx (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  cpu->x += 1;

  // atualizar flags
  cpu_n_escrever (cpu, cpu->x);
  cpu_z_escrever (cpu, cpu->x);
}

//! Incrementa o valor do indice Y por 1
static void
instrucao_iny (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  cpu->y += 1;

  // atualizar flags
  cpu_n_escrever (cpu, cpu->y);
  cpu_z_escrever (cpu, cpu->y);
}

//! Pula o programa para o endereço indicado
static void
instrucao_jmp (Instrucao *instrucao,
               Nes       *nes)
{
  // muda o endereço
  nes->cpu->pc = buscar_endereco (instrucao, nes);
}

//! Chama uma função/subrotina
static void
instrucao_jsr (Instrucao *instrucao,
               Nes       *nes)
{
  // salva o endereço para o qual a função deve retornar
  stack_push_16_bits (nes, nes->cpu->pc - 1);

  // muda o endereço para o da função
  nes->cpu->pc = buscar_endereco (instrucao, nes);
}

//! Carrega um valor da memoria no acumulador
static void
instrucao_lda (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);

  cpu->a = ler_memoria (nes, endereco);

  // atualizar flags
  cpu_n_escrever (cpu, cpu->a);
  cpu_z_escrever (cpu, cpu->a);
}


//! Carrega um valor da memoria no indice X
static void
instrucao_ldx (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);

  cpu->x = ler_memoria (nes, endereco);

  // atualizar flags
  cpu_n_escrever (cpu, cpu->x);
  cpu_z_escrever (cpu, cpu->x);
}

//! Carrega um valor da memoria no acumulador
static void
instrucao_ldy (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;
  uint16_t endereco = buscar_endereco (instrucao, nes);

  cpu->y = ler_memoria (nes, endereco);

  // atualizar flags
  cpu_n_escrever (cpu, cpu->y);
  cpu_z_escrever (cpu, cpu->y);
}

/*!
  Instrução shift para a direita.
  Utiliza a memoria ou o acumulador
 */
static void
instrucao_lsr (Instrucao *instrucao,
               Nes       *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    // checa se a posição 0 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit (nes->cpu->a, 0);

    nes->cpu->a >>= 1;

    // atualizar flags
    cpu_n_escrever (nes->cpu, nes->cpu->a);
    cpu_z_escrever (nes->cpu, nes->cpu->a);
  }
  else
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    uint8_t valor = ler_memoria (nes, endereco);

    // checa se a posição 0 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit (valor, 0);

    valor >>= 1;

    escrever_memoria (nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever (nes->cpu, valor);
    cpu_z_escrever (nes->cpu, valor);
  }
}

//! Não fazer nada
static void
instrucao_nop (Instrucao *instrucao,
               Nes       *nes)
{
}

//! Operanção OR entre um valor na memoria e o acumulador
static void
instrucao_ora (Instrucao *instrucao,
               Nes       *nes)
{
  Cpu *cpu = nes->cpu;

  uint16_t endereco = buscar_endereco (instrucao, nes);
  uint8_t valor = ler_memoria (nes, endereco);

  cpu->a = cpu->a | valor;

  //atualizar flags
  cpu_n_escrever (cpu, cpu->a);
  cpu_z_escrever (cpu, cpu->a);
}
