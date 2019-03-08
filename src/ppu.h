#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nesbrasa.h"

// referencias utilizadas:
// https://wiki.nesdev.com/w/index.php/PPU_registers

typedef struct _Nes Nes;
typedef struct _Ppu Ppu;

struct _Ppu {
        uint8_t  oam[0x100];
        uint8_t  nametables[0x800];

        uint8_t  ultimo_valor; // último valor escrito na ppu
        uint16_t vram_incrementar;
        uint16_t oam_endereco;
        uint16_t nametable_endereco;
        uint16_t padrao_fundo_endereco;
        uint16_t padrao_sprite_endereco;

        // PPUCTRL - $2000
        bool     flag_nmi;
        bool     flag_mestre_escravo;
        bool     flag_sprite_altura;
        bool     flag_padrao_fundo;
        bool     flag_padrao_sprite;
        bool     flag_incrementar;
        uint8_t  flag_nametable_base;

        // PPUMASK - $2001
        bool     flag_enfase_b;
        bool     flag_enfase_g;
        bool     flag_enfase_r;
        bool     flag_sprite_habilitar;
        bool     flag_fundo_habilitar;
        bool     flag_sprite_habilitar_col_esquerda;
        bool     flag_fundo_habilitar_col_esquerda;
        bool     flag_escala_cinza;

        // PPUSTATUS - $2002
        bool     flag_vblank;
        bool     flag_sprite_zero;
        bool     flag_sprite_transbordamento;

        // registradores internos
        uint16_t v;
        uint16_t t;
        uint8_t  x;
        bool     w;
};

Ppu*    ppu_new               (void);

void    ppu_free              (Ppu *ppu);

void    ppu_controle_escrever (Nes     *nes,
                               uint8_t  valor);

void    ppu_mascara_escrever  (Nes     *nes,
                               uint8_t  valor);

uint8_t ppu_estado_ler        (Nes *nes);

void    oam_enderco_escrever  (Nes     *nes,
                               uint8_t  valor);

void    oam_dados_escrever    (Nes     *nes,
                               uint8_t  valor);

uint8_t oam_dados_ler         (Nes *nes);

void    ppu_scroll_escrever   (Nes     *nes,
                               uint8_t  valor);

void    ppu_endereco_escrever (Nes     *nes,
                               uint8_t  valor);

void    omd_dma_escrever      (Nes     *nes,
                               uint8_t  valor);
