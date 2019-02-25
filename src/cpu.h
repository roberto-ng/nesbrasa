#pragma once

#include <stdint.h>

// referencias utilizadas:
// http://www.obelisk.me.uk/6502/registers.html

typedef struct _Cpu Cpu;

struct _Cpu {
        uint16_t pc; // contador de programa
        uint8_t  sp; // ponteiro da stack
        uint8_t  a;  // registrador acumulador
        uint8_t  x;  // registrador de indice x
        uint8_t  y;  // registrador de indice y
        uint8_t  c;  // flag de carregamento (carry flag)
        uint8_t  z;  // flag zero
        uint8_t  d;  // flag decimal
        uint8_t  v;  // flag de transbordamento (overflow flag)
        uint8_t  n;  // flag de negativo
        uint8_t  i;  // flag de desabilitar interrupções
        uint8_t  b;  // flag da instrução break (break command flag)
};
