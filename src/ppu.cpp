/* ppu.cpp
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

#include "ppu.hpp"
#include "nesbrasa.hpp"
#include "memoria.hpp"
#include "util.hpp"
#include "cores.hpp"

namespace nesbrasa::nucleo
{
    const int TELA_LARGURA = 256;
    const int TELA_ALTURA = 240;

    Ppu::Ppu(Memoria* memoria): 
        memoria(memoria),
        tela_dados({ 0 }),
        oam({ 0 }),
        oam_secundaria({}),
        tabelas_de_nomes({ 0 }),
        paletas({ 0 }),
        sprite_indices({ 0 })
    {
        this->espelhamento = Espelhamento::VERTICAL;

        this->ciclo = 0;
        this->scanline = 261;
        this->frame = 0;

        this->buffer_dados = 0;
        this->ultimo_valor = 0;
        this->vram_incrementar = 0;
        this->oam_endereco = 0;
        this->nametable_endereco = 0;
        this->padrao_fundo_endereco = 0;
        this->padrao_sprite_endereco = 0;

        this->tile_dados = 0;
        this->tile_byte_maior = 0;
        this->tile_byte_menor = 0;
        this->tabela_de_nomes_byte = 0;
        this->tabela_de_atributos_byte = 0;

        this->sprites_qtd = 0;

        this->nmi_ocorreu = false;
        this->nmi_anterior = false;
        this->nmi_atrasar = false;
        this->nmi_output = false;

        this->flag_nmi = false;
        this->flag_mestre_escravo = false;
        this->flag_sprite_altura = false;
        this->flag_padrao_fundo = false;
        this->flag_padrao_sprite = false;
        this->flag_incrementar = false;
        this->flag_nametable_base = 0;

        this->flag_enfase_b = false;
        this->flag_enfase_g = false;
        this->flag_enfase_r = false;
        this->flag_sprite_habilitar = false;
        this->flag_fundo_habilitar = false;
        this->flag_sprite_habilitar_col_esquerda = false;
        this->flag_fundo_habilitar_col_esquerda = false;
        this->flag_escala_cinza = false;

        this->flag_vblank = false;
        this->flag_sprite_zero = false;
        this->flag_sprite_transbordamento = false;

        this->v = 0;
        this->t = 0;
        this->x = 0;
        this->w = false;
        this->f = false;

        this->reiniciar();
    }

    void Ppu::reiniciar()
    {
        this->set_oam_enderco(0);
        this->set_controle(0);
        this->set_mascara(0);
        
        this->frame = 0;
        this->ciclo = 340;
        this->scanline = 240;

        this->espelhamento = Espelhamento::VERTICAL;
    }

    void Ppu::avancar()
    {
        auto ciclo_tipo = this->get_ciclo_tipo();
        auto scanline_tipo = this->get_scanline_tipo();

        if (this->nmi_output && this->nmi_ocorreu)
        {
            this->memoria->cpu_ativar_interrupcao(Interrupcao::NMI);
        }

        if (!this->flag_fundo_habilitar || this->flag_sprite_habilitar)
        {
            if (this->f && scanline_tipo == ScanLineTipo::PRE_RENDERIZACAO && this->ciclo == 339)
            {
                this->ciclo = 0;
                this->scanline = 0;
                this->frame += 1;
                this->f = !this->f;
                
                return;
            }
        }

        if (ciclo_tipo == CicloTipo::CONTINUAR)
        {
            this->ciclo = 0;
            ciclo_tipo = this->get_ciclo_tipo();

            if (scanline_tipo == ScanLineTipo::PRE_RENDERIZACAO)
            {
                this->scanline = 0;
                scanline_tipo = this->get_scanline_tipo();

                this->frame += 1;
                this->f = !this->f;
            }
            else
            {
                this->scanline += 1;
                scanline_tipo = this->get_scanline_tipo();
            }
        }
        else
        {
            this->ciclo += 1;
            ciclo_tipo = this->get_ciclo_tipo();
        }
        
        // se a renderização estiver habilitada
        if (this->flag_fundo_habilitar || this->flag_sprite_habilitar)
        {
            if (this->ciclo == 257 && scanline_tipo == ScanLineTipo::VISIVEL)
            {
                // avaliar sprites
                this->avaliar_sprites();
            }

            if (ciclo_tipo == CicloTipo::VISIVEL && scanline_tipo == ScanLineTipo::VISIVEL)
            {
                this->renderizar_pixel();
            }

            if (scanline_tipo == ScanLineTipo::PRE_RENDERIZACAO || scanline_tipo == ScanLineTipo::VISIVEL)
            {
                if (ciclo_tipo == CicloTipo::PRE_BUSCA || ciclo_tipo == CicloTipo::VISIVEL)
                {
                    this->tile_dados <<= 4;

                    switch (this->ciclo % 8)
                    {
                        case 1:
                            // buscar byte da tabela de nomes
                            this->buscar_byte_tabela_de_nomes();
                            break;

                        case 3:
                            // buscar byte da tabela de atributos
                            this->buscar_byte_tabela_de_atributos();
                            break;

                        case 5:
                            // buscar o byte de menor significancia do tile
                            this->buscar_tile_byte_menor();
                            break;
                        
                        case 7:
                            // buscar o byte de maior significancia do tile
                            this->buscar_tile_byte_maior();
                            break;
                        
                        case 0:
                            // guardar os dados do tile
                            this->tile_guardar_dados();
                            this->mudar_scroll_x();
                            break;

                        default:
                            break;
                    }
                }

                if (this->ciclo == 256)
                {
                    this->mudar_scroll_y();
                }
                else if (this->ciclo == 257)
                {
                    this->copiar_x();
                }
            }

            if (ciclo_tipo == CicloTipo::COPIAR_Y)
            {
                this->copiar_y();
            }
        }

        if (scanline_tipo == ScanLineTipo::VBLANK && ciclo_tipo == CicloTipo::UM)
        {
            this->executar_ciclo_vblank();
        }
        else if (scanline_tipo == ScanLineTipo::PRE_RENDERIZACAO && ciclo_tipo == CicloTipo::UM)
        {
            // encerrar vblank
            this->encerrar_ciclo_vblank();
            this->flag_sprite_zero = 0;
            this->flag_sprite_transbordamento = 0;
        }

    }

    byte Ppu::registrador_ler(uint16 endereco)
    {
        switch (endereco)
        {
            case 0x2002:
                return this->get_estado();

            case 0x2004:
                return this->get_oam_dados();

            case 0x2007:
                return this->get_dados();

            default:
                return 0;
        }
    }

    void Ppu::registrador_escrever(Nes *nes, uint16 endereco, byte valor)
    {
        switch (endereco)
        {
            case 0x2000:
                this->set_controle(valor);
                break;

            case 0x2001:
                this->set_mascara(valor);
                break;

            case 0x2003:
                this->set_oam_enderco(valor);
                break;

            case 0x2005:
                this->set_scroll(valor);
                break;

            case 0x2006:
                this->set_endereco(valor);
                break;

            case 0x2007:
                this->set_dados(nes, valor);
                break;
            
            case 0x4014:
                this->set_omd_dma(nes, valor);
                break;

            default:
                break;
        }
    }

    byte Ppu::ler(Nes *nes, uint16 endereco)
    {
        if (endereco < 0x2000)
        {
            return nes->cartucho->ler(endereco);
        }
        else if (endereco >= 0x2000 && endereco < 0x3F00)
        {
            uint16 endereco_espelhado = this->endereco_espelhado(endereco);
            uint16 posicao = endereco_espelhado % this->tabelas_de_nomes.size();

            return this->tabelas_de_nomes.at(posicao);
        }
        else if (endereco >= 0x3F00 && endereco < 0x4000)
        {
            //ler dados das paletas de cores
            this->ler_paleta(endereco % this->paletas.size());
        }

        return 0;
    }

    void Ppu::escrever(Nes *nes, uint16 endereco, byte valor)
    {
        if (endereco < 0x2000)
        {
            nes->cartucho->escrever(endereco, valor);
        }
        else if (endereco >= 0x2000 && endereco < 0x3F00)
        {
            uint16 endereco_espelhado = this->endereco_espelhado(endereco);
            uint16 posicao = endereco_espelhado % this->tabelas_de_nomes.size();
            
            this->tabelas_de_nomes.at(posicao) = valor;
        }
        else if (endereco >= 0x3F00 && endereco < 0x4000)
        {
            //escrever nas paletas de cores
            this->escrever_paleta(endereco % this->paletas.size(), valor);
        }
    }

    byte Ppu::ler_paleta(uint16 endereco)
    {
        return this->paletas[endereco];
    }

    void Ppu::escrever_paleta(uint16 endereco, byte valor)
    {
        this->paletas.at(endereco) = valor;
    }

    array<byte, (TELA_LARGURA*TELA_ALTURA*3)> Ppu::gerar_textura_rgb()
    {
        const uint pixels_quantidade = TELA_LARGURA*TELA_ALTURA;

        array<byte, (pixels_quantidade*3)> textura_rgb;
        for (uint i = 0; i < pixels_quantidade; i++)
        {
            byte cor = this->tela_dados.at(i);
            auto cor_rgb = cores::buscar_cor_rgb(cor);
            for (uint j = 0; j < cor_rgb.size(); j++)
            {
                textura_rgb.at(i*3 + j) = cor_rgb.at(j);
            }
        }

        return textura_rgb;
    }

    Ppu::CicloTipo Ppu::get_ciclo_tipo()
    {
        if (this->ciclo == 0)
        {
            return Ppu::CicloTipo::ZERO;
        }
        else if (this->ciclo == 1)
        {
            return Ppu::CicloTipo::UM;
        }
        else if (this->ciclo >= 2 && this->ciclo <= 256)
        {
            return Ppu::CicloTipo::VISIVEL;
        }
        else if (this->ciclo >= 280 && this->ciclo <= 304)
        {
            return CicloTipo::COPIAR_Y;
        }
        else if (this->ciclo >= 321 && this->ciclo <= 336)
        {
            return Ppu::CicloTipo::PRE_BUSCA;
        }
        else if (this->ciclo == 340)
        {
            return Ppu::CicloTipo::CONTINUAR;
        }
        else
        {
            return Ppu::CicloTipo::OUTRO;
        }
    }

    Ppu::ScanLineTipo Ppu::get_scanline_tipo()
    {
        if (this->scanline < 240)
        {
            return ScanLineTipo::VISIVEL;
        }
        else if (this->scanline == 241)
        {
            return ScanLineTipo::VBLANK;
        }
        else if (this->scanline == 261)
        {
            return ScanLineTipo::PRE_RENDERIZACAO;
        }
        else
        {
            return ScanLineTipo::OUTRO;
        }
    }

    byte Ppu::buscar_pixel_fundo()
    {
        if (!this->flag_fundo_habilitar)
            return 0;

        int pos_x = this->ciclo - 1;
        if (!this->flag_fundo_habilitar_col_esquerda && pos_x < 8)
            return 0;
        
        uint32 tile = static_cast<uint32>(this->tile_dados >> 32);
        byte cor = static_cast<byte>((tile >> (this->x*4)) & 0x0F);
        return cor;
    }

    byte Ppu::buscar_pixel_sprite(uint& indice)
    {
        indice = 0;

        if (this->flag_sprite_habilitar == 0)
            return 0;

        int pos_x = static_cast<int>(this->ciclo) - 1;
        int pos_y = static_cast<int>(this->scanline) - 1;
        if (!this->flag_fundo_habilitar_col_esquerda && pos_x < 8)
            return 0;

        uint32 endereco_padrao_atual = this->padrao_sprite_endereco;
        for (uint i = 0; i < this->sprites_qtd; i++)
        {
            const uint pos = i*4 + 1;
            byte altura = 8 - 1;
            byte largura = 8 - 1;

            int offset_x = pos_x - static_cast<int>(this->oam_secundaria.at(i + 3));
            if (offset_x < 0 || offset_x >= 8)
                continue;

            int offset_y = pos_y - static_cast<int>(this->oam_secundaria.at(i));

            byte padrao_indice = this->oam_secundaria.at(pos);
            if (this->flag_sprite_altura)
            {
                altura = 16 - 1;
                endereco_padrao_atual = (this->oam_secundaria.at(pos) & 1) * 0x1000;
                padrao_indice &= 0b11111110;
            }

            uint endereco_padrao = endereco_padrao_atual + padrao_indice*16;

            bool inverter_horizontal = false;
            bool inverter_vertical = false;            
            if ((this->oam_secundaria.at(pos+1) & 0b01000000) != 0)
            {
                inverter_horizontal = true;
            }
            if ((this->oam_secundaria.at(pos+1) & 0b10000000) != 0)
            {
                inverter_vertical = true;
            }

            if (inverter_horizontal)
            {
                offset_x = largura - offset_x;
            }
            if (inverter_vertical)
            {
                offset_y = altura - offset_y;
            }

            uint endereco_y = endereco_padrao + offset_y;
            if (offset_y >= 8)
            {
                endereco_y += 8;
            }

            const byte byte_1 = this->memoria->ler(endereco_y);
            const byte byte_2 = this->memoria->ler(endereco_y + 8);
            const byte bit_menor = (byte_1 >> (7-offset_x)) & 1;
            const byte bit_maior = (byte_2 >> (7-offset_x)) & 1;
            const byte cor_num = ((bit_maior << 1) | bit_menor) & 0b00000011;

            if (cor_num != 0)
            {
                indice = i;
                byte paleta_num = this->oam_secundaria.at(pos+1) & 0b00000011;
                return ((paleta_num << 2) | cor_num) & 0b00001111;
            }
            else
            {
                continue;
            }
        }

        return 0;
    }

    byte Ppu::buscar_cor_fundo(byte valor)
    {
        // indice da cor
        uint cor_pos = (valor & 0b00000011) - 1;
        // indice da paleta
        uint paleta_pos = (valor >> 2) & 0b00000011;

        uint16 paleta_endereco = 0;
        if (cor_pos + 1 == 0)
        {
            paleta_endereco = (0x3F00);
        }
        else
        {
            switch (paleta_pos)
            {
                case 0:
                    paleta_endereco = 0x3F01 + cor_pos;
                    break;
                
                case 1:
                    paleta_endereco = 0x3F05 + cor_pos;
                    break;

                case 2:
                    paleta_endereco = 0x3F09 + cor_pos;
                    break;

                case 3:
                    paleta_endereco = 0x3F0D + cor_pos;
                    break;
                
                default:
                    throw std::runtime_error("Paleta de fundo inválida");
                    break;
            }
        }

        return this->memoria->ler(paleta_endereco);
    }

    byte Ppu::buscar_cor_sprite(byte valor)
    {
        // indice da cor
        uint cor_pos = (valor & 0b00000011) - 1;
        // indice da paleta
        uint paleta_pos = (valor >> 2) & 0b00000011;

        uint16 paleta_endereco = 0;
        if (cor_pos + 1 == 0)
        {
            paleta_endereco = (0x3F00);
        }
        else
        {
            switch (paleta_pos)
            {
                case 0:
                    paleta_endereco = 0x3F11 + cor_pos;
                    break;
                
                case 1:
                    paleta_endereco = 0x3F15 + cor_pos;
                    break;

                case 2:
                    paleta_endereco = 0x3F19 + cor_pos;
                    break;

                case 3:
                    paleta_endereco = 0x3F1D + cor_pos;
                    break;
                
                default:
                    throw std::runtime_error("Paleta de sprite inválida");
                    break;
            }
        }

        return this->memoria->ler(paleta_endereco);
    }

    void Ppu::renderizar_pixel()
    {
        uint indice = 0;
        byte pixel_fundo = this->buscar_pixel_fundo();
        byte pixel_sprite = this->buscar_pixel_sprite(indice);

        byte cor = 0;
        if (pixel_sprite == 0)
        {
            cor = this->buscar_cor_fundo(pixel_fundo);
        }
        else if (pixel_sprite != 0 && pixel_fundo == 0)
        {
            cor = this->buscar_cor_sprite(pixel_sprite);
        }
        else if (pixel_sprite != 0 && pixel_fundo != 0)
        {
            if (this->sprite_indices.at(indice) == 0)
            {
                this->flag_sprite_zero = true;
            }

            int prioridade = ((this->oam_secundaria.at(indice*4)+2) >> 5) & 0b00000001;
            if (prioridade == 1)
            {
                cor = this->buscar_cor_fundo(pixel_fundo);
            }
            else
            {
                cor = this->buscar_cor_sprite(pixel_sprite);
            }
        }

        this->tela_dados.at(this->scanline*256 + this->ciclo-1) = cor;
    }

    void Ppu::executar_ciclo_vblank()
    {
        this->nmi_ocorreu = true;
        if (this->nmi_output)
        {
            this->nmi_anterior = true;
        }
        else
        {
            this->nmi_anterior = false;
        }
    }

    void Ppu::encerrar_ciclo_vblank()
    {
        this->nmi_ocorreu = false;
        this->nmi_anterior = false;
    }

    void Ppu::buscar_byte_tabela_de_nomes()
    {
        uint16 endereco = 0b0010000000000000 | (this->v  & 0x0FFF);
        this->tabela_de_nomes_byte = this->memoria->ler(endereco);
    }

    void Ppu::buscar_byte_tabela_de_atributos()
    {
        uint16 endereco = 0b0010001111000000 | (this->v  & 0b0000110000000000);
        endereco |= ((this->v >> 4) & 0b00111000); 
        endereco |= ((this->v >> 2) & 0b00000111);

        uint16 shift = ((this->v >> 4) & 0b00000100) | (this->v & 0b00000010);

        byte valor = this->memoria->ler(endereco);
        this->tabela_de_atributos_byte = ((valor >>  shift) & 0b00000011) << 2;
    }

    void Ppu::buscar_tile_byte_menor()
    {
        const uint16 y = (this->v >> 12) & 0b00000111;
        const uint16 tile = this->tabela_de_nomes_byte;

        uint16 endereco = tile * 16 + y;
        if (this->flag_padrao_fundo)
        {
            endereco += 0x1000;
        }

        this->tile_byte_menor = this->memoria->ler(endereco);
    }

    void Ppu::buscar_tile_byte_maior()
    {
        const uint16 y = (this->v >> 12) & 0b00000111;
        const uint16 tile = this->tabela_de_nomes_byte;

        uint16 endereco = tile * 16 + y + 8;
        if (this->flag_padrao_fundo)
        {
            endereco += 0x1000;
        }

        this->tile_byte_maior = this->memoria->ler(endereco);
    }

    void Ppu::tile_guardar_dados()
    {
        uint32 valor = 0;
        for (int i = 0; i < 8; i++)
        {
            byte byte_1 = (this->tile_byte_menor & 0b10000000) >> 7;
            byte byte_2 = (this->tile_byte_maior & 0b10000000) >> 6;

            this->tile_byte_menor <<= 1;
            this->tile_byte_maior <<= 1;

            valor <<= 4;
            valor |= static_cast<uint32>(this->tabela_de_atributos_byte);
            valor |= static_cast<uint32>(byte_1);
            valor |= static_cast<uint32>(byte_2);
        }

        this->tile_dados |= static_cast<uint64>(valor);
    }

    void Ppu::avaliar_sprites()
    {
        int altura = 8;
        if (this->flag_sprite_altura)
        {
            altura = 16;
        }

        this->sprites_qtd = 0;
        for (int i = this->oam_endereco; i < (256/4); i++)
        {
            uint indice = i*4;
            uint pos_y = this->oam_secundaria.at(indice);
            //uint atrib = this->oam_secundaria.at(indice+2);
            //uint pos_x = this->oam_secundaria.at(indice+3);
            
            int linha = static_cast<int16>(this->scanline) - static_cast<int16>(pos_y);
            if (linha >= 0 && linha < altura)
            {
                if (this->sprites_qtd >= 8)
                {
                    this->sprites_qtd = 8;
                    this->flag_sprite_transbordamento = true;
                    break;
                }
                else
                {
                    for (int j = 0; j < 4; j++)
                    {
                        int pos = this->sprites_qtd * 4;
                        this->oam_secundaria.at(pos + j) = this->oam.at(indice + j);
                    }

                    this->sprite_indices.at(this->sprites_qtd) = (indice - oam_endereco)/4;
                    this->sprites_qtd += 1;
                }
            }
        }
    }

    void Ppu::copiar_x()
    {
        // v: ....F.. ...EDCBA = t: ....F.. ...EDCBA
        this->v = (this->v & 0b1111101111100000) | (this->t & 0b0000010000011111);
    }

    void Ppu::copiar_y()
    {
        // v: IHGF.ED CBA..... = t: IHGF.ED CBA.....
        this->v = (this->v & 0b1000010000011111) | (this->t & 0b0111101111100000);
    }

    void Ppu::mudar_scroll_x()
    {
        if ((this->v & 0b00011111) == 31)
        {
            // x = 0
            this->v &= 0b1111111111100000;
            this->v ^= 0x0400;
        }
        else
        {
            this->v += 1;
        }
    }

    void Ppu::mudar_scroll_y()
    {
        if ((this->v & 0x7000) != 0x7000)
        {
            this->v += 0x1000;
        }
        else
        {
            this->v &= 0b1000111111111111;
            
            uint16 y = (this->v & 0b0000001111100000) >> 5;
            if (y == 29)
            {
                y = 0;
                this->v ^= 0x0800;
            }
            else if (y == 31)
            {
                y = 0;
            }
            else
            {
                y += 1;
            }

            this->v = (this->v & 0b1111110000011111) | (y << 5);
        }
    }

    void Ppu::set_controle(byte valor)
    {
        this->flag_nmi = buscar_bit(valor, 7);
        this->flag_mestre_escravo = buscar_bit(valor, 6);
        this->flag_sprite_altura = buscar_bit(valor, 5);
        this->flag_padrao_fundo = buscar_bit(valor, 4);
        this->flag_padrao_sprite = buscar_bit(valor, 3);
        this->flag_incrementar = buscar_bit(valor, 2);
        this->flag_nametable_base = valor & 0b00000011;

        this->nametable_endereco = 0x2000 + (0x400 * this->flag_nametable_base);

        if (this->flag_incrementar)
            this->vram_incrementar = 32;
        else
            this->vram_incrementar = 1;

        if (this->flag_padrao_sprite)
            this->padrao_sprite_endereco = 0x1000;
        else
            this->padrao_sprite_endereco = 0x0000;

        if (this->flag_padrao_fundo)
            this->padrao_fundo_endereco = 0x1000;
        else
            this->padrao_fundo_endereco = 0x0000;

        // t: ...BA.. ........ = d: ......BA
        this->t = (this->t & 0b1111001111111111) | ((valor & 0b00000011) << 10);
    }

    void Ppu::set_mascara(byte valor)
    {
        this->flag_enfase_b = buscar_bit(valor, 7);
        this->flag_enfase_g = buscar_bit(valor, 6);
        this->flag_enfase_r = buscar_bit(valor, 5);
        this->flag_sprite_habilitar = buscar_bit(valor, 4);
        this->flag_fundo_habilitar = buscar_bit(valor, 3);
        this->flag_sprite_habilitar_col_esquerda = buscar_bit(valor, 2);
        this->flag_fundo_habilitar_col_esquerda = buscar_bit(valor, 1);
        this->flag_escala_cinza = buscar_bit(valor, 0);
    }

    byte Ppu::get_estado()
    {
        // os 5 ultimos bits do último valor escrito na PPU
        const byte ultimo = this->ultimo_valor & 0b00011111;

        const byte v = (byte)this->flag_vblank << 7;
        const byte s = (byte)this->flag_sprite_zero << 6;
        const byte o = (byte)this->flag_sprite_transbordamento << 5;

        this->flag_vblank = false;
        // w: = 0
        this->w = 0;

        return v | s | o | ultimo;
    }

    void Ppu::set_oam_enderco(byte valor)
    {
        this->oam_endereco = valor;
    }

    void Ppu::set_oam_dados(byte valor)
    {
        this->oam.at(this->oam_endereco) = valor;
        this->oam_endereco += 1;
    }

    byte Ppu::get_oam_dados()
    {
        return this->oam.at(this->oam_endereco);
    }

    void Ppu::set_scroll(byte valor)
    {
        // se o valor de 'w' for 0, estamos na primeira escrita
        // caso não seja, estamos na segunda escrita
        if (this->w == false)
        {
            // t: ....... ...HGFED = d: HGFED...
            // x:              CBA = d: .....CBA
            // w:                  = 1
            this->t = (this->t & 0b1111111111100000) | (valor >> 3);
            this->x = valor & 0b00000111;
            this->w = true;
        }
        else
        {
            // t: CBA..HG FED..... = d: HGFEDCBA
            // w:                  = 0
            this->t = this->t & 0b0000110000011111;

            uint16 cba = valor & 0b00000111;
            cba <<= 12;

            uint16 hgfed = valor & 0b11111000;
            hgfed <<= 2;

            this->t = this->t | cba | hgfed;

            this->w = false;
        }
    }

    void Ppu::set_endereco(byte valor)
    {
        // se o valor de 'w' for 0, estamos na primeira escrita
        // caso não seja, estamos na segunda escrita
        if (this->w == false)
        {
            // t: .FEDCBA ........ = d: ..FEDCBA
            // t: X...... ........ = 0
            // w:                  = 1

            // mantem apenas os ultimos 8 bits ativos
            this->t = this->t & 0b0000000011111111;

            uint16 fedcba = (valor & 0b00111111);
            fedcba <<= 8;

            this->t = this->t | fedcba;

            this->w = true;
        }
        else
        {
            // t: ....... HGFEDCBA = d: HGFEDCBA
            // v                   = t
            // w:                  = 0

            // mantem apenas os primeiros 8 bits ativos
            this->t = (this->t & 0b1111111100000000) | (uint16)valor;

            this->w = false;
        }
    }

    void Ppu::set_omd_dma(Nes *nes, byte valor)
    {
        uint16 ponteiro = valor << 8;

        for (uint32_t i = 0; i < this->oam.size(); i++)
        {
            this->oam.at(this->oam_endereco) = this->memoria->ler(ponteiro + i);
            this->oam_endereco += 1;
        }

        // se o ciclo for impar
        if ((nes->cpu.get_ciclos() % 2) != 0)
        {
            nes->cpu.esperar_adicionar(514);
        }
        else
        {
            nes->cpu.esperar_adicionar(513);
        }
    }

    byte Ppu::get_dados()
    {
        if (this->v < 0x3F00)
        {
            const byte dados = this->buffer_dados;
            this->buffer_dados = this->memoria->ler(this->v);
            this->v += this->vram_incrementar;

            return dados;
        }
        else
        {
            const byte valor = this->memoria->ler(this->v);
            this->buffer_dados = this->memoria->ler(this->v - 0x1000);

            return valor;
        }
    }

    void Ppu::set_dados(Nes *nes, byte valor)
    {
        this->escrever(nes, this->v, valor);
        this->v += this->vram_incrementar;
    }

    uint16 Ppu::endereco_espelhado(uint16 endereco)
    {
        uint16 base = 0;
        switch (this->espelhamento)
        {
            case Espelhamento::HORIZONTAL:
                if (endereco >= 0x2000 && endereco < 0x2400)
                {
                    base = 0x2000;
                }
                else if (endereco >= 0x2400 && endereco < 0x2800)
                {
                    base = 0x2000;
                }
                else if (endereco >= 0x2800 && endereco < 0x2C00)
                {
                    base = 0x2400;
                }
                else
                {
                    base = 0x2400;
                }
                break;

            case Espelhamento::VERTICAL:
                if (endereco >= 0x2000 && endereco < 0x2400)
                {
                    base = 0x2000;
                }
                else if (endereco >= 0x2400 && endereco < 0x2800)
                {
                    base = 0x2400;
                }
                else if (endereco >= 0x2800 && endereco < 0x2C00)
                {
                    base = 0x2000;
                }
                else
                {
                    base = 0x2400;
                }
                break;

            case Espelhamento::TELA_UNICA:
                base = 0x2000;
                break;

            case Espelhamento::QUATRO_TELAS:
                if (endereco >= 0x2000 && endereco < 0x2400)
                {
                    base = 0x2000;
                }
                else if (endereco >= 0x2400 && endereco < 0x2800)
                {
                    base = 0x2400;
                }
                else if (endereco >= 0x2800 && endereco < 0x2C00)
                {
                    base = 0x2800;
                }
                else
                {
                    base = 0x2C00;
                }
                break;
        }

        return base | (endereco & 0b0000001111111111);
    }
}
