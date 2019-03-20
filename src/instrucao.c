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
static void instrucao_and (Instrucao *instrucao, Nes *nes)
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

  return instrucoes;
}
