#include <stdlib.h>

#include "rom.h"

Rom*
rom_new (void)
{
  Rom *rom = malloc (sizeof (Rom));
  rom->espelhamento = ESPELHAMENTO_TELA_UNICA;

  return rom;
}

void
rom_free (Rom *rom)
{
  free (rom);
}
