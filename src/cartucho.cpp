/* cartucho.cpp
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

#include <sstream>
#include <string>
#include <stdexcept>

#include "cartucho.hpp"
#include "nrom.hpp"
#include "util.hpp"

namespace nesbrasa::nucleo
{
    using std::string;
    using std::stringstream;
    using std::make_unique;
    using std::runtime_error;
    using namespace std::string_literals;
    
    const int Cartucho::PRG_BANCOS_TAMANHO = 0x4000;
    const int Cartucho::CHR_BANCOS_TAMANHO = 0x2000;

    Cartucho::Cartucho()
    {
        this->espelhamento = Espelhamento::VERTICAL;
        this->mapeador_tipo = MapeadorTipo::DESCONHECIDO;
        this->formato = ArquivoFormato::DESCONHECIDO;
        this->prg_quantidade = 0;
        this->chr_quantidade = 0;
        this->rom_carregada = false;
        this->possui_sram = false;
        this->mapeador = nullptr;
        this->_possui_chr_ram = false;

        this->sram.resize(0x2000);
    }

    void Cartucho::carregar_rom(vector<byte> rom)
    {
        this->rom_carregada = false;
        this->formato = ArquivoFormato::DESCONHECIDO;

        // limpa os arrays e o mapeador
        this->mapeador.reset();
        this->resetar_arrays();

        this->sram.resize(0x2000);
        for (auto& valor : this->sram)
        {
            valor = 0;
        }

        // checa se o arquivo é grande o suficiente para ter um cabeçalho
        if (rom.size() < 16)
        {
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        // lê os 4 primeiros bytes do arquivo como uma string
        string formato_string;
        for (int i = 0; i < 4; i++)
        {
            formato_string += static_cast<char>(rom.at(i));
        }

        // arquivos nos formatos iNES e NES 2.0 começam com a string "NES\x1A"
        if (formato_string == "NES\x1A")
        {
            if (buscar_bit(rom.at(7), 2) == false && buscar_bit(rom.at(7), 3) == true)
            {
                // o arquivo está no formato NES 2.0
                this->formato = ArquivoFormato::NES_2_0;
            }
            else
            {
                // o arquivo está no formato iNES
                this->formato = ArquivoFormato::INES;
            }
        }
        else
        {
            // formato inválido
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        this->possui_sram = buscar_bit(rom.at(6), 1);

        this->prg_quantidade = rom.at(4);
        this->chr_quantidade = rom.at(5);

        if (this->chr_quantidade == 0)
        {
            this->_possui_chr_ram = true;
        }

        uint rom_prg_tamanho = this->prg_quantidade * Cartucho::PRG_BANCOS_TAMANHO;
        uint rom_chr_tamanho = this->chr_quantidade * Cartucho::CHR_BANCOS_TAMANHO;

        this->prg.resize(rom_prg_tamanho);
        this->chr.resize(rom_chr_tamanho);

        // checa o tamanho do arquivo
        if ((buscar_bit(rom.at(6), 2) + rom_prg_tamanho + rom_chr_tamanho) > rom.size())
        {
            // formato inválido
            throw runtime_error("Erro: formato não reconhecido"s);
        }

        // busca o inicio da ROM PRG
        int rom_prg_inicio;
        if (buscar_bit(rom.at(6), 2) == true)
            rom_prg_inicio = 16 + 512;
        else
            rom_prg_inicio = 16;

        // calcula o inicio da ROM CHR
        int rom_chr_inicio = rom_prg_inicio + rom_prg_tamanho;

        // Copia os dados referentes à ROM PRG do arquivo para o array
        for (uint i = 0; i < this->prg.size(); i++)
        {
            this->prg.at(i) = rom.at(rom_prg_inicio+i);
        }

        // Copia os dados referentes à ROM CHR do arquivo para o array
        for (uint i = 0; i < this->chr.size(); i++) 
        {
            this->chr.at(i) = rom.at(rom_chr_inicio+i);
        }

        byte mapeador_nibble_menor = (rom.at(6) & 0xF0) >> 4;
        byte mapeador_nibble_maior = (rom.at(7) & 0xF0) >> 4;
        byte mapeador_codigo = (mapeador_nibble_maior << 4) | mapeador_nibble_menor;

        // converte o valor para uma enumeração do tipo MapeadorTipo
        this->mapeador_tipo = static_cast<MapeadorTipo>(mapeador_codigo);

        // construir o objeto do mapeador do cartucho
        switch (this->mapeador_tipo)
        {
            case MapeadorTipo::NROM:
                this->mapeador = make_unique<mapeadores::NRom>(this);
                break;

            default:
            {
                this->mapeador = nullptr;
                this->mapeador_tipo = MapeadorTipo::DESCONHECIDO;

                // jogar mensagem de erro
                stringstream erro_ss;
                erro_ss << "Erro: mapeador não reconhecido\n";
                erro_ss << "Código do mapeador: " << static_cast<int>(mapeador_codigo);
                throw runtime_error(erro_ss.str());
            }
        }

        if (buscar_bit(rom.at(6), 3) == true)
        {
            this->espelhamento = Espelhamento::QUATRO_TELAS;
        }
        else
        {
            if (buscar_bit(rom.at(6), 0) == false)
                this->espelhamento = Espelhamento::VERTICAL;
            else
                this->espelhamento = Espelhamento::HORIZONTAL;
        }

        //TODO: Completar suporte a ROMs no formato NES 2.0
        this->rom_carregada = true;
    }

    byte Cartucho::ler(uint16_t endereco)
    {
        if (this->mapeador != nullptr)
        {
            return this->mapeador->ler(this, endereco);
        }
        
        throw runtime_error("Mapeador não existente"s);
    }

    void Cartucho::escrever(uint16_t endereco, byte valor)
    {
        if (this->mapeador != nullptr)
        {
            this->mapeador->escrever(this, endereco, valor);
        }
        else 
        {
            throw runtime_error("Mapeador não existente"s);
        }
    }

    byte Cartucho::get_prg_quantidade()
    {
        return this->prg_quantidade;
    }

    byte Cartucho::get_chr_quantidade()
    {
        return this->chr_quantidade;
    }

    ArquivoFormato Cartucho::get_formato()
    {
        return this->formato;
    }

    MapeadorTipo Cartucho::get_mapeador_tipo()
    {
        return this->mapeador_tipo;
    }

    Espelhamento Cartucho::get_espelhamento()
    {
        return this->espelhamento;
    }

    bool Cartucho::possui_rom_carregada()
    {
        return this->rom_carregada;
    }

    bool Cartucho::possui_chr_ram()
    {
        return this->_possui_chr_ram;
    }

    void Cartucho::resetar_arrays()
    {
        vector<byte>().swap(this->prg);
        vector<byte>().swap(this->chr);
        vector<byte>().swap(this->sram);
        vector<byte>().swap(this->chr_ram);
    }
}
