#include <stdlib.h>

#include "mapeador.h"

Mapeador*
mapeador_new ()
{
  Mapeador *mapeador = malloc (sizeof (Mapeador));

  return mapeador;
}

void
mapeador_free (Mapeador *mapeador)
{
  free (mapeador);
}
