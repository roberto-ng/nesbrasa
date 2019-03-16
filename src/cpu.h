#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "nesbrasa.h"
#include "memoria.h"

// referencias utilizadas:
// http://www.obelisk.me.uk/6502/registers.html

typedef struct _Nes Nes;
typedef struct _Cpu Cpu;

struct _Cpu
{
        uint16_t pc; // contador de programa
        uint8_t  sp; // ponteiro da stack

        uint8_t a; // registrador acumulador
        uint8_t x; // registrador de indice x
        uint8_t y; // registrador de indice y

        bool c; // flag de carregamento (carry flag)
        bool z; // flag zero
        bool i; // flag de desabilitar interrupções
        bool d; // flag decimal
        bool b; // flag da instrução break (break command flag)
        bool v; // flag de transbordamento (overflow flag)
        bool n; // flag de negativo

        uint16_t esperar;
        uint32_t ciclos;
};

Cpu* cpu_new  (void);

void cpu_free (Cpu *cpu);


/*! Calcula a quantidade de ciclos em um branch e a soma em 'cpu->ciclos'.
 \param endereco O endereço em que o branch sera realizado
 */
void cpu_branch_somar_ciclos (Cpu      *cpu,
                              uint16_t  endereco);


uint8_t cpu_estado_ler       (Cpu *cpu);

void    cpu_estado_escrever  (Cpu     *cpu,
                              uint8_t  valor);

//! Ativa a flag de zero caso seja necessario
void    cpu_z_escrever       (Cpu     *cpu,
                              uint8_t  valor);

//! Ativa a flag de valor negativo caso seja necessario
void    cpu_n_escrever       (Cpu     *cpu,
                              uint8_t  valor);


//! Empurra um valor na stack
void     stack_push         (Nes    *nes,
                             uint8_t valor);

//! Empurra um valor na stack
void     stack_push_16_bits (Nes      *nes,
                             uint16_t  valor);

//! Puxa um valor da stack
uint8_t  stack_pull         (Nes *nes);

//! Puxa um valor da stack
uint16_t stack_pull_16_bits (Nes *nes);
