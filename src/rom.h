#pragma once

typedef enum
{
  ESPELHAMENTO_HORIZONTAL,
  ESPELHAMENTO_VERTICAL,
  ESPELHAMENTO_TELA_UNICA,
  ESPELHAMENTO_4_TELAS,
} Espelhamento;

typedef struct _Rom Rom;

struct _Rom
{
        Espelhamento espelhamento;
};

Rom* rom_new  (void);

void rom_free (Rom *rom);
