#include <stdlib.h>

#include "ppu.h"
#include "memoria.h"
#include "util.h"

Ppu*
ppu_new (void)
{
  Ppu *ppu = malloc (sizeof (Ppu));

  ppu->ultimo_valor = 0;
  ppu->vram_incrementar = 0;
  ppu->oam_endereco = 0;
  ppu->nametable_endereco = 0;
  ppu->padrao_fundo_endereco = 0;
  ppu->padrao_sprite_endereco = 0;

  ppu->flag_nmi = false;
  ppu->flag_mestre_escravo = false;
  ppu->flag_sprite_altura = false;
  ppu->flag_padrao_fundo = false;
  ppu->flag_padrao_sprite = false;
  ppu->flag_incrementar = false;
  ppu->flag_nametable_base = 0;

  ppu->flag_enfase_b = false;
  ppu->flag_enfase_g = false;
  ppu->flag_enfase_r = false;
  ppu->flag_sprite_habilitar = false;
  ppu->flag_fundo_habilitar = false;
  ppu->flag_sprite_habilitar_col_esquerda = false;
  ppu->flag_fundo_habilitar_col_esquerda = false;
  ppu->flag_escala_cinza = false;

  ppu->flag_vblank = false;
  ppu->flag_sprite_zero = false;
  ppu->flag_sprite_transbordamento = false;

  ppu->v = 0;
  ppu->t = 0;
  ppu->x = 0;
  ppu->w = false;

  for (int i = 0; i < TAMANHO (ppu->oam); i++) {
    ppu->oam[i] = 0;
  }

  for (int i = 0; i < TAMANHO (ppu->nametables); i++) {
    ppu->nametables[i] = 0;
  }

  return ppu;
}

void
ppu_free (Ppu *ppu)
{
  free (ppu);
}

void
ppu_controle_escrever (Nes     *nes,
                       uint8_t  valor)
{
  Ppu *ppu = nes->ppu;

  ppu->flag_nmi = buscar_bit (valor, 7);
  ppu->flag_mestre_escravo = buscar_bit (valor, 6);
  ppu->flag_sprite_altura = buscar_bit (valor, 5);
  ppu->flag_padrao_fundo = buscar_bit (valor, 4);
  ppu->flag_padrao_sprite = buscar_bit (valor, 3);
  ppu->flag_incrementar = buscar_bit (valor, 2);
  ppu->flag_nametable_base = valor & 0b00000011;

  ppu->nametable_endereco = 0x2000 + (0x400 * ppu->flag_nametable_base);

  if (ppu->flag_incrementar) {
    ppu->vram_incrementar = 32;
  }
  else {
    ppu->vram_incrementar = 1;
  }

  if (ppu->flag_padrao_sprite) {
    ppu->padrao_sprite_endereco = 0x1000;
  }
  else {
    ppu->padrao_sprite_endereco = 0x0000;
  }

  if (ppu->flag_padrao_fundo) {
    ppu->padrao_fundo_endereco = 0x1000;
  }
  else {
    ppu->padrao_fundo_endereco = 0x0000;
  }

  // t: ...BA.. ........ = d: ......BA
  ppu->t = (ppu->t & 0b1111001111111111) | ((valor & 0b00000011) << 10);
}

void
ppu_mascara_escrever (Nes     *nes,
                      uint8_t  valor)
{
  Ppu *ppu = nes->ppu;

  ppu->flag_enfase_b = buscar_bit (valor, 7);
  ppu->flag_enfase_g = buscar_bit (valor, 6);
  ppu->flag_enfase_r = buscar_bit (valor, 5);
  ppu->flag_sprite_habilitar = buscar_bit (valor, 4);
  ppu->flag_fundo_habilitar = buscar_bit (valor, 3);
  ppu->flag_sprite_habilitar_col_esquerda = buscar_bit (valor, 2);
  ppu->flag_fundo_habilitar_col_esquerda = buscar_bit (valor, 1);
  ppu->flag_escala_cinza = buscar_bit (valor, 0);
}

uint8_t
ppu_estado_ler (Nes *nes)
{
  Ppu *ppu = nes->ppu;

  // os 5 ultimos bits do último valor escrito na PPU
  const uint8_t ultimo = ppu->ultimo_valor & 0b00011111;

  const uint8_t v = (uint8_t)ppu->flag_vblank << 7;
  const uint8_t s = (uint8_t)ppu->flag_sprite_zero << 6;
  const uint8_t o = (uint8_t)ppu->flag_sprite_transbordamento << 5;

  ppu->flag_vblank = false;
  // w: = 0
  ppu->w = 0;

  return v | s | o | ultimo;
}

void
oam_enderco_escrever (Nes     *nes,
                      uint8_t  valor)
{
  nes->ppu->oam_endereco = valor;
}

void
oam_dados_escrever (Nes     *nes,
                    uint8_t  valor)
{
  Ppu *ppu = nes->ppu;
  ppu->oam[ppu->oam_endereco] = valor;
  ppu->oam_endereco += 1;
}

uint8_t
oam_dados_ler (Nes *nes)
{
  Ppu *ppu = nes->ppu;
  return ppu->oam[ppu->oam_endereco];
}

void
ppu_scroll_escrever (Nes     *nes,
                     uint8_t  valor)
{
  Ppu *ppu = nes->ppu;

  // se o valor de 'w' for 0, estamos na primeira escrita
  // caso não seja, estamos na segunda escrita
  if (ppu->w == false) {
    // t: ....... ...HGFED = d: HGFED...
    // x:              CBA = d: .....CBA
    // w:                  = 1
    ppu->t = (ppu->t & 0b1111111111100000) | (valor >> 3);
    ppu->x = valor & 0b00000111;
    ppu->w = true;
  }
  else {
    // t: CBA..HG FED..... = d: HGFEDCBA
    // w:                  = 0
    ppu->t = ppu->t & 0b0000110000011111;

    uint16_t cba = valor & 0b00000111;
    cba <<= 12;

    uint16_t hgfed = valor & 0b11111000;
    hgfed <<= 2;

    ppu->t = ppu->t | cba | hgfed;

    ppu->w = false;
  }
}

void
ppu_endereco_escrever (Nes     *nes,
                       uint8_t  valor)
{
  Ppu *ppu = nes->ppu;

  // se o valor de 'w' for 0, estamos na primeira escrita
  // caso não seja, estamos na segunda escrita
  if (ppu->w == false) {
    // t: .FEDCBA ........ = d: ..FEDCBA
    // t: X...... ........ = 0
    // w:                  = 1

    // mantem apenas os ultimos 8 bits ativos
    ppu->t = ppu->t & 0b0000000011111111;

    uint16_t fedcba = (valor & 0b00111111);
    fedcba <<= 8;

    ppu->t = ppu->t | fedcba;

    ppu->w = true;
  }
  else {
    // t: ....... HGFEDCBA = d: HGFEDCBA
    // v                   = t
    // w:                  = 0

    // mantem apenas os primeiros 8 bits ativos
    ppu->t = (ppu->t & 0b1111111100000000) | (uint16_t)valor;

    ppu->w = false;
  }
}

void
omd_dma_escrever (Nes     *nes,
                  uint8_t  valor)
{
  Ppu *ppu = nes->ppu;
  uint16_t ponteiro = valor << 8;

  for (int i = 0; i < TAMANHO (ppu->oam); i++) {
    ppu->oam[ppu->oam_endereco] = ler_memoria (nes, ponteiro);
    ppu->oam_endereco += 1;
    ponteiro += 1;
  }

  // se o ciclo for impar
  if ((nes->cpu->ciclos%2) != 0) {
    nes->cpu->esperar = 514;
  }
  else {
    nes->cpu->esperar = 513;
  }
}
