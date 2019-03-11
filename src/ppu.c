#include <stdlib.h>

#include "ppu.h"
#include "rom.h"
#include "memoria.h"
#include "util.h"

Ppu*
ppu_new (void)
{
  Ppu *ppu = malloc (sizeof (Ppu));

  ppu->buffer_dados = 0;
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

  for (int i = 0; i < TAMANHO (ppu->vram); i++) {
    ppu->vram[i] = 0;
  }

  return ppu;
}

void
ppu_free (Ppu *ppu)
{
  free (ppu);
}

uint8_t
ppu_registrador_ler (Nes      *nes,
                     uint16_t  endereco)
{
  switch (endereco) {
  case 0x2002:
    return ppu_estado_ler (nes);

  case 0x2004:
    return oam_dados_ler (nes);

  case 0x2007:
    return ppu_dados_ler (nes);

  default:
    return 0;
  }
}

void
ppu_registrador_escrever (Nes      *nes,
                          uint16_t  endereco,
                          uint8_t   valor)
{
  switch (endereco) {
  case 0x2000:
    ppu_controle_escrever (nes, valor);
    break;

  case 0x2001:
    ppu_mascara_escrever (nes, valor);
    break;

  case 0x2003:
    oam_enderco_escrever (nes, valor);
    break;

  case 0x2005:
    ppu_scroll_escrever (nes, valor);
    break;

  case 0x2006:
    ppu_endereco_escrever (nes, valor);
    break;

  case 0x2007:
    ppu_dados_escrever (nes, valor);
    break;

  default:
    break;
  }
}

uint8_t
ppu_ler (Nes      *nes,
         uint16_t  endereco)
{
  if (endereco < 0x2000) {
    return nes->mapeador->ler (nes, endereco);
  }
  else if (endereco >= 0x2000 && endereco < 0x3F00) {
    uint16_t espelhado = ppu_endereco_espelhado (nes, endereco);
    return nes->ppu->vram[espelhado];
  }
  else if (endereco >= 0x3F00 && endereco < 0x4000) {
    //ler dados das paletas de cores

    if (endereco >= 0x3F20) {
      // espelhar o endereço se necessario
      endereco = (endereco%0x20) + 0x3F00;
    }

    if (endereco == 0x3F10) {
      return nes->ppu->vram[0x3F00];
    }
    else if (endereco == 0x3F14) {
      return nes->ppu->vram[0x3F04];
    }
    else if (endereco == 0x3F18) {
      return nes->ppu->vram[0x3F08];
    }
    else if (endereco == 0x3F1C) {
      return nes->ppu->vram[0x3F0C];
    }
    else {
      return nes->ppu->vram[endereco];
    }
  }

  return 0;
}

void
ppu_escrever (Nes      *nes,
              uint16_t  endereco,
              uint8_t   valor)
{
  if (endereco < 0x2000) {
    nes->mapeador->escrever (nes, endereco, valor);
  }
  else if (endereco >= 0x2000 && endereco < 0x3F00) {
    uint16_t espelhado = ppu_endereco_espelhado (nes, endereco);
    nes->ppu->vram[espelhado] = valor;
  }
  else if (endereco >= 0x3F00 && endereco < 0x4000) {
    //escrever nas paletas de cores

    if (endereco >= 0x3F20) {
      // espelhar o endereço se necessario
      endereco = (endereco%0x20) + 0x3F00;
    }

    if (endereco == 0x3F10) {
      nes->ppu->vram[0x3F00] = valor;
    }
    else if (endereco == 0x3F14) {
      nes->ppu->vram[0x3F04] = valor;
    }
    else if (endereco == 0x3F18) {
      nes->ppu->vram[0x3F08] = valor;
    }
    else if (endereco == 0x3F1C) {
      nes->ppu->vram[0x3F0C] = valor;
    }
    else {
      nes->ppu->vram[endereco] = valor;
    }
  }
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

uint8_t
ppu_dados_ler (Nes *nes)
{
  Ppu *ppu = nes->ppu;

  if (ppu->v < 0x3F00) {
    const uint8_t dados = ppu->buffer_dados;
    ppu->buffer_dados = ler_memoria (nes, ppu->v);
    ppu->v += ppu->vram_incrementar;

    return dados;
  }
  else {
    const uint8_t valor = ler_memoria (nes, ppu->v);
    ppu->buffer_dados = ler_memoria (nes, ppu->v - 0x1000);

    return valor;
  }
}

void
ppu_dados_escrever (Nes     *nes,
                    uint8_t  valor)
{
  Ppu *ppu = nes->ppu;

  ppu_escrever (nes, ppu->v, valor);
  ppu->v += ppu->vram_incrementar;
}

uint16_t
ppu_endereco_espelhado (Nes      *nes,
                        uint16_t  endereco)
{
  uint16_t base = 0;
  switch (nes->rom->espelhamento) {
  case ESPELHAMENTO_HORIZONTAL:
    if (endereco >= 0x2000 && endereco < 0x2400) {
      base = 0x2000;
    }
    else if (endereco >= 0x2400 && endereco < 0x2800) {
      base = 0x2000;
    }
    else if (endereco >= 0x2800 && endereco < 0x2C00) {
      base = 0x2400;
    }
    else {
      base = 0x2400;
    }
    break;

  case ESPELHAMENTO_VERTICAL:
    if (endereco >= 0x2000 && endereco < 0x2400) {
      base = 0x2000;
    }
    else if (endereco >= 0x2400 && endereco < 0x2800) {
      base = 0x2400;
    }
    else if (endereco >= 0x2800 && endereco < 0x2C00) {
      base = 0x2000;
    }
    else {
      base = 0x2400;
    }
    break;

  case ESPELHAMENTO_TELA_UNICA:
    base = 0x2000;
    break;

  case ESPELHAMENTO_4_TELAS:
    if (endereco >= 0x2000 && endereco < 0x2400) {
      base = 0x2000;
    }
    else if (endereco >= 0x2400 && endereco < 0x2800) {
      base = 0x2400;
    }
    else if (endereco >= 0x2800 && endereco < 0x2C00) {
      base = 0x2800;
    }
    else {
      base = 0x2C00;
    }
    break;
  }

  return base | (endereco & 0b0000001111111111);
}
