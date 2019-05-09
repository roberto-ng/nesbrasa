/* cartucho.hpp
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
#include <vector>
#include <memory>

#include "mapeador.hpp"
#include "nrom.hpp"
#include "tipos_numeros.hpp"

namespace nesbrasa::nucleo
{
    using std::vector;
    using std::unique_ptr;
    using mapeadores::Mapeador;
    using mapeadores::MapeadorTipo;
    using namespace tipos;

    enum class Espelhamento
    {
        HORIZONTAL,
        VERTICAL,
        TELA_UNICA,
        QUATRO_TELAS,
    };

    enum class ArquivoFormato
    {
        DESCONHECIDO,
        INES,
        NES_2_0,
    };

    class Cartucho
    {
    private:
        unique_ptr<Mapeador> mapeador;
        
        byte prg_quantidade;
        byte chr_quantidade;

        bool rom_carregada;
        bool possui_sram;
        bool _possui_chr_ram;

        ArquivoFormato formato;
        MapeadorTipo   mapeador_tipo;
        Espelhamento   espelhamento;

    public:
        // tamanho em bytes de um banco da ROM PRG
        static const int PRG_BANCOS_TAMANHO;
        // tamanho em bytes de um banco da ROM CHR
        static const int CHR_BANCOS_TAMANHO;

        vector<byte> prg;
        vector<byte> chr;
        vector<byte> chr_ram;
        vector<byte> sram;

        Cartucho();

        void carregar_rom(vector<byte> arquivo);

        byte ler(uint16 endereco);

        void escrever(uint16 endereco, byte valor);

        byte get_prg_quantidade();

        byte get_chr_quantidade();

        ArquivoFormato get_formato();

        MapeadorTipo get_mapeador_tipo();

        Espelhamento get_espelhamento();

        bool possui_rom_carregada();

        bool possui_chr_ram();
    
    private:
        void resetar_arrays();
    };
}
