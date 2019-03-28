/* instrucao.h
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

#pragma once

#include <cstdint>
#include <functional>

#include "cpu.hpp"

using std::function;

// referencias utilizadas:
// https://www.masswerk.at/6502/6502_instruction_set.html

//! Modos de endereçamento das instruções
typedef enum
{
  MODO_ENDER_ACM,       // acumulador
  MODO_ENDER_ABS,       // absoluto
  MODO_ENDER_ABS_X,     // absoluto, indexado pelo registrador x
  MODO_ENDER_ABS_Y,     // absoluto, indexado pelo registrador y
  MODO_ENDER_IMED,      // imediato
  MODO_ENDER_IMPL,      // implicado
  MODO_ENDER_IND,       // indireto
  MODO_ENDER_IND_X,     // indireto indexado por x
  MODO_ENDER_IND_Y,     // indireto indexado por y
  MODO_ENDER_REL,       // relativo
  MODO_ENDER_P_ZERO,    // página 0
  MODO_ENDER_P_ZERO_X,  // página 0, indexado pelo registrador x
  MODO_ENDER_P_ZERO_Y,  // página 0, indexado pelo registrador y
} InstrucaoModo;

class Nes;

//! Uma instrução da arquitetura 6502
class Instrucao
{
public:
        char    *nome;
        uint8_t  bytes;
        int32_t  ciclos;

        // quantidade de ciclos adicionais que devem ocorrer quando a
        // página da memoria for alterada durante a leitura do endereço
        int32_t ciclos_pag_alterada;

        InstrucaoModo modo;

	      /*! Uma fução de alto nivel que sera usada para reimplementar
            uma instrução da arquitetura 6502 */
        function<void(Instrucao*, Nes*, uint16_t)> funcao;

        Instrucao(
          char *nome,
          uint8_t bytes,
          int32_t ciclos,
          int32_t ciclos_pag_alterada,
          InstrucaoModo  modo,
          function<void(Instrucao*, Nes*, uint16_t)> funcao
	      );

        /*!
          Busca o endereço que vai ser usado por uma instrução de
          acordo com o modo de endereçamento da CPU
        */
	      uint16_t buscar_endereco(Nes* nes);
};

Instrucao** carregar_instrucoes(void);
