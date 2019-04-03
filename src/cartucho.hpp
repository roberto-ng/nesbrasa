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

using std::vector;
using std::unique_ptr;

namespace nesbrasa::nucleo
{
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

    class Cartucho
    {
    private:
        void resetar_arrays();

    public:
        vector<uint8_t> prg;
        vector<uint8_t> chr;
        vector<uint8_t> sram;

        uint8_t prg_quantidade;
        uint8_t chr_quantidade;

        unique_ptr<IMapeador> mapeador;

        MapeadorTipo mapeador_tipo;
        Espelhamento espelhamento;

        bool rom_carregada;
        bool possui_sram;

        Cartucho();

        int carregar_rom(vector<uint8_t> rom);

        uint8_t mapeador_ler(uint16_t endereco);

        void mapeador_escrever(uint16_t endereco, uint8_t valor);
    };
}
