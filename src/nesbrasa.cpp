/* nesbrasa.cpp
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

#include <string>
#include <sstream>

#include "nesbrasa.hpp"
#include "mapeadores/nrom.hpp"
#include "util.hpp"

namespace nesbrasa::nucleo
{
    using std::make_unique;
    using std::stringstream;
    using std::runtime_error;
    using namespace std::string_literals;
    using namespace mapeadores;

    Nes::Nes(): 
        memoria(this),
        cpu(&this->memoria),
        ppu(&this->memoria)
    {
        this->programa_carregado = false;
        this->cartucho = nullptr;
        this->cartucho_tipo = CartuchoTipo::DESCONHECIDO;
    }

    void Nes::carregar_rom(vector<byte> arquivo)
    {
        this->cpu.resetar();

        this->cartucho = nullptr;
        this->programa_carregado = false;
        auto formato = ArquivoFormato::DESCONHECIDO;

        // checa se o arquivo é grande o suficiente para ter um cabeçalho
        if (arquivo.size() < 16)
        {
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        // lê os 4 primeiros bytes do arquivo como uma string
        string formato_string;
        for (int i = 0; i < 4; i++)
        {
            formato_string += static_cast<char>(arquivo.at(i));
        }

        // arquivos nos formatos iNES e NES 2.0 começam com a string "NES\x1A"
        if (formato_string == "NES\x1A")
        {
            if (buscar_bit(arquivo.at(7), 2) == false && buscar_bit(arquivo.at(7), 3) == true)
            {
                // o arquivo está no formato NES 2.0
                formato = ArquivoFormato::NES_2_0;
            }
            else
            {
                // o arquivo está no formato iNES
                formato = ArquivoFormato::INES;
            }
        }
        else
        {
            // formato inválido
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        int prg_qtd = arquivo.at(4); // quantidade de bancos na rom prg
        int chr_qtd = arquivo.at(5); // quantidade de bancos na rom chr

        byte mapeador_nibble_menor = (arquivo.at(6) & 0xF0) >> 4;
        byte mapeador_nibble_maior = (arquivo.at(7) & 0xF0) >> 4;
        byte mapeador_codigo = (mapeador_nibble_maior << 4) | mapeador_nibble_menor;

        // converte o valor para uma enumeração do tipo MapeadorTipo
        this->cartucho_tipo = static_cast<CartuchoTipo>(mapeador_codigo);

        // construir o objeto do cartucho
        switch (this->cartucho_tipo)
        {
            case CartuchoTipo::NROM:
                this->cartucho = make_unique<NRom>(prg_qtd, chr_qtd, arquivo, formato);
                break;

            default:
            {
                this->cartucho = nullptr;
                this->cartucho_tipo = CartuchoTipo::DESCONHECIDO;

                // jogar mensagem de erro
                stringstream erro_ss;
                erro_ss << "Erro: mapeador não reconhecido\n";
                erro_ss << "Código do mapeador: " << static_cast<int>(mapeador_codigo);
                throw runtime_error(erro_ss.str());
            }
        }

        if (buscar_bit(arquivo.at(6), 3) == true)
        {
            this->ppu.espelhamento = Espelhamento::QUATRO_TELAS;
        }
        else
        {
            if (buscar_bit(arquivo.at(6), 0) == false)
                this->ppu.espelhamento = Espelhamento::VERTICAL;
            else
                this->ppu.espelhamento = Espelhamento::HORIZONTAL;
        }

        //TODO: Completar suporte a ROMs no formato NES 2.0
        this->programa_carregado = true;
    }

    void Nes::ciclo()
    {
        if (!this->programa_carregado)
        {
            throw runtime_error("Erro: nenhuma programa na memória"s);
        }

        this->cpu.ciclo();
    }
}