#pragma once

#include <stdint.h>

#include "cpu.h"

// modos de endereçamento das instruções
typedef enum {
  MODO_ENDER_ACM,      // acumulador
  MODO_ENDER_ABS,      // absoluto
  MODO_ENDER_ABS_X,    // absoluto, indexado pelo registrador x
  MODO_ENDER_ABS_Y,    // absoluto, indexado pelo registrador y
  MODO_ENDER_IMED,     // imediato
  MODO_ENDER_IMPL,     // implicado
  MODO_ENDER_IND,      // indireto
  MODO_ENDER_IND_X,    // indireto, indexado pelo registrador x
  MODO_ENDER_IND_Y,    // indireto, indexado pelo registrador y
  MODO_ENDER_REL,      // relativo
  MODO_ENDER_P_ZERO,   // página 0
  MODO_ENDER_P_ZERO_X, // página 0, indexado pelo registrador x
  MODO_ENDER_P_ZERO_Y, // página 0, indexado pelo registrador y
} InstrucaoModo;

typedef struct _Instrucao Instrucao;

/* Ponteiro para uma fução de alto nivel que
   sera usada para reimplementar uma instrução
   da arquitetura 6502 */
typedef void (*InstrucaoFunc)(Instrucao*, Cpu*);

struct _Instrucao {
        uint8_t       codigo;
        uint8_t       bytes;
        int           ciclos;
        InstrucaoModo modo;
        InstrucaoFunc funcao;
};

Instrucao* instrucao_new  (uint8_t       codigo,
                           uint8_t       bytes,
                           int           ciclos,
                           InstrucaoModo modo,
                           InstrucaoFunc funcao);

void       instrucao_free (Instrucao *instr);
