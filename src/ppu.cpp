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

namespace nesbrasa::nucleo
{
    Ppu::Ppu(Memoria* memoria): 
        memoria(memoria),
        oam({ 0 }),
        tabelas_de_nomes({ 0 }),
        paletas({ 0 })
    {
        this->espelhamento = Espelhamento::VERTICAL;
        this->buffer_dados = 0;
        this->ultimo_valor = 0;
        this->vram_incrementar = 0;
        this->oam_endereco = 0;
        this->nametable_endereco = 0;
        this->padrao_fundo_endereco = 0;
        this->padrao_sprite_endereco = 0;

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
    }

    byte Ppu::registrador_ler(Nes *nes, uint16 endereco)
    {
        switch (endereco)
        {
            case 0x2002:
                return this->get_estado(nes);

            case 0x2004:
                return this->get_oam_dados(nes);

            case 0x2007:
                return this->get_dados(nes);

            default:
                return 0;
        }
    }

    void Ppu::registrador_escrever(Nes *nes, uint16 endereco, byte valor)
    {
        switch (endereco)
        {
            case 0x2000:
                this->set_controle(nes, valor);
                break;

            case 0x2001:
                this->set_mascara(nes, valor);
                break;

            case 0x2003:
                this->set_oam_enderco(nes, valor);
                break;

            case 0x2005:
                this->set_scroll(nes, valor);
                break;

            case 0x2006:
                this->set_endereco(nes, valor);
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
            uint16 endereco_espelhado = this->endereco_espelhado(nes, endereco);
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
            uint16 endereco_espelhado = this->endereco_espelhado(nes, endereco);
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

    void Ppu::set_controle(Nes *nes, byte valor)
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

    void Ppu::set_mascara(Nes *nes, byte  valor)
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

    byte Ppu::get_estado(Nes *nes)
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

    void Ppu::set_oam_enderco(Nes *nes, byte valor)
    {
        this->oam_endereco = valor;
    }

    void Ppu::set_oam_dados(Nes *nes, byte valor)
    {
        this->oam.at(this->oam_endereco) = valor;
        this->oam_endereco += 1;
    }

    byte Ppu::get_oam_dados(Nes *nes)
    {
        return this->oam.at(this->oam_endereco);
    }

    void Ppu::set_scroll(Nes *nes, byte valor)
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

    void Ppu::set_endereco(Nes *nes, byte valor)
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

    byte Ppu::get_dados(Nes *nes)
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

    uint16 Ppu::endereco_espelhado(Nes *nes, uint16 endereco)
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