#pragma once

#include "nesbrasa.h"

typedef struct _Nes Nes;

typedef uint8_t (*MapeadorLerFunc)      (Nes      *nes,
                                         uint16_t  endereco);

typedef void    (*MapeadorEscreverFunc) (Nes      *nes,
                                         uint16_t  endereco,
                                         uint8_t   valor);

typedef struct _Mapeador Mapeador;

struct _Mapeador
{
        MapeadorLerFunc      ler;
        MapeadorEscreverFunc escrever;
};

Mapeador* mapeador_new  ();

void      mapeador_free (Mapeador *mapeador);

