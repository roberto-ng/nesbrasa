/* ppu.h
 *
 * Copyright 2019 Roberto Nazareth <nazarethroberto97@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "nesbrasa.h"

// referencias utilizadas:
// https://wiki.nesdev.com/w/index.php/PPU_registers
// http://nemulator.com/files/nes_emu.txt

typedef struct _Nes Nes;
typedef struct _Ppu Ppu;

struct _Ppu
{
        uint8_t oam[0x100];
        uint8_t vram[0x4000];

        uint8_t  buffer_dados;
        uint8_t  ultimo_valor; // último valor escrito na ppu
        uint16_t vram_incrementar;
        uint16_t oam_endereco;
        uint16_t nametable_endereco;
        uint16_t padrao_fundo_endereco;
        uint16_t padrao_sprite_endereco;

        // PPUCTRL - $2000
        bool    flag_nmi;
        bool    flag_mestre_escravo;
        bool    flag_sprite_altura;
        bool    flag_padrao_fundo;
        bool    flag_padrao_sprite;
        bool    flag_incrementar;
        uint8_t flag_nametable_base;

        // PPUMASK - $2001
        bool flag_enfase_b;
        bool flag_enfase_g;
        bool flag_enfase_r;
        bool flag_sprite_habilitar;
        bool flag_fundo_habilitar;
        bool flag_sprite_habilitar_col_esquerda;
        bool flag_fundo_habilitar_col_esquerda;
        bool flag_escala_cinza;

        // PPUSTATUS - $2002
        bool flag_vblank;
        bool flag_sprite_zero;
        bool flag_sprite_transbordamento;

        // registradores internos
        uint16_t v;
        uint16_t t;
        uint8_t  x;
        bool     w;
};

Ppu* ppu_new(void);

void ppu_free(Ppu *ppu);


uint8_t ppu_ler(Nes *nes, uint16_t endereco);

void ppu_escrever(Nes *nes, uint16_t endereco, uint8_t valor);

uint8_t ppu_registrador_ler(Nes *nes, uint16_t endereco);

void ppu_registrador_escrever(Nes *nes, uint16_t endereco, uint8_t valor);

void ppu_controle_escrever(Nes *nes, uint8_t valor);

void ppu_mascara_escrever(Nes *nes, uint8_t  valor);

uint8_t ppu_estado_ler(Nes *nes);

void oam_enderco_escrever(Nes *nes, uint8_t valor);

void oam_dados_escrever(Nes *nes, uint8_t valor);

uint8_t oam_dados_ler(Nes *nes);

void ppu_scroll_escrever(Nes *nes, uint8_t valor);

void ppu_endereco_escrever (Nes *nes, uint8_t valor);

void omd_dma_escrever(Nes *nes, uint8_t valor);

uint8_t ppu_dados_ler(Nes *nes);

void ppu_dados_escrever(Nes *nes, uint8_t valor);

uint16_t ppu_endereco_espelhado(Nes *nes, uint16_t endereco);
