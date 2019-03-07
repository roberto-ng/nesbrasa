#include "ppu.h"

void
ppu_controle_escrever (Ppu     *ppu,
                       uint8_t  valor)
{
  ppu->flag_nmi = (bool)((valor & 0b10000000) >> 7);
  ppu->flag_mestre_escravo = (bool)((valor & 0b01000000) >> 6);
  ppu->flag_sprite_altura = (bool)((valor & 0b00100000) >> 5);
  ppu->flag_padrao_fundo = (bool)((valor & 0b00010000) >> 4);
  ppu->flag_padrao_sprite = (bool)((valor & 0b00001000) >> 3);
  ppu->flag_incrementar = (bool)((valor & 0b00000100) >> 2);
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
ppu_mascara_escrever (Ppu     *ppu,
                      uint8_t  valor)
{
  ppu->flag_enfase_b = (bool)((valor & 0b10000000) >> 7);
  ppu->flag_enfase_g = (bool)((valor & 0b01000000) >> 6);
  ppu->flag_enfase_r = (bool)((valor & 0b00100000) >> 5);
  ppu->flag_sprite_habilitar = (bool)((valor & 0b00010000) >> 4);
  ppu->flag_fundo_habilitar = (bool)((valor & 0b00001000) >> 3);
  ppu->flag_sprite_habilitar_col_esquerda = (bool)((valor & 0b00000100) >> 2);
  ppu->flag_fundo_habilitar_col_esquerda = (bool)((valor & 0b00000010) >> 1);
  ppu->flag_escala_cinza = (bool)(valor & 0b00000001);
}

uint8_t
ppu_estado_ler (Ppu *ppu)
{
  // os 5 bits menos significativos do último valor escrito na PPU
  const uint8_t ultimo = ppu->ultimo_valor & 0b00011111;

  const uint8_t v = (uint8_t)ppu->flag_vblank << 7;
  const uint8_t s = (uint8_t)ppu->flag_sprite_zero << 6;
  const uint8_t o = (uint8_t)ppu->flag_sprite_transbordamento << 5;

  ppu->flag_vblank = false;
  // w: = 0
  ppu->w = 0;

  return v | s | o | ultimo;
}

