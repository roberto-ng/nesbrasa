#pragma once

typedef struct _Mapeador Mapeador;

struct _Mapeador {};

Mapeador* mapeador_new  ();

void      mapeador_free (Mapeador *mapeador);
