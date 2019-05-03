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

namespace nesbrasa::nucleo
{
    using std::vector;
    using std::unique_ptr;

    enum class Espelhamento
    {
        HORIZONTAL,
        VERTICAL,
        TELA_UNICA,
        QUATRO_TELAS,
    };

    enum class MapeadorTipo
    {
        NROM,
        MMC1,
        DESCONHECIDO,
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
        
        uint8_t prg_quantidade;
        uint8_t chr_quantidade;

        bool rom_carregada;
        bool possui_sram;
        bool _possui_chr_ram;

        ArquivoFormato formato;
        MapeadorTipo   mapeador_tipo;
        Espelhamento   espelhamento;

    public:
        vector<uint8_t> prg;
        vector<uint8_t> chr;
        vector<uint8_t> chr_ram;
        vector<uint8_t> sram;

        Cartucho();

        void carregar_rom(vector<uint8_t> rom);

        uint8_t ler(uint16_t endereco);

        void escrever(uint16_t endereco, uint8_t valor);

        uint8_t get_prg_quantidade();

        uint8_t get_chr_quantidade();

        ArquivoFormato get_formato();

        MapeadorTipo get_mapeador_tipo();

        Espelhamento get_espelhamento();

        bool possui_rom_carregada();

        bool possui_chr_ram();
    
    private:
        void resetar_arrays();
    };
}
