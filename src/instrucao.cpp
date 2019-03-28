/* instrucao->c
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

#include <cstdlib>
#include <cstring>

#include "nesbrasa.hpp"
#include "cpu.hpp"
#include "instrucao->hpp"
#include "memoria.hpp"
#include "util.hpp"

Instrucao::Instrucao(
  char *nome,
  uint8_t bytes,
  int32_t ciclos,
  int32_t ciclos_pag_alterada,
  InstrucaoModo  modo,
  function<void(Instrucao*, Nes*, uint16_t)> funcao
)
{
  this->nome = strdup(nome);
  this->bytes = bytes;
  this->ciclos = ciclos;
  this->modo = modo;
  this->funcao = funcao;
}

uint16_t Instrucao::buscar_endereco(Nes* nes)
{
  nes->cpu.pag_alterada = false;

  switch (this->modo)
  {
    case MODO_ENDER_ACM:
      return 0;

    case MODO_ENDER_IMPL:
      return 0;

    case MODO_ENDER_IMED:
      return nes->cpu.pc + 1;

    case MODO_ENDER_P_ZERO:
      return ler_memoria(nes, (nes->cpu.pc + 1)%0xFF);

    case MODO_ENDER_P_ZERO_X:
      return ler_memoria(nes, (nes->cpu.pc + 1 + nes->cpu.x)%0xFF);

    case MODO_ENDER_P_ZERO_Y:
      return ler_memoria(nes, (nes->cpu.pc + 1 + nes->cpu.y)%0xFF);

    case MODO_ENDER_ABS:
      return ler_memoria_16_bits(nes, nes->cpu.pc + 1);

    case MODO_ENDER_ABS_X:
    {
      uint16_t endereco = ler_memoria_16_bits(nes, nes->cpu.pc + 1 + ne->cpu.x);
      nes->cpu.pag_alterada = !comparar_paginas(endereco - nes->cpu.x, endereco);

      return endereco;
    }

    case MODO_ENDER_ABS_Y:
    {
      uint16_t endereco =  ler_memoria_16_bits(nes, nes->cpu.pc + 1 + nes->cpu.y);
      nes->cpu.pag_alterada = !comparar_paginas(endereco - nes->cpu.y, endereco);

      return endereco;
    }

    case MODO_ENDER_IND:
    {
      const uint16_t valor = ler_memoria_16_bits(nes, nes->cpu.pc+1);
      return ler_memoria_16_bits_bug(nes, valor);
    }

    case MODO_ENDER_IND_X:
    {
      const uint16_t valor = ler_memoria(nes, nes->cpu.pc + 1);
      return ler_memoria_16_bits_bug(nes, valor + nes->cpu.x);
    }

    case MODO_ENDER_IND_Y:
    {
      const uint16_t valor = ler_memoria(nes, nes->cpu.pc + 1);
      uint16_t endereco = ler_memoria_16_bits_bug(nes, valor) + nes->cpu.y;
      nes->cpu.pag_alterada = !comparar_paginas(endereco - nes->cpu.y, endereco);

      return endereco;
    }

    case MODO_ENDER_REL:
    {
      const uint16_t valor = ler_memoria(nes, nes->cpu.pc + 1);

      if (valor < 0x80)
        return nes->cpu.pc + 2 + valor;
      else
        return nes->cpu.pc + 2 + valor - 0x100;
    }
  }

  return 0;
}

/*!
  Instrução ADC
  A + M + C -> A, C
 */
static void instrucao_adc(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  const uint8_t a = nes->cpu.a;
  const uint8_t c = (nes->cpu.c) ? 1 : 0;

  nes->cpu.a = a + valor + c;

  // atualiza a flag c
  int32_t soma_total = (int32_t)a + (int32_t)valor + (int32_t)c;
  if (soma_total > 0xFF)
    nes->cpu.c = 1;
  else
    nes->cpu.c = 0;

  // checa se houve um overflow/transbordamento e atualiza a flag v
  // solução baseada em: https://stackoverflow.com/a/16861251
  if ((~(a ^ valor)) & (a ^ c) & 0x80)
    nes->cpu.v = 1;
  else
    nes->cpu.v = 0;

  // atualiza as flags z e n
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

/*!
  Instrução AND
  A AND M -> A
 */
static void instrucao_and(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  const uint8_t a = nes->cpu.a;
  const uint8_t m = valor;

  nes->cpu.a = a & m;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

/*!
  Instrução shift para a esquerda.
  Utiliza a memoria ou o acumulador
 */
static void instrucao_asl(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    // checa se a posição 7 do byte é '1' ou '0'
    nes->cpu.c = buscar_bit(nes->cpu.a, 7);

    nes->cpu.a <<= 1;

    // atualizar flags
    cpu_n_escrever(&nes->cpu, nes->cpu.a);
    cpu_z_escrever(&nes->cpu, nes->cpu.a);
  }
  else
  {
    uint8_t valor = ler_memoria(nes, endereco);

    // checa se a posição 7 do byte é '1' ou '0'
    nes->cpu.c = buscar_bit(valor, 7);

    valor <<= 1;

    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(&nes->cpu, valor);
    cpu_z_escrever(&nes->cpu, valor);
  }
}

//! Pula para o endereço indicado se a flag 'c' não estiver ativa
static void instrucao_bcc(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.c == false)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'c' estiver ativa
static void instrucao_bcs(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.c == true)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'z' estiver ativa
static void instrucao_beq(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.z == true)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

/*! BIT
  Busca um byte na memoria e depois salva a posição 7 do byte em 'n'
  e a posição 6 do byte em 'v'.
  A flag 'z' tambem é alterada sendo calculada com 'a' AND valor
 */
static void instrucao_bit(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  nes->cpu.n = buscar_bit(valor, 7);
  nes->cpu.v = buscar_bit(valor, 6);
  nes->cpu.z = valor & nes->cpu.a;
}

//! Pula para o endereço indicado se a flag 'n' estiver ativa
static void instrucao_bmi(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.n == true)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'z' não estiver ativa
static void instrucao_bne(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.z == true)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'n' não estiver ativa
static void instrucao_bpl(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.n == false)
  {
    cpu_branch_somar_ciclos(nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Instrução BRK
static void instrucao_brk(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  stack_empurrar_16_bits(&nes, nes->cpu.pc);
  stack_empurrar(&nes, cpu_estado_ler(&nes->cpu));

  nes->cpu.b = 1;
  nes->cpu.pc = ler_memoria_16_bits(nes, 0xFFFE);
}

//! Pula para o endereço indicado se a flag 'v' não estiver ativa
static void instrucao_bvc (Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.n == false)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Pula para o endereço indicado se a flag 'v' estiver ativa
static void instrucao_bvs(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (nes->cpu.n == true)
  {
    cpu_branch_somar_ciclos(&nes->cpu, endereco);
    nes->cpu.pc = endereco;
  }
}

//! Limpa a flag 'c'
static void instrucao_clc(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.c = false;
}

//! Limpa a flag 'd'
static void instrucao_cld(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.d = false;
}

//! Limpa a flag 'i'
static void instrucao_cli(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.i = false;
}

//! Limpa a flag 'v'
static void instrucao_clv(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.v = false;
}

//! Compara o acumulador com um valor
static void instrucao_cmp(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  if (nes->cpu.a >= valor)
    nes->cpu.c = true;
  else
    nes->cpu.c = false;

  uint8_t resultado = nes->cpu.a - valor;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, resultado);
  cpu_z_escrever(&nes->cpu, resultado);
}

//! Compara o indice X com um valor
static void instrucao_cpx(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  if (nes->cpu.x >= valor)
    nes->cpu.c = true;
  else
    nes->cpu.c = false;

  uint8_t resultado = nes->cpu.x - valor;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, resultado);
  cpu_z_escrever(&nes->cpu, resultado);
}

//! Compara o indice Y com um valor
static void instrucao_cpy(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  if (nes->cpu.y >= valor)
    nes->cpu.c = true;
  else
    nes->cpu.c = false;

  uint8_t resultado = nes->cpu.y - valor;
  cpu_n_escrever (&nes->cpu, resultado);
  cpu_z_escrever (&nes->cpu, resultado);
}

//! Diminui um valor na memoria por 1
static void instrucao_dec(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  valor -= 1;

  // atualizar o valor na memoria
  escrever_memoria(nes, endereco, valor);

  // atualizar flags
  cpu_n_escrever(&nes->cpu, valor);
  cpu_z_escrever(&nes->cpu, valor);
}

//! Diminui o valor do indice X por 1
static void instrucao_dex(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.x -= 1;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.x);
  cpu_z_escrever(&nes->cpu, nes->cpu.x);
}

//! Diminui o valor do indice Y por 1
static void instrucao_dey(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.y -= 1;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.y);
  cpu_z_escrever(&nes->cpu, nes->cpu.y);
}

//! OR exclusivo de um valor na memoria com o acumulador
static void instrucao_eor(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  nes->cpu.a = nes->cpu.a ^ valor;

  //atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

//! Incrementa um valor na memoria por 1
static void instrucao_inc(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  valor += 1;

  // atualizar o valor na memoria
  escrever_memoria(nes, endereco, valor);

  // atualizar flags
  cpu_n_escrever(&nes->cpu, valor);
  cpu_z_escrever(&nes->cpu, valor);
}

//! Incrementa o valor do indice X por 1
static void instrucao_inx(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.x += 1;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.x);
  cpu_z_escrever(&nes->cpu, nes->cpu.x);
}

//! Incrementa o valor do indice Y por 1
static void instrucao_iny(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.y += 1;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.y);
  cpu_z_escrever(&nes->cpu, nes->cpu.y);
}

//! Pula o programa para o endereço indicado
static void instrucao_jmp(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  // muda o endereço
  nes->cpu.pc = endereco;
}

//! Chama uma função/subrotina
static void instrucao_jsr(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  // Salva o endereço da próxima instrução subtraído por 1 na stack.
  // O endereço guardado vai ser usado para retornar da função quando
  // o opcode 'rts' for usado
  stack_empurrar_16_bits(nes, nes->cpu.pc - 1);

  // muda o endereço atual do programa para o da função indicada
  nes->cpu.pc = endereco;
}

//! Carrega um valor da memoria no acumulador
static void instrucao_lda(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.a = ler_memoria(nes, endereco);

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}


//! Carrega um valor da memoria no indice X
static void instrucao_ldx(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.x = ler_memoria(nes, endereco);

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.x);
  cpu_z_escrever(&nes->cpu, nes->cpu.x);
}

//! Carrega um valor da memoria no acumulador
static void instrucao_ldy(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.y = ler_memoria(nes, endereco);

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.y);
  cpu_z_escrever(&nes->cpu, nes->cpu.y);
}

/*!
  Instrução shift para a direita.
  Utiliza a memoria ou o acumulador
 */
static void instrucao_lsr(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    // checa se a posição 0 do byte é '1' ou '0'
    nes->cpu.c = buscar_bit(nes->cpu.a, 0);

    nes->cpu.a >>= 1;

    // atualizar flags
    cpu_n_escrever(&nes->cpu, nes->cpu.a);
    cpu_z_escrever(&nes->cpu, nes->cpu.a);
  }
  else
  {
    uint8_t valor = ler_memoria(nes, endereco);

    // checa se a posição 0 do byte é '1' ou '0'
    nes->cpu.c = buscar_bit(valor, 0);

    valor >>= 1;

    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(&nes->cpu, valor);
    cpu_z_escrever(&nes->cpu, valor);
  }
}

//! Não fazer nada
static void instrucao_nop(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
}

//! Operanção OR entre um valor na memoria e o acumulador
static void instrucao_ora(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  nes->cpu.a = nes->cpu.a | valor;

  //atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

//! Empurra o valor do acumulador na stack
static void instrucao_pha(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  stack_empurrar(nes, nes->cpu.a);
}

//! Empurra o valor do estado do processador na stack
static void instrucao_php(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  const uint8_t estado = cpu_estado_ler(nes->cpu);
  stack_empurrar(nes, estado);
}

//! Puxa um valor da stack e salva esse valor no acumulador
static void instrucao_pla(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.a = stack_puxar(nes);

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

//! Puxa um valor da stack e salva esse valor no estado do processador
static void instrucao_plp(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  const uint8_t estado = stack_puxar(nes);
  cpu_estado_escrever(nes->cpu, estado);
}

//! Gira um valor pra a esquerda
static void instrucao_rol(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    bool carregar = nes->cpu.c;
    nes->cpu.c = buscar_bit(nes->cpu.a, 7);
    nes->cpu.a <<= 1;
    nes->cpu.a = nes->cpu.a | ((carregar) ? 1 : 0);

    // atualizar flags
    cpu_n_escrever(&nes->cpu, nes->cpu.a);
    cpu_z_escrever(&nes->cpu, nes->cpu.a);
  }
  else
  {
    uint8_t valor = ler_memoria(nes, endereco);

    bool carregar = nes->cpu.c;
    nes->cpu.c = buscar_bit(valor, 7);
    valor <<= 1;
    valor = valor | ((carregar) ? 1 : 0);

    // atualizar o valor na memoria
    escrever_memoria(nes, endereco, valor);

    // atualizar flags
    cpu_n_escrever(&nes->cpu, nes->cpu.a);
    cpu_z_escrever(&nes->cpu, nes->cpu.a);
  }
}

//! Gira um valor pra a direita
static void instrucao_ror(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  if (instrucao->modo == MODO_ENDER_ACM)
  {
    bool carregar = nes->cpu.c;
    nes->cpu.c = buscar_bit(nes->cpu.a, 0);
    nes->cpu.a >>= 1;
    nes->cpu.a = nes->cpu.a | ((carregar) ? 0b10000000 : 0);

    // atualizar flags
    cpu_n_escrever(&nes->cpu, nes->cpu.a);
    cpu_z_escrever(&nes->cpu, nes->cpu.a);
  }
  else
  {
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
static void instrucao_rti(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  const uint8_t estado = stack_puxar(nes);
  cpu_estado_escrever(nes->cpu, estado);

  nes->cpu.pc = stack_puxar_16_bits(nes);
}

//! Retorna de uma função/sub-rotina
static void instrucao_rts(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.pc = stack_puxar_16_bits(nes) + 1;
}

//! Subtrai um valor da memoria usando o acumulador
static void instrucao_sbc(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  uint8_t valor = ler_memoria(nes, endereco);

  const uint8_t a = nes->cpu.a;
  const uint8_t c = (nes->cpu.c) ? 1 : 0;

  nes->cpu.a = a - valor - 1 - c;

  // atualiza a flag c
  int32_t subtracao_total = (int32_t)a - (int32_t)valor - 1 - (int32_t)c;
  if (subtracao_total >= 0)
    nes->cpu.c = 1;
  else
    nes->cpu.c = 0;

  // checa se houve um overflow/transbordamento e atualiza a flag v
  // solução baseada em: https://stackoverflow.com/a/16861251
  if ((~(a ^ (valor*-1 - 1))) & (a ^ c) & 0x80)
    nes->cpu.v = 1;
  else
    nes->cpu.v = 0;

  // atualiza as flags z e n
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

//! Ativa a flag 'c'
static void instrucao_sec(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.c = true;
}

//! Ativa a flag 'd'
static void instrucao_sed(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.d = true;
}

//! Ativa a flag 'i'
static void instrucao_sei(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.i = true;
}

//! Guarda o valor do acumulador na memoria
static void instrucao_sta(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  escrever_memoria(nes, enderecones->cpu.a);
}

//! Guarda o valor do registrador 'x' na memoria
static void instrucao_stx(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  escrever_memoria(nes, endereco, nes->cpu.x);
}

//! Guarda o valor do registrador 'y' na memoria
static void instrucao_sty(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  escrever_memoria(nes, endereco, nes->cpu.y);
}

//! Atribui o valor do acumulador ao registrador 'x'
static void instrucao_tax(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.x = nes->cpu.a;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.x);
  cpu_z_escrever(&nes->cpu, nes->cpu.x);
}

//! Atribui o valor do acumulador ao registrador 'y'
static void instrucao_tay(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.y = nes->cpu.a;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.y);
  cpu_z_escrever(&nes->cpu, nes->cpu.y);
}

//! Atribui o valor do ponteiro da stack ao registrador 'x'
static void instrucao_tsx(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.x = nes->cpu.sp;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.x);
  cpu_z_escrever(&nes->cpu, nes->cpu.x);
}

//! Atribui o valor do registrador 'x' ao acumulador
static void instrucao_txa(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.a = nes->cpu.x;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

//! Atribui o valor do registrador 'x' ao ponteiro da stack
static void instrucao_txs(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.sp = nes->cpu.x;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.sp);
  cpu_z_escrever(&nes->cpu, nes->cpu.sp);
}

//! Atribui o valor do registrador 'y' ao acumulador
static void instrucao_tya(Instrucao* instrucao, Nes* nes, uint16_t endereco)
{
  nes->cpu.a = nes->cpu.y;

  // atualizar flags
  cpu_n_escrever(&nes->cpu, nes->cpu.a);
  cpu_z_escrever(&nes->cpu, nes->cpu.a);
}

Instrucao** carregar_instrucoes(void)
{
  // cria um array com 0x100 (256 em decimal) ponteiros para instruções
  Instrucao **instrucoes = (Instrucao**)malloc(sizeof(Instrucao*) * 0x100);
  for (int i = 0; i < 0x100; i++)
  {
    instrucoes[i] = NULL;
  }

  // modos da instrução ADC
  instrucoes[0x69] = new Instrucao("ADC", 2, 2, 0, MODO_ENDER_IMED, instrucao_adc);
  instrucoes[0x65] = new Instrucao("ADC", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_adc);
  instrucoes[0x75] = new Instrucao("ADC", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_adc);
  instrucoes[0x6D] = new Instrucao("ADC", 3, 4, 0, MODO_ENDER_ABS, instrucao_adc);
  instrucoes[0x7D] = new Instrucao("ADC", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_adc);
  instrucoes[0x79] = new Instrucao("ADC", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_adc);
  instrucoes[0x61] = new Instrucao("ADC", 2, 6, 0, MODO_ENDER_IND_X, instrucao_adc);
  instrucoes[0x71] = new Instrucao("ADC", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_adc);

  // modos da instrução AND
  instrucoes[0x29] = new Instrucao("AND", 2, 2, 0, MODO_ENDER_IMED, instrucao_and);
  instrucoes[0x25] = new Instrucao("AND", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_and);
  instrucoes[0x35] = new Instrucao("AND", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_and);
  instrucoes[0x2D] = new Instrucao("AND", 3, 4, 0, MODO_ENDER_ABS, instrucao_and);
  instrucoes[0x3D] = new Instrucao("AND", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_and);
  instrucoes[0x39] = new Instrucao("AND", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_and);
  instrucoes[0x21] = new Instrucao("AND", 2, 6, 0, MODO_ENDER_IND_X, instrucao_and);
  instrucoes[0x21] = new Instrucao("AND", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_and);

  // modos da instrução ASL
  instrucoes[0x0A] = new Instrucao("ASL", 1, 2, 0, MODO_ENDER_ACM, instrucao_asl);
  instrucoes[0x06] = new Instrucao("ASL", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_asl);
  instrucoes[0x16] = new Instrucao("ASL", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_asl);
  instrucoes[0x0E] = new Instrucao("ASL", 3, 6, 0, MODO_ENDER_ABS, instrucao_asl);
  instrucoes[0x1E] = new Instrucao("ASL", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_asl);

  // modos da instrução BCC
  instrucoes[0x90] = new Instrucao("BCC", 2, 2, 0, MODO_ENDER_REL, instrucao_bcc);

  // modos da instrução BCS
  instrucoes[0xB0] = new Instrucao("BCS", 2, 2, 0, MODO_ENDER_REL, instrucao_bcs);

  // modos da instrução BEQ
  instrucoes[0xF0] = new Instrucao("BEQ", 2, 2, 0, MODO_ENDER_REL, instrucao_beq);

  // modos da instrução BIT
  instrucoes[0x24] = new Instrucao("BIT", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_bit);
  instrucoes[0x2C] = new Instrucao("BIT", 3, 4, 0, MODO_ENDER_ABS, instrucao_bit);

  // modos da instrução BMI
  instrucoes[0x30] = new Instrucao("BIM", 2, 2, 0, MODO_ENDER_REL, instrucao_bmi);

  // modos da instrução BNE
  instrucoes[0xD0] = new Instrucao("BNE", 2, 2, 0, MODO_ENDER_REL, instrucao_bne);

  // modos da instrução BPL
  instrucoes[0x10] = new Instrucao("BPL", 2, 2, 0, MODO_ENDER_REL, instrucao_bpl);

  // modos da instrução BRK
  instrucoes[0x00] = new Instrucao("BRK", 1, 7, 0, MODO_ENDER_IMPL, instrucao_brk);

  // modos da instrução BVC
  instrucoes[0x50] = new Instrucao("BVC", 2, 2, 0, MODO_ENDER_REL, instrucao_bvc);

  // modos da instrução BVS
  instrucoes[0x70] = new Instrucao("BVS", 2, 2, 0, MODO_ENDER_REL, instrucao_bvs);

  // modos da instrução CLC
  instrucoes[0x18] = new Instrucao("CLC", 1, 2, 0, MODO_ENDER_IMPL, instrucao_clc);

  // modos da instrução CLD
  instrucoes[0xD8] = new Instrucao("CLD", 1, 2, 0, MODO_ENDER_IMPL, instrucao_cld);

  // modos da instrução CLI
  instrucoes[0x58] = new Instrucao("CLI", 1, 2, 0, MODO_ENDER_IMPL, instrucao_cli);

  // modos da instrução CLV
  instrucoes[0xB8] = new Instrucao("CLV", 1, 2, 0, MODO_ENDER_IMPL, instrucao_clv);

  // modos da instrução CMP
  instrucoes[0xC9] = new Instrucao("CMP", 2, 2, 0, MODO_ENDER_IMED, instrucao_cmp);
  instrucoes[0xC5] = new Instrucao("CMP", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_cmp);
  instrucoes[0xD5] = new Instrucao("CMP", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_cmp);
  instrucoes[0xCD] = new Instrucao("CMP", 3, 4, 0, MODO_ENDER_ABS, instrucao_cmp);
  instrucoes[0xDD] = new Instrucao("CMP", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_cmp);
  instrucoes[0xD9] = new Instrucao("CMP", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_cmp);
  instrucoes[0xC1] = new Instrucao("CMP", 2, 6, 0, MODO_ENDER_IND_X, instrucao_cmp);
  instrucoes[0xD1] = new Instrucao("CMP", 3, 5, 1, MODO_ENDER_IND_Y, instrucao_cmp);

  // modos da instrução CPX
  instrucoes[0xE0] = new Instrucao("CPX", 2, 2, 0, MODO_ENDER_IMED, instrucao_cpx);
  instrucoes[0xE4] = new Instrucao("CPX", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_cpx);
  instrucoes[0xEC] = new Instrucao("CPX", 3, 4, 0, MODO_ENDER_ABS, instrucao_cpx);

   // modos da instrução CPY
  instrucoes[0xC0] = new Instrucao("CPY", 2, 2, 0, MODO_ENDER_IMED, instrucao_cpy);
  instrucoes[0xC4] = new Instrucao("CPY", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_cpy);
  instrucoes[0xCC] = new Instrucao("CPY", 3, 4, 0, MODO_ENDER_ABS, instrucao_cpy);

  // modos da instrução DEC
  instrucoes[0xC6] = new Instrucao("DEC", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_dec);
  instrucoes[0xD6] = new Instrucao("DEC", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_dec);
  instrucoes[0xCE] = new Instrucao("DEC", 3, 3, 0, MODO_ENDER_ABS, instrucao_dec);
  instrucoes[0xDE] = new Instrucao("DEC", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_dec);

  // modos da instrução DEX
  instrucoes[0xCA] = new Instrucao("DEX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_dex);

  // modos da instrução DEY
  instrucoes[0x88] = new Instrucao("DEX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_dey);

  // modos da instrução EOR
  instrucoes[0x49] = new Instrucao("EOR", 2, 2, 0, MODO_ENDER_IMED, instrucao_eor);
  instrucoes[0x45] = new Instrucao("EOR", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_eor);
  instrucoes[0x55] = new Instrucao("EOR", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_eor);
  instrucoes[0x4D] = new Instrucao("EOR", 3, 4, 0, MODO_ENDER_ABS, instrucao_eor);
  instrucoes[0x5D] = new Instrucao("EOR", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_eor);
  instrucoes[0x59] = new Instrucao("EOR", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_eor);
  instrucoes[0x41] = new Instrucao("EOR", 2, 6, 0, MODO_ENDER_IND_X, instrucao_eor);
  instrucoes[0x51] = new Instrucao("EOR", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_eor);

  // modos da instrução INC
  instrucoes[0xE6] = new Instrucao("INC", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_inc);
  instrucoes[0xF6] = new Instrucao("INC", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_inc);
  instrucoes[0xEE] = new Instrucao("INC", 3, 6, 0, MODO_ENDER_ABS, instrucao_inc);
  instrucoes[0xFE] = new Instrucao("INC", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_inc);

  // modos da instrução INX
  instrucoes[0xE8] = new Instrucao("INX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_inx);

  // modos da instrução INY
  instrucoes[0xC8] = new Instrucao("INY", 1, 2, 0, MODO_ENDER_IMPL, instrucao_iny);

  // modos da instrução JMP
  instrucoes[0x4C] = new Instrucao("JMP", 3, 3, 0, MODO_ENDER_ABS, instrucao_jmp);
  instrucoes[0x6C] = new Instrucao("JMP", 3, 5, 0, MODO_ENDER_IND, instrucao_jmp);

  // modos da instrução JSR
  instrucoes[0x20] = new Instrucao("JSR", 3, 6, 0, MODO_ENDER_ABS, instrucao_jsr);

  // modos da instrução LDA
  instrucoes[0xA9] = new Instrucao("LDA", 2, 2, 0, MODO_ENDER_IMED, instrucao_lda);
  instrucoes[0xA5] = new Instrucao("LDA", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_lda);
  instrucoes[0xB5] = new Instrucao("LDA", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_lda);
  instrucoes[0xAD] = new Instrucao("LDA", 3, 4, 0, MODO_ENDER_ABS, instrucao_lda);
  instrucoes[0xBD] = new Instrucao("LDA", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_lda);
  instrucoes[0xB9] = new Instrucao("LDA", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_lda);
  instrucoes[0xA1] = new Instrucao("LDA", 2, 6, 0, MODO_ENDER_IND_X, instrucao_lda);
  instrucoes[0xB1] = new Instrucao("LDA", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_lda);

  // modos da instrução LDX
  instrucoes[0xA2] = new Instrucao("LDX", 2, 2, 0, MODO_ENDER_IMED, instrucao_ldx);
  instrucoes[0xA6] = new Instrucao("LDX", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_ldx);
  instrucoes[0xB6] = new Instrucao("LDX", 2, 4, 0, MODO_ENDER_P_ZERO_Y, instrucao_ldx);
  instrucoes[0xAE] = new Instrucao("LDX", 3, 4, 0, MODO_ENDER_ABS, instrucao_ldx);
  instrucoes[0xBE] = new Instrucao("LDX", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_ldx);

  // modos da instrução LDY
  instrucoes[0xA0] = new Instrucao("LDY", 2, 2, 0, MODO_ENDER_IMED, instrucao_ldy);
  instrucoes[0xA4] = new Instrucao("LDY", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_ldy);
  instrucoes[0xB4] = new Instrucao("LDY", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_ldy);
  instrucoes[0xAC] = new Instrucao("LDY", 3, 4, 0, MODO_ENDER_ABS, instrucao_ldy);
  instrucoes[0xBC] = new Instrucao("LDY", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_ldy);

  // modos da instrução LSR
  instrucoes[0x4A] = new Instrucao("LSR", 1, 2, 0, MODO_ENDER_ACM, instrucao_lsr);
  instrucoes[0x46] = new Instrucao("LSR", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_lsr);
  instrucoes[0x56] = new Instrucao("LSR", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_lsr);
  instrucoes[0x4E] = new Instrucao("LSR", 3, 6, 0, MODO_ENDER_ABS, instrucao_lsr);
  instrucoes[0x5E] = new Instrucao("LSR", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_lsr);

  // modos da instrução NOP
  instrucoes[0xEA] = new Instrucao("NOP", 1, 2, 0, MODO_ENDER_IMPL, instrucao_nop);

  // modos da instrução ORA
  instrucoes[0x09] = new Instrucao("ORA", 2, 2, 0, MODO_ENDER_IMED, instrucao_ora);
  instrucoes[0x05] = new Instrucao("ORA", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_ora);
  instrucoes[0x15] = new Instrucao("ORA", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_ora);
  instrucoes[0x0D] = new Instrucao("ORA", 3, 4, 0, MODO_ENDER_ABS, instrucao_ora);
  instrucoes[0x1D] = new Instrucao("ORA", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_ora);
  instrucoes[0x19] = new Instrucao("ORA", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_ora);
  instrucoes[0x01] = new Instrucao("ORA", 2, 6, 0, MODO_ENDER_IND_X, instrucao_ora);
  instrucoes[0x11] = new Instrucao("ORA", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_ora);

  // modos da instrução PHA
  instrucoes[0x48] = new Instrucao("PHA", 1, 3, 0, MODO_ENDER_IMPL, instrucao_pha);

  // modos da instrução PHP
  instrucoes[0x08] = new Instrucao("PHP", 1, 3, 0, MODO_ENDER_IMPL, instrucao_php);

  // modos da instrução PLA
  instrucoes[0x68] = new Instrucao("PLA", 1, 4, 0, MODO_ENDER_IMPL, instrucao_pla);

  // modos da instrução PLP
  instrucoes[0x28] = new Instrucao("PLP", 1, 4, 0, MODO_ENDER_IMPL, instrucao_plp);

  // modos da instrução ROL
  instrucoes[0x2A] = new Instrucao("ROL", 1, 2, 0, MODO_ENDER_ACM, instrucao_rol);
  instrucoes[0x26] = new Instrucao("ROL", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_rol);
  instrucoes[0x36] = new Instrucao("ROL", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_rol);
  instrucoes[0x2E] = new Instrucao("ROL", 3, 6, 0, MODO_ENDER_ABS, instrucao_rol);
  instrucoes[0x3E] = new Instrucao("ROL", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_rol);

  // modos da instrução ROR
  instrucoes[0x6A] = new Instrucao("ROR", 1, 2, 0, MODO_ENDER_ACM, instrucao_ror);
  instrucoes[0x66] = new Instrucao("ROR", 2, 5, 0, MODO_ENDER_P_ZERO, instrucao_ror);
  instrucoes[0x76] = new Instrucao("ROR", 2, 6, 0, MODO_ENDER_P_ZERO_X, instrucao_ror);
  instrucoes[0x6E] = new Instrucao("ROR", 3, 6, 0, MODO_ENDER_ABS, instrucao_ror);
  instrucoes[0x7E] = new Instrucao("ROR", 3, 7, 0, MODO_ENDER_ABS_X, instrucao_ror);

  // modos da instrução RTI
  instrucoes[0x40] = new Instrucao("RTI", 1, 6, 0, MODO_ENDER_IMPL, instrucao_rti);

  // modos da instrução RTS
  instrucoes[0x60] = new Instrucao("RTS", 1, 6, 0, MODO_ENDER_IMPL, instrucao_rts);

  // modos da instrução SBC
  instrucoes[0xE9] = new Instrucao("SBC", 2, 2, 0, MODO_ENDER_IMED, instrucao_sbc);
  instrucoes[0xE5] = new Instrucao("SBC", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_sbc);
  instrucoes[0xF5] = new Instrucao("SBC", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_sbc);
  instrucoes[0xED] = new Instrucao("SBC", 3, 4, 0, MODO_ENDER_ABS, instrucao_sbc);
  instrucoes[0xFD] = new Instrucao("SBC", 3, 4, 1, MODO_ENDER_ABS_X, instrucao_sbc);
  instrucoes[0xF9] = new Instrucao("SBC", 3, 4, 1, MODO_ENDER_ABS_Y, instrucao_sbc);
  instrucoes[0xE1] = new Instrucao("SBC", 2, 6, 0, MODO_ENDER_IND_X, instrucao_sbc);
  instrucoes[0xF1] = new Instrucao("SBC", 2, 5, 1, MODO_ENDER_IND_Y, instrucao_sbc);

  // modos da instrução SEC
  instrucoes[0x38] = new Instrucao("SEC", 1, 2, 0, MODO_ENDER_IMPL, instrucao_sec);

  // modos da instrução SED
  instrucoes[0xF8] = new Instrucao("SED", 1, 2, 0, MODO_ENDER_IMPL, instrucao_sed);

  // modos da instrução SEI
  instrucoes[0x78] = new Instrucao("SEI", 1, 2, 0, MODO_ENDER_IMPL, instrucao_sei);

  // modos da instrução STA
  instrucoes[0x85] = new Instrucao("STA", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_sta);
  instrucoes[0x95] = new Instrucao("STA", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_sta);
  instrucoes[0x8D] = new Instrucao("STA", 3, 4, 0, MODO_ENDER_ABS, instrucao_sta);
  instrucoes[0x9D] = new Instrucao("STA", 3, 5, 0, MODO_ENDER_ABS_X, instrucao_sta);
  instrucoes[0x99] = new Instrucao("STA", 3, 5, 0, MODO_ENDER_ABS_Y, instrucao_sta);
  instrucoes[0x81] = new Instrucao("STA", 2, 6, 0, MODO_ENDER_IND_X, instrucao_sta);
  instrucoes[0x91] = new Instrucao("STA", 2, 6, 0, MODO_ENDER_IND_Y, instrucao_sta);

  // modos da instrução STX
  instrucoes[0x86] = new Instrucao("STX", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_stx);
  instrucoes[0x96] = new Instrucao("STX", 2, 4, 0, MODO_ENDER_P_ZERO_Y, instrucao_stx);
  instrucoes[0x8E] = new Instrucao("STX", 3, 4, 0, MODO_ENDER_ABS, instrucao_stx);

  // modos da instrução STY
  instrucoes[0x84] = new Instrucao("STY", 2, 3, 0, MODO_ENDER_P_ZERO, instrucao_sty);
  instrucoes[0x94] = new Instrucao("STY", 2, 4, 0, MODO_ENDER_P_ZERO_X, instrucao_sty);
  instrucoes[0x8C] = new Instrucao("STY", 3, 4, 0, MODO_ENDER_ABS, instrucao_sty);

  // modos da instrução TAX
  instrucoes[0xAA] = new Instrucao("TAX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_tax);

  // modos da instrução TAY
  instrucoes[0xA8] = new Instrucao("TAY", 1, 2, 0, MODO_ENDER_IMPL, instrucao_tay);

  // modos da instrução TSX
  instrucoes[0xBA] = new Instrucao("TSX", 1, 2, 0, MODO_ENDER_IMPL, instrucao_tsx);

  // modos da instrução TXA
  instrucoes[0x8A] = new Instrucao("TXA", 1, 2, 0, MODO_ENDER_IMPL, instrucao_txa);

  // modos da instrução TXS
  instrucoes[0x9A] = new Instrucao("TXS", 1, 2, 0, MODO_ENDER_IMPL, instrucao_txs);

  // modos da instrução TYA
  instrucoes[0x98] = new Instrucao("TYA", 1, 2, 0, MODO_ENDER_IMPL, instrucao_tya);

  return instrucoes;
}
