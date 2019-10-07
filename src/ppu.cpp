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
#include <iostream>

namespace nesbrasa::nucleo
{
    const int TELA_LARGURA = 256;
    const int TELA_ALTURA = 240;

    Ppu::Ppu(Memoria* memoria): 
        memoria(memoria),
        oam({ 0 }),
        sprites_padroes({ 0 }),
        sprites_posicoes({ 0 }),
        sprites_prioridades({ 0 }),
        sprites_indices({ 0 }),
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
        this->frame = 0;
        this->ciclo = 340;
        this->scanline = 240;

        this->set_oam_enderco(0);
        this->set_controle(0);
        this->set_mascara(0);
        
        this->espelhamento = Espelhamento::VERTICAL;
    }

    void Ppu::atualizar()
    {
        if (nmi_atrasar > 0)
        {
            nmi_atrasar -= 1;
            if (nmi_atrasar == 0 && this->nmi_output && this->nmi_ocorreu)
            {
                //std::cout << "OI NMI\n";
                this->memoria->cpu_ativar_interrupcao(Interrupcao::NMI);
            }
        }

        if (this->flag_fundo_habilitar || this->flag_sprite_habilitar)
        {
            if (this->f == 1 && this->scanline == 261 && this->ciclo == 339)
            {
                this->ciclo = 0;
                this->scanline = 0;
                this->frame += 1;
                this->f ^= 1;
                
                return;
            }
        }

        this->ciclo += 1;
        if (this->ciclo > 340)
        {
            this->ciclo = 0;
            this->scanline += 1;

            if (this->scanline > 261)
            {
                this->scanline = 0;
                this->frame += 1;
                this->f ^= 1;
            }
        }
    }

    void Ppu::avancar()
    {
        this->atualizar();

        bool renderizacao_habilitada = this->flag_fundo_habilitar || this->flag_sprite_habilitar;
        bool prelinha = this->scanline == 261;
        bool linha_visivel = this->scanline < 240;
        bool linha_renderizacao = prelinha || linha_visivel;
        bool ciclo_pre_busca = this->ciclo >= 321 && this->ciclo <= 336;
        bool ciclo_visivel = this->ciclo >= 1 && this->ciclo <= 256;
        bool ciclo_busca = ciclo_pre_busca || ciclo_visivel;
        
        // se a renderização estiver habilitada
        if (renderizacao_habilitada)
        {
            if (linha_visivel && ciclo_visivel)
            {
                this->renderizar_pixel();
            }

            if (linha_renderizacao && ciclo_busca)
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
                        break;

                    default:
                        break;
                }
            }

            if (prelinha && this->ciclo >= 280 && this->ciclo <= 304)
            {
                this->copiar_y();
            }

            if (linha_renderizacao)
            {
                if (ciclo_busca && this->ciclo%8 == 0)
                {
                    this->mudar_scroll_x();
                }

                if (this->ciclo == 256)
                    this->mudar_scroll_y();

                if (this->ciclo == 257)
                    this->copiar_x();
            }
        }

        if (renderizacao_habilitada)
        {
            if (this->ciclo == 257)
            {
                if (linha_visivel)
                {
                    // avaliar sprites
                    this->avaliar_sprites();
                }
                else
                {
                    this->sprites_qtd = 0;
                }
            }
        }

        if (this->scanline == 241 && this->ciclo == 1)
        {
            this->executar_ciclo_vblank();
        }
        if (prelinha && this->ciclo == 1)
        {
            this->encerrar_ciclo_vblank();
            this->flag_sprite_zero = false;
            this->flag_sprite_transbordamento = false;
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
        this->ultimo_valor = valor;
        switch (endereco)
        {
            case 0x2000:
                this->set_controle(valor);
                break;

            case 0x2001:
                //std::cout << "Escrevendo r endereco: $" << std::hex << endereco << " val: $" << std::hex << (int)valor << "\n";
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
        endereco = endereco % 0x4000;

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
        endereco = endereco % 0x4000;
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
        if (endereco >= 16 && endereco%4 == 0)
		    endereco -= 16;

        return this->paletas.at(endereco);
    }

    void Ppu::escrever_paleta(uint16 endereco, byte valor)
    {
        if (endereco >= 16 && endereco%4 == 0)
            endereco -= 16;

        this->paletas.at(endereco) = valor;
    }

    array<byte, (TELA_LARGURA*TELA_ALTURA*3)> Ppu::gerar_textura_rgb()
    {
        const uint pixels_quantidade = TELA_LARGURA*TELA_ALTURA;

        array<byte, (pixels_quantidade*3)> textura_rgb;
        for (uint i = 0; i < pixels_quantidade; i++)
        {
            byte cor = this->frente.at(i);
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
        
        uint32 tile = static_cast<uint32>(this->tile_dados >> 32);
        uint32 cor = tile >> ((7 - this->x) * 4);
        return static_cast<byte>(cor & 0x0F);
    }

    byte Ppu::buscar_pixel_sprite(uint& indice)
    {
        if (!flag_sprite_habilitar)
        {
            indice = 0;
            return 0;
        }

        for (int i = 0; i < this->sprites_qtd; i++)
        {
            auto offset = (static_cast<int>(this->ciclo) - 1) - static_cast<int>(this->sprites_posicoes.at(i));
            if (offset < 0 || offset > 7)
                continue;

            offset = 7 - offset;
            byte cor = static_cast<byte>((this->sprites_padroes.at(i) >> static_cast<byte>(offset*4)) & 0x0F);
            if (cor%4 == 0)
                continue;

            indice = i;
            return cor;
        }

        indice = 0;
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

    uint32 Ppu::buscar_padrao_sprite(int i, int linha)
    {
        uint16 tile = this->oam.at(i*4 + 1);
        byte atributos = this->oam.at(i*4 + 2);

        uint16 endereco = 0;
        if (!this->flag_sprite_altura)
        {
            if (atributos%0x80 == 0x80)
            {
                linha = 7 - linha;
            }

            uint16 padrao = this->flag_padrao_sprite ? 1 : 0;
            endereco = 0x1000*padrao + tile*16 + linha;
        }
        else
        {
            if (atributos%0x80 == 0x80)
            {
                linha = 15 - linha;
            }

            uint16 padrao = tile & 1;
            tile &= 0xFE;
            if (linha > 7)
            {
                tile += 1;
                linha -= 8;
            }
            endereco = 0x1000*padrao + tile*16 + linha;
        }

        auto a = (atributos & 3) << 2;
        byte tile_byte_menor = this->ler(this->memoria->nes, endereco);
        byte tile_byte_maior = this->ler(this->memoria->nes, endereco+8);

        uint32 valor = 0;
        for (int i = 0; i < 8; i++)
        {
            byte p1 = 0;
            byte p2 = 0;
            if (atributos&0x40 == 0x40)
            {
                p1 = (tile_byte_menor & 1) << 0;
                p2 = (tile_byte_maior & 1) << 1;
                tile_byte_menor >>= 1;
                tile_byte_maior >>= 1;
            }
            else
            {
                p1 = (tile_byte_menor & 0x80) >> 7;
                p2 = (tile_byte_maior & 0x80) >> 6;
                tile_byte_menor <<= 1;
                tile_byte_maior <<= 1;
            }

            valor << 4;
            valor |= static_cast<uint32>(a | p1 | p2);
        }

        return valor;
    }

    void Ppu::renderizar_pixel()
    {
        int x = this->ciclo - 1;
        int y = this->scanline;
        uint indice = 0;
        byte fundo = this->buscar_pixel_fundo();
        auto sprite = this->buscar_pixel_sprite(indice);

        if (x < 8 && !this->flag_fundo_habilitar_col_esquerda)
            fundo = 0;
        if (x < 8 && !this->flag_sprite_habilitar_col_esquerda)
            sprite = 0;

        bool f = fundo%4 != 0;
        bool s = sprite%4 != 0;
        byte cor = 0;
        if (!f && !s)
        {
            cor = 0;
        }
        else if (!f && s)
        {
            cor = sprite | 0x10;
        }
        else if (f && !s)
        {
            cor = fundo;
        }
        else
        {
            if (this->sprites_indices.at(indice) == 0 && x < 255)
            {
                this->flag_sprite_zero = true;
            }
            
            if (this->sprites_prioridades.at(indice) == 0)
            {
                cor = sprite | 0x10;
            }
            else
            {
                cor = fundo;
            }
        }
        
        auto cor_nes = this->ler_paleta(static_cast<uint16>(cor)%64);
        this->set_textura_valor(this->fundo, x, y, cor_nes);
    }

    void Ppu::executar_ciclo_vblank()
    {
        auto aux = this->frente;
        this->frente = this->fundo;
        this->fundo = aux;

        this->nmi_ocorreu = true;
        this->alterar_nmi();
    }

    void Ppu::encerrar_ciclo_vblank()
    {
        this->nmi_ocorreu = false;
        this->alterar_nmi();
    }

    void Ppu::alterar_nmi()
    {
        bool nmi = this->nmi_output && this->nmi_ocorreu;
        if (nmi && !this->nmi_anterior)
        {
            //std::cout << "ALTERAR NMI\n";
            this->nmi_atrasar = 15;
        }
        this->nmi_anterior = nmi;
    }

    void Ppu::buscar_byte_tabela_de_nomes()
    {
        uint16 v = this->v;
	    uint16 endereco = 0x2000 | (v & 0x0FFF);
	    this->tabela_de_nomes_byte = this->ler(this->memoria->nes, endereco);
    }

    void Ppu::buscar_byte_tabela_de_atributos()
    {
        uint16 v = this->v;
        uint16 endereco = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
        uint16 shift = ((v >> 4) & 4) | (v & 2);
        this->tabela_de_atributos_byte = ((this->ler(this->memoria->nes, endereco) >> shift) & 3) << 2;
    }

    void Ppu::buscar_tile_byte_menor()
    {
        uint16 y = (this->v >> 12) & 7;
        uint16 table = this->flag_padrao_fundo;
        uint16 tile = this->tabela_de_nomes_byte;
        uint16 endereco = 0x1000*static_cast<uint16>(table) + static_cast<uint16>(tile)*16 + y;
        this->tile_byte_menor = this->ler(this->memoria->nes, endereco);
    }

    void Ppu::buscar_tile_byte_maior()
    {
        uint16 y = (this->v >> 12) & 7;
        uint16 table = this->flag_padrao_fundo;
        uint16 tile = this->tabela_de_nomes_byte;
        uint16 endereco = 0x1000*static_cast<uint16>(table) + static_cast<uint16>(tile)*16 + y;
        this->tile_byte_maior = this->ler(this->memoria->nes, endereco+8);
    }

    void Ppu::tile_guardar_dados()
    {
        uint32 valor = 0;
        for (int i = 0; i < 8; i++)
        {
            byte a = this->tabela_de_atributos_byte;
            int p1 = (this->tile_byte_menor & 0x80) >> 7;
            int p2 = (this->tile_byte_maior & 0x80) >> 6;
            this->tile_byte_menor <<= 1;
		    this->tile_byte_maior <<= 1;
            valor << 4;
            valor |= a | p1 | p2;
        }
        this->tile_dados |= static_cast<uint64>(valor);
    }

    void Ppu::avaliar_sprites()
    {
        int altura = 0;
        if (!this->flag_sprite_altura)
        {
            altura = 8;
        }
        else
        {
            altura = 16;
        }

        int contagem = 0;
        for (int i = 0; i < 64; i++)
        {
            uint pos_y = this->oam.at(i*4);
            uint atrib = this->oam.at(i*4+2);
            uint pos_x = this->oam.at(i*4+3);
            int linha = static_cast<int>(this->scanline) - static_cast<int>(pos_y);

            if (linha < 0 || linha >= altura)
            {
                continue;
            }

            if (contagem < 8)
            {
                this->sprites_padroes.at(contagem) = this->buscar_padrao_sprite(i, linha);
                this->sprites_posicoes.at(contagem) = x;
                this->sprites_prioridades.at(contagem) = (atrib >> 5) & 1;
                this->sprites_indices.at(contagem) = static_cast<byte>(i);
            }
            contagem += 1;
        }
        if (contagem > 8)
        {
            contagem = 8;
            this->flag_sprite_transbordamento = true;
        }
        this->sprites_qtd = contagem;
    }

    void Ppu::copiar_x()
    {
        // v: ....F.. ...EDCBA = t: ....F.. ...EDCBA
        this->v = (this->v & 0xFBE0) | (this->t & 0x041F);
    }

    void Ppu::copiar_y()
    {
        // v: IHGF.ED CBA..... = t: IHGF.ED CBA.....
        this->v = (this->v & 0x841F) | (this->t & 0x7BE0);
    }

    void Ppu::mudar_scroll_x()
    {
        if ((this->v & 0x001F) == 31)
        {
            // x = 0
            this->v &= 0xFFE0;
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
            this->v &= 0x8FFF;
            uint16 y = (this->v & 0x03E0) >> 5;
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

            this->v = (this->v & 0xFC1F) | (y << 5);
        }
    }

    void Ppu::set_controle(byte valor)
    {
        this->flag_nametable_base = valor & 0b00000011;
        this->flag_incrementar = buscar_bit(valor, 2);
        this->flag_padrao_sprite = buscar_bit(valor, 3);
        this->flag_padrao_fundo = buscar_bit(valor, 4);
        this->flag_sprite_altura = buscar_bit(valor, 5);
        this->flag_mestre_escravo = buscar_bit(valor, 6);
        this->nmi_output = buscar_bit(valor, 7) == 1;
        this->alterar_nmi();

        // t: ...BA.. ........ = d: ......BA
        this->t = (this->t & 0xF3FF) | ((static_cast<uint16>(valor) & 0x03) << 10);
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
        byte resultado = this->ultimo_valor & 0x1F;
        resultado |= static_cast<byte>(this->flag_sprite_transbordamento) << 5;
        resultado |= static_cast<byte>(this->flag_sprite_zero) << 6;
        
        if (this->nmi_ocorreu)
        {
            resultado |= 1 << 7;
        }
        this->nmi_ocorreu = false;
        this->alterar_nmi();
        
        // w: = 0
        this->w = 0;
        return resultado;
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
            this->t = (this->t & 0xFFE0) | (static_cast<uint16>(valor) >> 3);
            this->x = valor & 0x07;
            this->w = true;
        }
        else
        {
            // t: CBA..HG FED..... = d: HGFEDCBA
            // w:                  = 0
            this->t = (this->t & 0x8FFF) | ((static_cast<uint16>(valor) & 0x07) << 12);
		    this->t = (this->t & 0xFC1F) | ((static_cast<uint16>(valor) & 0xF8) << 2);
		    this->w = false;
        }
    }

    void Ppu::set_endereco(byte valor)
    {
        // se o valor de 'w' for 0, estamos na primeira escrita
        // caso não seja, estamos na segunda escrita
        if (this->w == 0)
        {
            // t: .FEDCBA ........ = d: ..FEDCBA
            // t: X...... ........ = 0
            // w:                  = 1
            this->t = (this->t & 0x80FF) | ((static_cast<uint16>(valor) & 0x3F) << 8);
		    this->w = 1;
        }
        else
        {
            // t: ....... HGFEDCBA = d: HGFEDCBA
            // v                   = t
            // w:                  = 0
            this->t = (this->t & 0xFF00) | static_cast<uint16>(valor);
		    this->v = this->t;
		    this->w = 0;
        }
    }

    void Ppu::set_omd_dma(Nes *nes, byte valor)
    {
        uint16 ponteiro = static_cast<uint16>(valor) << 8;

        for (uint32 i = 0; i < 256; i++)
        {
            this->oam.at(this->oam_endereco) = this->memoria->ler(ponteiro);
            this->oam_endereco += 1;
            ponteiro += 1;
        }

        nes->cpu.esperar_adicionar(513);
        if ((nes->cpu.get_ciclos() % 2) == 1)
        {
            nes->cpu.esperar_adicionar(1);
        }
    }

    byte Ppu::get_dados()
    {
        byte valor = this->ler(this->memoria->nes, this->v);
        if ((this->v%0x4000) < 0x3F00)
        {
            const byte buffer_dados = this->buffer_dados;
            this->buffer_dados = valor;
            valor = buffer_dados;
        }
        else
        {
            this->buffer_dados = this->ler(this->memoria->nes, this->v - 0x1000);
        }

        if (this->flag_incrementar == false)
            this->v += 1;
        else
            this->v += 32;

        return valor;
    }

    void Ppu::set_dados(Nes *nes, byte valor)
    {
        this->escrever(nes, this->v, valor);
        
        if (this->flag_incrementar == false)
            this->v += 1;
        else
            this->v += 32;
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

    void Ppu::set_textura_valor(array<byte, (256*240)>& textura, int x, int y, int valor)
    {
        const int largura = 256;
        textura.at(x + largura*y) = valor;
    }

    array<byte, (256*240)> Ppu::get_textura()
    {
        return this->frente;
    }
}
