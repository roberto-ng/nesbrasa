/* ppu.hpp
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

#include <cstdint>
#include <array>
#include <memory>

#include "memoria.hpp"

// referencias utilizadas:
// https://wiki.nesdev.com/w/index.php/PPU_registers
// http://nemulator.com/files/nes_emu.txt

namespace nesbrasa::nucleo
{
    using std::array;
    using std::shared_ptr;
    
    class Ppu
    {
    public:
        enum class Espelhamento
        {
            HORIZONTAL,
            VERTICAL,
            TELA_UNICA,
            QUATRO_TELAS,
        };

        enum class CicloTipo
        {
            ZERO,
            UM,
            VISIVEL,
            PRE_BUSCA,
            CONTINUAR,
            OUTRO,
        };

        enum class ScanLineTipo
        {
            VISIVEL,
            VBLANK,
            PRE_RENDERIZACAO,
            OUTRO,
        };
    
        Espelhamento espelhamento;

    private:
        Memoria* memoria;

        array<byte, 0x100> oam;
        array<byte, 0x800> tabelas_de_nomes;
        array<byte, 0x20>  paletas;

        uint16 ciclo;
        uint16 scanline;
        uint64 frame;

        byte buffer_dados;
        byte ultimo_valor; // último valor escrito na ppu
        uint16 vram_incrementar;
        uint16 oam_endereco;
        uint16 nametable_endereco;
        uint16 padrao_fundo_endereco;
        uint16 padrao_sprite_endereco;
        
        // flags do nmi
        bool nmi_ocorreu;
        bool nmi_anterior;
        bool nmi_output;
        uint nmi_atrasar;

        // PPUCTRL - $2000
        bool flag_nmi;
        bool flag_mestre_escravo;
        bool flag_sprite_altura;
        bool flag_padrao_fundo;
        bool flag_padrao_sprite;
        bool flag_incrementar;
        byte flag_nametable_base;

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
        uint16 v;
        uint16 t;
        byte   x;
        bool   w;
        bool   f;

    public:        
        Ppu(Memoria* memoria);

        void reiniciar();
        void avancar();

        byte ler(Nes *nes, uint16 endereco);
        void escrever(Nes *nes, uint16 endereco, byte valor);

        byte registrador_ler(uint16 endereco);
        void registrador_escrever(Nes *nes, uint16 endereco, byte valor);

        byte ler_paleta(uint16 endereco);
        void escrever_paleta(uint16 endereco, byte valor);

    private:
        CicloTipo get_ciclo_tipo();
        ScanLineTipo get_scanline_tipo();

        void set_controle(byte valor);
        void set_mascara(byte  valor);
        byte get_estado();
        void set_oam_enderco(byte valor);
        void set_oam_dados(byte valor);
        byte get_oam_dados();
        void set_scroll(byte valor);
        void set_endereco(byte valor);
        void set_omd_dma(Nes *nes, byte valor);
        byte get_dados();
        void set_dados(Nes *nes, byte valor);
            
        uint16 endereco_espelhado(uint16 endereco);
    };
}