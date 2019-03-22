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
#include <string.h>

#include "cpu.h"
#include "instrucao.h"
#include "memoria.h"
#include "nesbrasa.h"
#include "util.h"

Instrucao* instrucao_new(char          *nome,
                         uint8_t        bytes,
                         int32_t        ciclos,
                         int32_t        ciclos_pag_alterada,
                         InstrucaoModo  modo,
                         InstrucaoFunc  funcao)
{
  Instrucao *instr = malloc(sizeof(Instrucao));
  instr->nome = strdup(nome);
  instr->bytes = bytes;
  instr->ciclos = ciclos;
  instr->modo = modo;
  instr->funcao = funcao;

  return instr;
}

void
instrucao_free(Instrucao *instrucao)
{
  free(instrucao->nome);
  free(instrucao);
}

/*!
  Busca o endereço que vai ser usado pela instrução de
  acordo com o modo de endereçamento da CPU
 */
static uint16_t buscar_endereco(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->pag_alterada = false;

  switch (instrucao->modo)
  {
    case MODO_ENDER_ACM:
      return 0;

    case MODO_ENDER_IMPL:
      return 0;

    case MODO_ENDER_IMED:
      return nes->cpu->pc + 1;

    case MODO_ENDER_P_ZERO:
      return ler_memoria(nes, (nes->cpu->pc + 1)%0xFF);

    case MODO_ENDER_P_ZERO_X:
      return ler_memoria(nes, (nes->cpu->pc + 1 + nes->cpu->x)%0xFF);

    case MODO_ENDER_P_ZERO_Y:
      return ler_memoria(nes, (nes->cpu->pc + 1 + nes->cpu->y)%0xFF);

    case MODO_ENDER_ABS:
      return ler_memoria_16_bits(nes, nes->cpu->pc + 1);

    case MODO_ENDER_ABS_X:
    {
      uint16_t endereco = ler_memoria_16_bits(nes, nes->cpu->pc + 1 + nes->cpu->x);
      nes->cpu->pag_alterada = !comparar_paginas(endereco - nes->cpu->x, endereco);

      return endereco;
    }

    case MODO_ENDER_ABS_Y:
    {
      uint16_t endereco =  ler_memoria_16_bits(nes, nes->cpu->pc + 1 + nes->cpu->y);
      nes->cpu->pag_alterada = !comparar_paginas(endereco - nes->cpu->y, endereco);

      return endereco;
    }

    case MODO_ENDER_IND:
    {
      const uint16_t valor = ler_memoria_16_bits(nes, nes->cpu->pc+1);
      return ler_memoria_16_bits_bug(nes, valor);
    }

    case MODO_ENDER_IND_X:
    {
      const uint16_t valor = ler_memoria(nes, nes->cpu->pc + 1);
      return ler_memoria_16_bits_bug(nes, valor + nes->cpu->x);
    }

    case MODO_ENDER_IND_Y:
    {
      const uint16_t valor = ler_memoria(nes, nes->cpu->pc + 1);
      uint16_t endereco = ler_memoria_16_bits_bug(nes, valor) + nes->cpu->y;
      nes->cpu->pag_alterada = !comparar_paginas(endereco - nes->cpu->y, endereco);

      return endereco;
    }

    case MODO_ENDER_REL:
    {
      const uint16_t valor = ler_memoria(nes, nes->cpu->pc + 1);

      if (valor < 0x80)
        return nes->cpu->pc + 2 + valor;
      else
        return nes->cpu->pc + 2 + valor - 0x100;
    }
  }

  return 0;
}

/*!
  Instrução ADC
  A + M + C -> A, C
 */
static void instrucao_adc(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t c = (nes->cpu->c) ? 1 : 0;

  nes->cpu->a = a + valor + c;

  // atualiza a flag c
  int32_t soma_total = (int32_t)a + (int32_t)valor + (int32_t)c;
  if (soma_total > 0xFF)
    nes->cpu->c = 1;
  else
    nes->cpu->c = 0;

  // checa se houve um overflow/transbordamento e atualiza a flag v
  // solução baseada em: https://stackoverflow.com/a/16861251
  if ((~(a ^ valor)) & (a ^ c) & 0x80)
    nes->cpu->v = 1;
  else
    nes->cpu->v = 0;

  // atualiza as flags z e n
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

/*!
  Instrução AND
  A AND M -> A
 */
static void instrucao_and(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t m = valor;

  nes->cpu->a = a & m;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

/*!
  Instrução shift para a esquerda.
  Utiliza a memoria ou o acumulador
 */
static void instrucao_asl(Instrucao *instrucao, Nes *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    // checa se a posição 7 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit(nes->cpu->a, 7);

    nes->cpu->a <<= 1;

    // atualizar flags
    cpu_n_escrever(nes->cpu, nes->cpu->a);
    cpu_z_escrever(nes->cpu, nes->cpu->a);
  }
  else
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    uint8_t valor = ler_memoria(nes, endereco);

    // checa se a posição 7 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit(valor, 7);

    valor <<= 1;

    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(nes->cpu, valor);
    cpu_z_escrever(nes->cpu, valor);
  }
}

//! Pula para o endereço indicado se a flag 'c' não estiver ativa
static void instrucao_bcc(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->c == false)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'c' estiver ativa
static void instrucao_bcs(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->c == true)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'z' estiver ativa
static void instrucao_beq(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->z == true)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

/*! BIT
  Busca um byte na memoria e depois salva a posição 7 do byte em 'n'
  e a posição 6 do byte em 'v'.
  A flag 'z' tambem é alterada sendo calculada com 'a' AND valor
 */
static void instrucao_bit(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  nes->cpu->n = buscar_bit(valor, 7);
  nes->cpu->v = buscar_bit(valor, 6);
  nes->cpu->z = valor & nes->cpu->a;
}

//! Pula para o endereço indicado se a flag 'n' estiver ativa
static void instrucao_bmi(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->n == true)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'z' não estiver ativa
static void instrucao_bne(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->z == true)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'n' não estiver ativa
static void instrucao_bpl(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->n == false)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Instrução BRK
static void instrucao_brk(Instrucao *instrucao, Nes *nes)
{
  stack_empurrar_16_bits(nes, nes->cpu->pc);
  stack_empurrar(nes, cpu_estado_ler(nes->cpu));

  nes->cpu->b = 1;
  nes->cpu->pc = ler_memoria_16_bits(nes, 0xFFFE);
}

//! Pula para o endereço indicado se a flag 'v' não estiver ativa
static void instrucao_bvc (Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->n == false)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'v' estiver ativa
static void instrucao_bvs(Instrucao *instrucao, Nes *nes)
{
  if (nes->cpu->n == true)
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu->pc = endereco;
  }
}

//! Limpa a flag 'c'
static void instrucao_clc(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->c = false;
}

//! Limpa a flag 'd'
static void instrucao_cld(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->d = false;
}

//! Limpa a flag 'i'
static void instrucao_cli(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->i = false;
}

//! Limpa a flag 'v'
static void instrucao_clv(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->v = false;
}

//! Compara o acumulador com um valor
static void instrucao_cmp(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  if (nes->cpu->a >= valor)
    nes->cpu->c = true;
  else
    nes->cpu->c = false;

  uint8_t resultado = nes->cpu->a - valor;

  // atualizar flags
  cpu_n_escrever(nes->cpu, resultado);
  cpu_z_escrever(nes->cpu, resultado);
}

//! Compara o indice X com um valor
static void instrucao_cpx(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  if (nes->cpu->x >= valor)
    nes->cpu->c = true;
  else
    nes->cpu->c = false;

  uint8_t resultado = nes->cpu->x - valor;

  // atualizar flags
  cpu_n_escrever(nes->cpu, resultado);
  cpu_z_escrever(nes->cpu, resultado);
}

//! Compara o indice Y com um valor
static void instrucao_cpy(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  if (nes->cpu->y >= valor)
    nes->cpu->c = true;
  else
    nes->cpu->c = false;

  uint8_t resultado = nes->cpu->y - valor;
  cpu_n_escrever (nes->cpu, resultado);
  cpu_z_escrever (nes->cpu, resultado);
}

//! Diminui um valor na memoria por 1
static void instrucao_dec(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  valor -= 1;

  // atualizar o valor na memoria
  escrever_memoria(nes, endereco, valor);

  // atualizar flags
  cpu_n_escrever(nes->cpu, valor);
  cpu_z_escrever(nes->cpu, valor);
}

//! Diminui o valor do indice X por 1
static void instrucao_dex(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->x -= 1;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->x);
  cpu_z_escrever(nes->cpu, nes->cpu->x);
}

//! Diminui o valor do indice Y por 1
static void instrucao_dey(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->y -= 1;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->y);
  cpu_z_escrever(nes->cpu, nes->cpu->y);
}

//! OR exclusivo de um valor na memoria com o acumulador
static void instrucao_eor(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  nes->cpu->a = nes->cpu->a ^ valor;

  //atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

//! Incrementa um valor na memoria por 1
static void instrucao_inc(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  valor += 1;

  // atualizar o valor na memoria
  escrever_memoria(nes, endereco, valor);

  // atualizar flags
  cpu_n_escrever(nes->cpu, valor);
  cpu_z_escrever(nes->cpu, valor);
}

//! Incrementa o valor do indice X por 1
static void instrucao_inx(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->x += 1;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->x);
  cpu_z_escrever(nes->cpu, nes->cpu->x);
}

//! Incrementa o valor do indice Y por 1
static void instrucao_iny(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->y += 1;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->y);
  cpu_z_escrever(nes->cpu, nes->cpu->y);
}

//! Pula o programa para o endereço indicado
static void instrucao_jmp(Instrucao *instrucao, Nes *nes)
{
  // muda o endereço
  nes->cpu->pc = buscar_endereco(instrucao, nes);
}

//! Chama uma função/subrotina
static void instrucao_jsr(Instrucao *instrucao, Nes *nes)
{
  // salva o endereço da próxima instrução subtraído por 1 na stack,
  // o endereço será usado para retornar da função quando o opcode 'rts'
  // for usado
  stack_empurrar_16_bits(nes, nes->cpu->pc - 1);

  // muda o endereço para o da função
  nes->cpu->pc = buscar_endereco(instrucao, nes);
}

//! Carrega um valor da memoria no acumulador
static void instrucao_lda(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);

  nes->cpu->a = ler_memoria(nes, endereco);

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}


//! Carrega um valor da memoria no indice X
static void instrucao_ldx(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);

  nes->cpu->x = ler_memoria(nes, endereco);

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->x);
  cpu_z_escrever(nes->cpu, nes->cpu->x);
}

//! Carrega um valor da memoria no acumulador
static void instrucao_ldy(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);

  nes->cpu->y = ler_memoria(nes, endereco);

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->y);
  cpu_z_escrever(nes->cpu, nes->cpu->y);
}

/*!
  Instrução shift para a direita.
  Utiliza a memoria ou o acumulador
 */
static void instrucao_lsr(Instrucao *instrucao, Nes *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    // checa se a posição 0 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit(nes->cpu->a, 0);

    nes->cpu->a >>= 1;

    // atualizar flags
    cpu_n_escrever(nes->cpu, nes->cpu->a);
    cpu_z_escrever(nes->cpu, nes->cpu->a);
  }
  else
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    uint8_t valor = ler_memoria(nes, endereco);

    // checa se a posição 0 do byte é '1' ou '0'
    nes->cpu->c = buscar_bit(valor, 0);

    valor >>= 1;

    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(nes->cpu, valor);
    cpu_z_escrever(nes->cpu, valor);
  }
}

//! Não fazer nada
static void instrucao_nop(Instrucao *instrucao, Nes *nes)
{
}

//! Operanção OR entre um valor na memoria e o acumulador
static void instrucao_ora(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  nes->cpu->a = nes->cpu->a | valor;

  //atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

//! Empurra o valor do acumulador na stack
static void instrucao_pha(Instrucao *instrucao, Nes *nes)
{
  stack_empurrar(nes, nes->cpu->a);
}

//! Empurra o valor do estado do processador na stack
static void instrucao_php(Instrucao *instrucao, Nes *nes)
{
  const uint8_t estado = cpu_estado_ler(nes->cpu);
  stack_empurrar(nes, estado);
}

//! Puxa um valor da stack e salva esse valor no acumulador
static void instrucao_pla(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->a = stack_puxar(nes);

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

//! Puxa um valor da stack e salva esse valor no estado do processador
static void instrucao_plp(Instrucao *instrucao, Nes *nes)
{
  const uint8_t estado = stack_puxar(nes);
  cpu_estado_escrever(nes->cpu, estado);
}

//! Gira um valor pra a esquerda
static void instrucao_rol(Instrucao *instrucao, Nes *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    bool carregar = nes->cpu->c;
    nes->cpu->c = buscar_bit(nes->cpu->a, 7);
    nes->cpu->a <<= 1;
    nes->cpu->a = nes->cpu->a | ((carregar) ? 1 : 0);

    // atualizar flags
    cpu_n_escrever(nes->cpu, nes->cpu->a);
    cpu_z_escrever(nes->cpu, nes->cpu->a);
  }
  else
  {
    uint16_t endereco = buscar_endereco(instrucao, nes);
    uint8_t valor = ler_memoria(nes, endereco);

    bool carregar = nes->cpu->c;
    nes->cpu->c = buscar_bit(valor, 7);
    valor <<= 1;
    valor = valor | ((carregar) ? 1 : 0);

    // atualizar o valor na memoria
    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(nes->cpu, nes->cpu->a);
    cpu_z_escrever(nes->cpu, nes->cpu->a);
  }
}

//! Gira um valor pra a direita
static void instrucao_ror(Instrucao *instrucao, Nes *nes)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    bool carregar = nes->cpu->c;
    nes->cpu->c = buscar_bit(nes->cpu->a, 0);
    nes->cpu->a >>= 1;
    nes->cpu->a = nes->cpu->a | ((carregar) ? 0b10000000 : 0);

    // atualizar flags
    cpu_n_escrever(nes->cpu, nes->cpu->a);
    cpu_z_escrever(nes->cpu, nes->cpu->a);
  }
  else
  {
    uint16_t endereco = buscar_endereco (instrucao, nes);
    uint8_t valor = ler_memoria(nes, endereco);

    bool carregar = nes->cpu->c;
    nes->cpu->c = buscar_bit(nes->cpu->a, 0);
    valor >>= 1;
    valor = valor | ((carregar) ? 0b10000000 : 0);

    // atualizar o valor na memoria
    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(nes->cpu, nes->cpu->a);
    cpu_z_escrever(nes->cpu, nes->cpu->a);
  }
}

//! Retorna de uma interupção
static void instrucao_rti(Instrucao *instrucao, Nes *nes)
{
  const uint8_t estado = stack_puxar(nes);
  cpu_estado_escrever(nes->cpu, estado);

  nes->cpu->pc = stack_puxar_16_bits(nes);
}

//! Retorna de uma função/sub-rotina
static void instrucao_rts(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->pc = stack_puxar_16_bits(nes) + 1;
}

//! Subtrai um valor da memoria usando o acumulador
static void instrucao_sbc(Instrucao *instrucao, Nes *nes)
{
 uint16_t endereco = buscar_endereco(instrucao, nes);
  uint8_t valor = ler_memoria(nes, endereco);

  const uint8_t a = nes->cpu->a;
  const uint8_t c = (nes->cpu->c) ? 1 : 0;

  nes->cpu->a = a - valor - 1 - c;

  // atualiza a flag c
  int32_t subtracao_total = (int32_t)a - (int32_t)valor - 1 - (int32_t)c;
  if (subtracao_total >= 0)
    nes->cpu->c = 1;
  else
    nes->cpu->c = 0;

  // checa se houve um overflow/transbordamento e atualiza a flag v
  // solução baseada em: https://stackoverflow.com/a/16861251
  if ((~(a ^ (valor*-1 - 1))) & (a ^ c) & 0x80)
    nes->cpu->v = 1;
  else
    nes->cpu->v = 0;

  // atualiza as flags z e n
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

//! Ativa a flag 'c'
static void instrucao_sec(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->c = true;
}

//! Ativa a flag 'd'
static void instrucao_sed(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->d = true;
}

//! Ativa a flag 'i'
static void instrucao_sei(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->i = true;
}

//! Guarda o valor do acumulador na memoria
static void instrucao_sta(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  escrever_memoria(nes, endereco, nes->cpu->a);
}

//! Guarda o valor do registrador 'x' na memoria
static void instrucao_stx(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  escrever_memoria(nes, endereco, nes->cpu->x);
}

//! Guarda o valor do registrador 'y' na memoria
static void instrucao_sty(Instrucao *instrucao, Nes *nes)
{
  uint16_t endereco = buscar_endereco(instrucao, nes);
  escrever_memoria(nes, endereco, nes->cpu->y);
}

//! Atribui o valor do acumulador ao registrador 'x'
static void instrucao_tax(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->x = nes->cpu->a;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->x);
  cpu_z_escrever(nes->cpu, nes->cpu->x);
}

//! Atribui o valor do acumulador ao registrador 'y'
static void instrucao_tay(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->y = nes->cpu->a;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->y);
  cpu_z_escrever(nes->cpu, nes->cpu->y);
}

//! Atribui o valor do ponteiro da stack ao registrador 'x'
static void instrucao_tsx(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->x = nes->cpu->sp;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->x);
  cpu_z_escrever(nes->cpu, nes->cpu->x);
}

//! Atribui o valor do registrador 'x' ao acumulador
static void instrucao_txa(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->a = nes->cpu->x;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

//! Atribui o valor do registrador 'x' ao ponteiro da stack
static void instrucao_txs(Instrucao *instrucao, Nes *nes)
{
  nes->cpu->sp = nes->cpu->x;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->sp);
  cpu_z_escrever(nes->cpu, nes->cpu->sp);
}

//! Atribui o valor do registrador 'y' ao acumulador
static void instrucao_tya(Instrucao *isntrucao, Nes *nes)
{
  nes->cpu->a = nes->cpu->y;

  // atualizar flags
  cpu_n_escrever(nes->cpu, nes->cpu->a);
  cpu_z_escrever(nes->cpu, nes->cpu->a);
}

Instrucao** carregar_instrucoes(void)
{
  // cria um array com 0x100 (256 em decimal) ponteiros para instruções
  Instrucao **instrucoes = malloc(sizeof(Instrucao*) * 0x100);
  for (int i = 0; i < 0x100; i++)
  {
    instrucoes[i] = NULL;
  }

  // modos da instrução ADC
  instrucoes[0x69] = instrucao_new("ADC", 2, 2, 0, MODO_ENDER_IMED, instrucao_adc);
  instrucoes[0x65] = instrucao_new("ADC", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_adc);
  instrucoes[0x75] = instrucao_new("ADC", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_adc);
  instrucoes[0x6D] = instrucao_new("ADC", 3, 4, 0, MODO_ENDER_ABS, instrucao_adc);
  instrucoes[0x7D] = instrucao_new("ADC", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_adc);
  instrucoes[0x79] = instrucao_new("ADC", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_adc);
  instrucoes[0x61] = instrucao_new("ADC", 2, 6, 0, MODO_ENDER_IND_X, instrucao_adc);
  instrucoes[0x71] = instrucao_new("ADC", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_adc);

  // modos da instrução AND
  instrucoes[0x29] = instrucao_new("AND", 2, 2, 0, MODO_ENDER_IMED, instrucao_and);
  instrucoes[0x25] = instrucao_new("AND", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_and);
  instrucoes[0x35] = instrucao_new("AND", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_and);
  instrucoes[0x2D] = instrucao_new("AND", 3, 4, 0, MODO_ENDER_ABS, instrucao_and);
  instrucoes[0x3D] = instrucao_new("AND", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_and);
  instrucoes[0x39] = instrucao_new("AND", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_and);
  instrucoes[0x21] = instrucao_new("AND", 2, 6, 0, MODO_ENDER_IND_X, instrucao_and);
  instrucoes[0x21] = instrucao_new("AND", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_and);

  // modos da instrução ASL
  instrucoes[0x0A] = instrucao_new("ASL", 1, 2, 0, MODO_ENDER_ACM, instrucao_asl);
  instrucoes[0x06] = instrucao_new("ASL", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_asl);
  instrucoes[0x16] = instrucao_new("ASL", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_asl);
  instrucoes[0x0E] = instrucao_new("ASL", 3, 6, 0, MODO_ENDER_ABS, instrucao_asl);
  instrucoes[0x1E] = instrucao_new("ASL", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_asl);

  // modos da instrução BCC
  instrucoes[0x90] = instrucao_new("BCC", 2, 2, 0, MODO_ENDER_REL, instrucao_bcc);

  // modos da instrução BCS
  instrucoes[0xB0] = instrucao_new("BCS", 2, 2, 0, MODO_ENDER_REL, instrucao_bcs);

  // modos da instrução BEQ
  instrucoes[0xF0] = instrucao_new("BEQ", 2, 2, 0, MODO_ENDER_REL, instrucao_beq);

  // modos da instrução BIT
  instrucoes[0x24] = instrucao_new("BIT", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_bit);
  instrucoes[0x2C] = instrucao_new("BIT", 3, 4, 0, MODO_ENDER_ABS, instrucao_bit);

  // modos da instrução BMI
  instrucoes[0x30] = instrucao_new("BIM", 2, 2, 0, MODO_ENDER_REL, instrucao_bmi);

  // modos da instrução BNE
  instrucoes[0xD0] = instrucao_new("BNE", 2, 2, 0, MODO_ENDER_REL, instrucao_bne);

  // modos da instrução BPL
  instrucoes[0x10] = instrucao_new("BPL", 2, 2, 0, MODO_ENDER_REL, instrucao_bpl);

  // modos da instrução BRK
  instrucoes[0x00] = instrucao_new("BRK", 1, 7, 0, MODO_ENDER_IMPL, instrucao_brk);

  // modos da instrução BVC
  instrucoes[0x50] = instrucao_new("BVC", 2, 2, 0, MODO_ENDER_REL, instrucao_bvc);

  // modos da instrução BVS
  instrucoes[0x70] = instrucao_new("BVS", 2, 2, 0, MODO_ENDER_REL, instrucao_bvs);

  // modos da instrução CLC
  instrucoes[0x18] = instrucao_new("CLC", 1, 2, 0, MODO_ENDER_IMPL, instrucao_clc);

  // modos da instrução CLD
  instrucoes[0xD8] = instrucao_new("CLD", 1, 2, 0, MODO_ENDER_IMPL, instrucao_cld);

  // modos da instrução CLI
  instrucoes[0x58] = instrucao_new("CLI", 1, 2, 0, MODO_ENDER_IMPL, instrucao_cli);

  // modos da instrução CLV
  instrucoes[0xB8] = instrucao_new("CLV", 1, 2, 0, MODO_ENDER_IMPL, instrucao_clv);

  // modos da instrução CMP
  instrucoes[0xC9] = instrucao_new("CMP", 2, 2, 0, MODO_ENDER_IMED, instrucao_cmp);
  instrucoes[0xC5] = instrucao_new("CMP", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_cmp);
  instrucoes[0xD5] = instrucao_new("CMP", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_cmp);
  instrucoes[0xCD] = instrucao_new("CMP", 3, 4, 0, MODO_ENDER_ABS, instrucao_cmp);
  instrucoes[0xDD] = instrucao_new("CMP", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_cmp);
  instrucoes[0xD9] = instrucao_new("CMP", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_cmp);
  instrucoes[0xC1] = instrucao_new("CMP", 2, 6, 0, MODO_ENDER_IND_X, instrucao_cmp);
  instrucoes[0xD1] = instrucao_new("CMP", 3, 5, 1, MODO_ENDER_IND_Y, instrucao_cmp);

  // modos da instrução CPX
  instrucoes[0xE0] = instrucao_new("CPX", 2, 2, 0, MODO_ENDER_IMED, instrucao_cpx);
  instrucoes[0xE4] = instrucao_new("CPX", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_cpx);
  instrucoes[0xEC] = instrucao_new("CPX", 3, 4, 0, MODO_ENDER_ABS, instrucao_cpx);

   // modos da instrução CPY
  instrucoes[0xC0] = instrucao_new("CPY", 2, 2, 0, MODO_ENDER_IMED, instrucao_cpy);
  instrucoes[0xC4] = instrucao_new("CPY", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_cpy);
  instrucoes[0xCC] = instrucao_new("CPY", 3, 4, 0, MODO_ENDER_ABS, instrucao_cpy);

  // modos da instrução DEC
  instrucoes[0xC6] = instrucao_new("DEC", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_dec);
  instrucoes[0xD6] = instrucao_new("DEC", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_dec);
  instrucoes[0xCE] = instrucao_new("DEC", 3, 3, 0, MODO_ENDER_ABS, instrucao_dec);
  instrucoes[0xDE] = instrucao_new("DEC", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_dec);

  // modos da instrução DEX
  instrucoes[0xCA] = instrucao_new("DEX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_dex);

  // modos da instrução DEY
  instrucoes[0x88] = instrucao_new("DEX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_dey);

  // modos da instrução EOR
  instrucoes[0x49] = instrucao_new("EOR", 2, 2, 0, MODO_ENDER_IMED, instrucao_eor);
  instrucoes[0x45] = instrucao_new("EOR", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_eor);
  instrucoes[0x55] = instrucao_new("EOR", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_eor);
  instrucoes[0x4D] = instrucao_new("EOR", 3, 4, 0, MODO_ENDER_ABS, instrucao_eor);
  instrucoes[0x5D] = instrucao_new("EOR", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_eor);
  instrucoes[0x59] = instrucao_new("EOR", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_eor);
  instrucoes[0x41] = instrucao_new("EOR", 2, 6, 0, MODO_ENDER_IND_X, instrucao_eor);
  instrucoes[0x51] = instrucao_new("EOR", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_eor);

  // modos da instrução INC
  instrucoes[0xE6] = instrucao_new("INC", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_inc);
  instrucoes[0xF6] = instrucao_new("INC", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_inc);
  instrucoes[0xEE] = instrucao_new("INC", 3, 6, 0, MODO_ENDER_ABS, instrucao_inc);
  instrucoes[0xFE] = instrucao_new("INC", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_inc);

  // modos da instrução INX
  instrucoes[0xE8] = instrucao_new("INX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_inx);

  // modos da instrução INY
  instrucoes[0xC8] = instrucao_new("INY", 1, 2, 0, MODO_ENDER_IMPL, instrucao_iny);

  // modos da instrução JMP
  instrucoes[0x4C] = instrucao_new("JMP", 3, 3, 0, MODO_ENDER_ABS, instrucao_jmp);
  instrucoes[0x6C] = instrucao_new("JMP", 3, 5, 0, MODO_ENDER_IND, instrucao_jmp);

  // modos da instrução JSR
  instrucoes[0x20] = instrucao_new("JSR", 3, 6, 0, MODO_ENDER_ABS, instrucao_jsr);

  // modos da instrução LDA
  instrucoes[0xA9] = instrucao_new("LDA", 2, 2, 0, MODO_ENDER_IMED, instrucao_lda);
  instrucoes[0xA5] = instrucao_new("LDA", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_lda);
  instrucoes[0xB5] = instrucao_new("LDA", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_lda);
  instrucoes[0xAD] = instrucao_new("LDA", 3, 4, 0, MODO_ENDER_ABS, instrucao_lda);
  instrucoes[0xBD] = instrucao_new("LDA", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_lda);
  instrucoes[0xB9] = instrucao_new("LDA", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_lda);
  instrucoes[0xA1] = instrucao_new("LDA", 2, 6, 0, MODO_ENDER_IND_X, instrucao_lda);
  instrucoes[0xB1] = instrucao_new("LDA", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_lda);

  // modos da instrução LDX
  instrucoes[0xA2] = instrucao_new("LDX", 2, 2, 0, MODO_ENDER_IMED, instrucao_ldx);
  instrucoes[0xA6] = instrucao_new("LDX", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_ldx);
  instrucoes[0xB6] = instrucao_new("LDX", 2, 4, 0, MODO_ENDER_P_ZERO_Y, instrucao_ldx);
  instrucoes[0xAE] = instrucao_new("LDX", 3, 4, 0, MODO_ENDER_ABS, instrucao_ldx);
  instrucoes[0xBE] = instrucao_new("LDX", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_ldx);

  // modos da instrução LDY
  instrucoes[0xA0] = instrucao_new("LDY", 2, 2, 0, MODO_ENDER_IMED, instrucao_ldx);
  instrucoes[0xA4] = instrucao_new("LDY", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_ldx);
  instrucoes[0xB4] = instrucao_new("LDY", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_ldx);
  instrucoes[0xAC] = instrucao_new("LDY", 3, 4, 0, MODO_ENDER_ABS, instrucao_ldx);
  instrucoes[0xBC] = instrucao_new("LDY", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_ldx);

  // modos da instrução LSR
  instrucoes[0x4A] = instrucao_new("LSR", 1, 2, 0, MODO_ENDER_ACM, instrucao_lsr);
  instrucoes[0x46] = instrucao_new("LSR", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_lsr);
  instrucoes[0x56] = instrucao_new("LSR", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_lsr);
  instrucoes[0x4E] = instrucao_new("LSR", 3, 6, 0, MODO_ENDER_ABS, instrucao_lsr);
  instrucoes[0x5E] = instrucao_new("LSR", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_lsr);

  // modos da instrução NOP
  instrucoes[0xEA] = instrucao_new("NOP", 1, 2, 0, MODO_ENDER_IMPL, instrucao_nop);

  // modos da instrução ORA
  instrucoes[0x09] = instrucao_new("ORA", 2, 2, 0, MODO_ENDER_IMED, instrucao_ora);
  instrucoes[0x05] = instrucao_new("ORA", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_ora);
  instrucoes[0x15] = instrucao_new("ORA", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_ora);
  instrucoes[0x0D] = instrucao_new("ORA", 3, 4, 0, MODO_ENDER_ABS, instrucao_ora);
  instrucoes[0x1D] = instrucao_new("ORA", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_ora);
  instrucoes[0x19] = instrucao_new("ORA", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_ora);
  instrucoes[0x01] = instrucao_new("ORA", 2, 6, 0, MODO_ENDER_IND_X, instrucao_ora);
  instrucoes[0x11] = instrucao_new("ORA", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_ora);

  // modos da instrução PHA
  instrucoes[0x48] = instrucao_new("PHA", 1, 3, 0, MODO_ENDER_IMPL, instrucao_pha);

  // modos da instrução PHP
  instrucoes[0x08] = instrucao_new("PHP", 1, 3, 0, MODO_ENDER_IMPL, instrucao_php);

  // modos da instrução PLA
  instrucoes[0x68] = instrucao_new("PLA", 1, 4, 0, MODO_ENDER_IMPL, instrucao_pla);

  // modos da instrução PLP
  instrucoes[0x28] = instrucao_new("PLP", 1, 4, 0, MODO_ENDER_IMPL, instrucao_plp);

  // modos da instrução ROL
  instrucoes[0x2A] = instrucao_new("ROL", 1, 2, 0, MODO_ENDER_ACM, instrucao_rol);
  instrucoes[0x26] = instrucao_new("ROL", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_rol);
  instrucoes[0x36] = instrucao_new("ROL", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_rol);
  instrucoes[0x2E] = instrucao_new("ROL", 3, 6, 0, MODO_ENDER_ABS, instrucao_rol);
  instrucoes[0x3E] = instrucao_new("ROL", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_rol);

  // modos da instrução ROR
  instrucoes[0x6A] = instrucao_new("ROR", 1, 2, 0, MODO_ENDER_ACM, instrucao_ror);
  instrucoes[0x66] = instrucao_new("ROR", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_ror);
  instrucoes[0x76] = instrucao_new("ROR", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_ror);
  instrucoes[0x6E] = instrucao_new("ROR", 3, 6, 0, MODO_ENDER_ABS, instrucao_ror);
  instrucoes[0x7E] = instrucao_new("ROR", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_ror);

  // modos da instrução RTI
  instrucoes[0x40] = instrucao_new("RTI", 1, 6, 0, MODO_ENDER_IMPL, instrucao_rti);

  // modos da instrução RTS
  instrucoes[0x60] = instrucao_new("RTS", 1, 6, 0, MODO_ENDER_IMPL, instrucao_rts);

  // modos da instrução SBC
  instrucoes[0xE9] = instrucao_new("SBC", 2, 2, 0, MODO_ENDER_IMED, instrucao_sbc);
  instrucoes[0xE5] = instrucao_new("SBC", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_sbc);
  instrucoes[0xF5] = instrucao_new("SBC", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_sbc);
  instrucoes[0xED] = instrucao_new("SBC", 3, 4, 0, MODO_ENDER_ABS, instrucao_sbc);
  instrucoes[0xFD] = instrucao_new("SBC", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_sbc);
  instrucoes[0xF9] = instrucao_new("SBC", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_sbc);
  instrucoes[0xE1] = instrucao_new("SBC", 2, 6, 0, MODO_ENDER_IND_X, instrucao_sbc);
  instrucoes[0xF1] = instrucao_new("SBC", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_sbc);

  // modos da instrução SEC
  instrucoes[0x38] = instrucao_new("SEC", 1, 2, 0, MODO_ENDER_IMPL, instrucao_sec);

  // modos da instrução SED
  instrucoes[0xF8] = instrucao_new("SED", 1, 2, 0, MODO_ENDER_IMPL, instrucao_sed);

  // modos da instrução SEI
  instrucoes[0x78] = instrucao_new("SEI", 1, 2, 0, MODO_ENDER_IMPL, instrucao_sei);

  // modos da instrução STA
  instrucoes[0x85] = instrucao_new("STA", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_sta);
  instrucoes[0x95] = instrucao_new("STA", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_sta);
  instrucoes[0x8D] = instrucao_new("STA", 3, 4, 0, MODO_ENDER_ABS, instrucao_sta);
  instrucoes[0x9D] = instrucao_new("STA", 3, 5, 0, MODO_ENDER_ABS_X, instrucao_sta);
  instrucoes[0x99] = instrucao_new("STA", 3, 5, 0, MODO_ENDER_ABS_Y, instrucao_sta);
  instrucoes[0x81] = instrucao_new("STA", 2, 6, 0, MODO_ENDER_IND_X, instrucao_sta);
  instrucoes[0x91] = instrucao_new("STA", 2, 6, 0, MODO_ENDER_IND_Y, instrucao_sta);

  // modos da instrução STX
  instrucoes[0x86] = instrucao_new("STX", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_stx);
  instrucoes[0x96] = instrucao_new("STX", 2, 4, 0, MODO_ENDER_P_ZERO_Y, instrucao_stx);
  instrucoes[0x8E] = instrucao_new("STX", 3, 4, 0, MODO_ENDER_ABS, instrucao_stx);

  // modos da instrução STY
  instrucoes[0x84] = instrucao_new("STY", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_sty);
  instrucoes[0x94] = instrucao_new("STY", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_sty);
  instrucoes[0x8C] = instrucao_new("STY", 3, 4, 0, MODO_ENDER_ABS, instrucao_sty);


  return instrucoes;
}
