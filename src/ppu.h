#pragma once

#include <stdbool.h>
#include <stdint.h>

// referencias utilizadas:
// https://wiki.nesdev.com/w/index.php/PPU_registers

#define PPU_OAM_TAM 0x800

typedef struct _Ppu Ppu;

struct _Ppu {
        // Memoria interna da ppu
        uint8_t  oam[PPU_OAM_TAM];
        // último valor escrito na ppu
        uint8_t  ultimo_valor;
        uint16_t vram_incrementar;
        uint16_t nametable_endereco;
        uint16_t padrao_fundo_endereco;
        uint16_t padrao_sprite_endereco;
        uint16_t oam_endereco;

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
        uint8_t  w;
};

Ppu*    ppu_new               (void);

void    ppu_free              (Ppu *ppu);

void    ppu_controle_escrever (Ppu     *ppu,
                               uint8_t  valor);

void    ppu_mascara_escrever  (Ppu     *ppu,
                               uint8_t  valor);

uint8_t ppu_estado_ler        (Ppu *ppu);

void    oam_enderco_escrever  (Ppu     *ppu,
                               uint8_t  valor);

void    oam_dados_escrever    (Ppu     *ppu,
                               uint8_t  valor);

uint8_t oam_dados_ler         (Ppu     *ppu,
                               uint8_t  valor);
