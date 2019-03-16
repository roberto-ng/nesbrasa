#pragma once

#include <stdint.h>
#include <stdbool.h>

/*! Macro que retorna um o tamanho de um array
 *  (Não funciona com ponteiros ou arrays passados como argumentos)
 */
#define TAMANHO(__LISTA)\
  (sizeof (__LISTA)/sizeof (__LISTA[0]))

//! Busca uma posição de 0 a 7 em um byte e returna o seu valor
bool buscar_bit       (uint8_t byte,
                       uint8_t pos);

//! Checa se dois endereços da memoria estão na mesma pagina
bool comparar_paginas (uint16_t pagina_1,
                       uint16_t pagina_2);

//! Uma alternativa segura ao 'sprintf'
char* formatar_str    (char *fmt, ...);
