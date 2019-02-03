#pragma once

#include <stdint.h>

// modos de endereçamento
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
  MODO_ENDER_P_ZERO,   // indireto
  MODO_ENDER_P_ZERO_X, // indireto, indexado pelo registrador x
  MODO_ENDER_P_ZERO_Y, // indireto, indexado pelo registrador y
} InstrucaoModo;

typedef struct _Instrucao Instrucao;

struct _Instrucao {
  uint8_t codigo;
  uint8_t bytes;
  int ciclos;
  InstrucaoModo modo;
};
