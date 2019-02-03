#include <stdlib.h>

#include "instrucao.h"

Instrucao*
instrucao_new (uint8_t codigo,
               uint8_t bytes,
               int ciclos,
               InstrucaoModo modo)
{
  Instrucao *instr = malloc (sizeof (Instrucao));
  instr->codigo = codigo;
  instr->bytes = bytes;
  instr->ciclos = ciclos;
  instr->modo = modo;

  return instr;
}

void
instrucao_free (Instrucao *instr)
{
  free(instr);
}
