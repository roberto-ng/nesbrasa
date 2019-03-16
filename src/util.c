#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

inline bool
buscar_bit (uint8_t byte,
            uint8_t pos)
{
  // dar a volta quando a posição do bit for maior que 7
  pos = pos % 8;

  const uint8_t tmp = byte & (1 << pos);
  return (tmp >> pos) != 0;
}

inline bool
comparar_paginas (uint16_t pagina_1,
                  uint16_t pagina_2)
{
  return (pagina_1 & 0xFF00) != (pagina_2 & 0xFF00);
}

char*
formatar_str (char *fmt, ...)
{
  va_list args;
  va_list args_copia;
  // inicializa a lista de argumentos
  va_start(args, fmt);

  // copia a lista de argumentos
  va_copy(args_copia, args);
  // calcula o tamanho necessário para o buffer
  size_t tam = vsnprintf(NULL, 0, fmt, args_copia) + 1;
  // limpa a variável 'args_copia'
  va_end(args_copia);

  // aloca o buffer de acordo com o tamanho calculado
  char *str = malloc(tam);
  // copia a lista de argumentos novamente
  va_copy(args_copia, args);
  // formata a string
  vsnprintf(str, tam, fmt, args_copia);
  va_end(args_copia);

  va_end(args);
  return str;
}
