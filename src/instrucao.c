#include <stdlib.h>

#include "instrucao.h"

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
  free(instr);
}
