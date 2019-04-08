/* instrucao.cpp
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

#include <memory>

#include "nesbrasa.hpp"
#include "cpu.hpp"
#include "instrucao.hpp"
#include "memoria.hpp"
#include "util.hpp"

namespace nesbrasa::nucleo
{

    Instrucao::Instrucao(
        string nome,
        uint8_t bytes,
        int32_t ciclos,
        int32_t ciclos_pag_alterada,
        InstrucaoModo  modo,
        function< void(Instrucao*,Cpu*,uint16_t) > funcao
    )
    {
        this->nome = nome;
        this->bytes = bytes;
        this->ciclos = ciclos;
        this->ciclos_pag_alterada = ciclos_pag_alterada;
        this->modo = modo;
        this->funcao = funcao;
    }

    uint16_t Instrucao::buscar_endereco(Cpu* cpu)
    {
        cpu->pag_alterada = false;

        switch (this->modo)
        {
            case InstrucaoModo::ACM:
                return 0;

            case InstrucaoModo::IMPL:
                return 0;

            case InstrucaoModo::IMED:
                return cpu->pc + 1;

            case InstrucaoModo::P_ZERO:
                return cpu->memoria->ler(cpu->pc + 1);

            case InstrucaoModo::P_ZERO_X:
                return cpu->memoria->ler(cpu->pc + 1 + cpu->x) & 0xFF;

            case InstrucaoModo::P_ZERO_Y:
                return cpu->memoria->ler(cpu->pc + 1 + cpu->y) & 0xFF;

            case InstrucaoModo::ABS:
                return cpu->memoria->ler_16_bits(cpu->pc + 1);

            case InstrucaoModo::ABS_X:
            {
                uint16_t endereco = cpu->memoria->ler_16_bits(cpu->pc + 1 + cpu->x);
                cpu->pag_alterada = !comparar_paginas(endereco - cpu->x, endereco);

                return endereco;
            }

            case InstrucaoModo::ABS_Y:
            {
                uint16_t endereco =  cpu->memoria->ler_16_bits(cpu->pc + 1 + cpu->y);
                cpu->pag_alterada = !comparar_paginas(endereco - cpu->y, endereco);

                return endereco;
            }

            case InstrucaoModo::IND:
            {
                const uint16_t valor = cpu->memoria->ler_16_bits(cpu->pc+1);
                return cpu->memoria->ler_16_bits_bug(valor);
            }

            case InstrucaoModo::IND_X:
            {
                const uint16_t valor = cpu->memoria->ler(cpu->pc + 1);
                return cpu->memoria->ler_16_bits_bug((valor + cpu->x)%0x100);
            }

            case InstrucaoModo::IND_Y:
            {
                const uint16_t valor = cpu->memoria->ler(cpu->pc + 1);
                uint16_t endereco = cpu->memoria->ler_16_bits_bug(valor) + cpu->y;
                cpu->pag_alterada = !comparar_paginas(endereco - cpu->y, endereco);

                return endereco;
            }

            case InstrucaoModo::REL:
            {
                const uint16_t valor = cpu->memoria->ler(cpu->pc + 1);

                if (valor < 0x80)
                    return cpu->pc + 2 + valor;
                else
                    return cpu->pc + 2 + valor - 0x100;
            }
        }

        return 0;
    }

    /*!
    Instrução ADC
    A + M + C -> A, C
    */
    static void instrucao_adc(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        const uint8_t a = cpu->a;
        const uint8_t c = (cpu->c) ? 1 : 0;

        cpu->a = a + valor + c;

        // atualiza a flag c
        int32_t soma_total = (int32_t)a + (int32_t)valor + (int32_t)c;
        if (soma_total > 0xFF)
            cpu->c = 1;
        else
            cpu->c = 0;

        // checa se houve um overflow/transbordamento e atualiza a flag v
        // solução baseada em: https://stackoverflow.com/a/16861251
        if ((~(a ^ valor) & (a ^ soma_total) & 0x80) != 0)
            cpu->v = 1;
        else
            cpu->v = 0;

        // atualiza as flags z e n
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*!
    Instrução AND
    A AND M -> A
    */
    static void instrucao_and(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        const uint8_t a = cpu->a;
        const uint8_t m = valor;

        cpu->a = a & m;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    /*!
    Instrução shift para a esquerda.
    Utiliza a memoria ou o acumulador
    */
    static void instrucao_asl(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
        {
            // checa se a posição 7 do byte é '1' ou '0'
            cpu->c = buscar_bit(cpu->a, 7);

            cpu->a <<= 1;

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            uint8_t valor = cpu->memoria->ler(endereco);

            // checa se a posição 7 do byte é '1' ou '0'
            cpu->c = buscar_bit(valor, 7);

            valor <<= 1;

            cpu->memoria->escrever(endereco, valor);

            // atualizar flags
            cpu->set_n(valor);
            cpu->set_z(valor);
        }
    }

    //! Pula para o endereço indicado se a flag 'c' não estiver ativa
    static void instrucao_bcc(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->c == false)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Pula para o endereço indicado se a flag 'c' estiver ativa
    static void instrucao_bcs(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->c == true)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Pula para o endereço indicado se a flag 'z' estiver ativa
    static void instrucao_beq(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->z == true)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    /*! BIT
    Busca um byte na memoria e depois salva a posição 7 do byte em 'n'
    e a posição 6 do byte em 'v'.
    A flag 'z' tambem é alterada sendo calculada com 'a' AND valor
    */
    static void instrucao_bit(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        cpu->n = buscar_bit(valor, 7);
        cpu->v = buscar_bit(valor, 6);
        cpu->z = (valor & cpu->a) == 0;
    }

    //! Pula para o endereço indicado se a flag 'n' estiver ativa
    static void instrucao_bmi(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->n == true)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Pula para o endereço indicado se a flag 'z' não estiver ativa
    static void instrucao_bne(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->z == false)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Pula para o endereço indicado se a flag 'n' não estiver ativa
    static void instrucao_bpl(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->n == false)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Instrução BRK
    static void instrucao_brk(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->stack_empurrar_16_bits(cpu->pc);
        cpu->stack_empurrar(cpu->get_estado());

        cpu->b = 1;
        cpu->pc = cpu->memoria->ler_16_bits(0xFFFE);
    }

    //! Pula para o endereço indicado se a flag 'v' não estiver ativa
    static void instrucao_bvc (Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->v == false)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Pula para o endereço indicado se a flag 'v' estiver ativa
    static void instrucao_bvs(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (cpu->v == true)
        {
            cpu->branch_somar_ciclos(endereco);
            cpu->pc = endereco;
        }
    }

    //! Limpa a flag 'c'
    static void instrucao_clc(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->c = false;
    }

    //! Limpa a flag 'd'
    static void instrucao_cld(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->d = false;
    }

    //! Limpa a flag 'i'
    static void instrucao_cli(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->i = false;
    }

    //! Limpa a flag 'v'
    static void instrucao_clv(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->v = false;
    }

    //! Compara o acumulador com um valor
    static void instrucao_cmp(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        if (cpu->a >= valor)
            cpu->c = true;
        else
            cpu->c = false;

        uint8_t resultado = cpu->a - valor;

        // atualizar flags
        cpu->set_n(resultado);
        cpu->set_z(resultado);
    }

    //! Compara o indice X com um valor
    static void instrucao_cpx(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        if (cpu->x >= valor)
            cpu->c = true;
        else
            cpu->c = false;

        uint8_t resultado = cpu->x - valor;

        // atualizar flags
        cpu->set_n(resultado);
        cpu->set_z(resultado);
    }

    //! Compara o indice Y com um valor
    static void instrucao_cpy(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        if (cpu->y >= valor)
            cpu->c = true;
        else
            cpu->c = false;

        uint8_t resultado = cpu->y - valor;
        cpu->set_n(resultado);
        cpu->set_z (resultado);
    }

    //! Diminui um valor na memoria por 1
    static void instrucao_dec(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        valor -= 1;

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco, valor);

        // atualizar flags
        cpu->set_n(valor);
        cpu->set_z(valor);
    }

    //! Diminui o valor do indice X por 1
    static void instrucao_dex(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->x -= 1;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Diminui o valor do indice Y por 1
    static void instrucao_dey(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->y -= 1;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! OR exclusivo de um valor na memoria com o acumulador
    static void instrucao_eor(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        cpu->a = cpu->a ^ valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Incrementa um valor na memoria por 1
    static void instrucao_inc(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        valor += 1;

        // atualizar o valor na memoria
        cpu->memoria->escrever(endereco, valor);

        // atualizar flags
        cpu->set_n(valor);
        cpu->set_z(valor);
    }

    //! Incrementa o valor do indice X por 1
    static void instrucao_inx(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->x += 1;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Incrementa o valor do indice Y por 1
    static void instrucao_iny(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->y += 1;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! Pula o programa para o endereço indicado
    static void instrucao_jmp(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        // muda o endereço
        cpu->pc = endereco;
    }

    //! Chama uma função/subrotina
    static void instrucao_jsr(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        // Salva o endereço da próxima instrução subtraído por 1 na stack.
        // O endereço guardado vai ser usado para retornar da função quando
        // o opcode 'rts' for usado
        cpu->stack_empurrar_16_bits(cpu->pc - 1);

        // muda o endereço atual do programa para o da função indicada
        cpu->pc = endereco;
    }

    //! Carrega um valor da memoria no acumulador
    static void instrucao_lda(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->a = cpu->memoria->ler(endereco);

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }


    //! Carrega um valor da memoria no indice X
    static void instrucao_ldx(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->x = cpu->memoria->ler(endereco);

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Carrega um valor da memoria no acumulador
    static void instrucao_ldy(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->y = cpu->memoria->ler(endereco);

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    /*!
    Instrução shift para a direita.
    Utiliza a memoria ou o acumulador
    */
    static void instrucao_lsr(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
        {
            // checa se a posição 0 do byte é '1' ou '0'
            cpu->c = buscar_bit(cpu->a, 0);

            cpu->a >>= 1;

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            uint8_t valor = cpu->memoria->ler(endereco);

            // checa se a posição 0 do byte é '1' ou '0'
            cpu->c = buscar_bit(valor, 0);

            valor >>= 1;

            cpu->memoria->escrever(endereco, valor);

            // atualizar flags
            cpu->set_n(valor);
            cpu->set_z(valor);
        }
    }

    //! Não fazer nada
    static void instrucao_nop(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
    }

    //! Operanção OR entre um valor na memoria e o acumulador
    static void instrucao_ora(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        cpu->a = cpu->a | valor;

        //atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Empurra o valor do acumulador na stack
    static void instrucao_pha(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->stack_empurrar(cpu->a);
    }

    //! Empurra o valor do estado do processador na stack
    static void instrucao_php(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        const uint8_t estado = cpu->get_estado();
        cpu->stack_empurrar(estado);
    }

    //! Puxa um valor da stack e salva esse valor no acumulador
    static void instrucao_pla(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->a = cpu->stack_puxar();

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Puxa um valor da stack e salva esse valor no estado do processador
    static void instrucao_plp(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        const uint8_t estado = cpu->stack_puxar();
        cpu->set_estado(estado);
    }

    //! Gira um valor pra a esquerda
    static void instrucao_rol(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
        {
            bool carregar = cpu->c;
            cpu->c = buscar_bit(cpu->a, 7);
            cpu->a <<= 1;
            cpu->a = cpu->a | ((carregar) ? 1 : 0);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            uint8_t valor = cpu->memoria->ler(endereco);

            bool carregar = cpu->c;
            cpu->c = buscar_bit(valor, 7);
            valor <<= 1;
            valor = valor | ((carregar) ? 1 : 0);

            // atualizar o valor na memoria
            cpu->memoria->escrever(endereco, valor);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
    }

    //! Gira um valor pra a direita
    static void instrucao_ror(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        if (instrucao->modo == InstrucaoModo::ACM)
        {
            bool carregar = cpu->c;
            cpu->c = buscar_bit(cpu->a, 0);
            cpu->a >>= 1;
            cpu->a = cpu->a | ((carregar) ? 0b10000000 : 0);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
        else
        {
            uint8_t valor = cpu->memoria->ler(endereco);

            bool carregar = cpu->c;
            cpu->c = buscar_bit(cpu->a, 0);
            valor >>= 1;
            valor = valor | ((carregar) ? 0b10000000 : 0);

            // atualizar o valor na memoria
            cpu->memoria->escrever(endereco, valor);

            // atualizar flags
            cpu->set_n(cpu->a);
            cpu->set_z(cpu->a);
        }
    }

    //! Retorna de uma interupção
    static void instrucao_rti(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        const uint8_t estado = cpu->stack_puxar();
        cpu->set_estado(estado);

        cpu->pc = cpu->stack_puxar_16_bits();
    }

    //! Retorna de uma função/sub-rotina
    static void instrucao_rts(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->pc = cpu->stack_puxar_16_bits() + 1;
    }

    //! Subtrai um valor da memoria usando o acumulador
    static void instrucao_sbc(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        uint8_t valor = cpu->memoria->ler(endereco);

        const uint8_t a = cpu->a;
        const uint8_t c = (!cpu->c) ? 1 : 0;

        cpu->a = a - valor - c;

        // atualiza a flag c
        int32_t subtracao_total = (int32_t)a - (int32_t)valor - (int32_t)c;
        if (subtracao_total >= 0)
            cpu->c = 1;
        else
            cpu->c = 0;

        // checa se houve um overflow/transbordamento e atualiza a flag v
        if (((a ^ valor) & (a ^ subtracao_total) & 0x80) != 0)
            cpu->v = 1;
        else
            cpu->v = 0;

        // atualiza as flags z e n
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Ativa a flag 'c'
    static void instrucao_sec(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->c = true;
    }

    //! Ativa a flag 'd'
    static void instrucao_sed(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->d = true;
    }

    //! Ativa a flag 'i'
    static void instrucao_sei(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->i = true;
    }

    //! Guarda o valor do acumulador na memoria
    static void instrucao_sta(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->memoria->escrever(endereco, cpu->a);
    }

    //! Guarda o valor do registrador 'x' na memoria
    static void instrucao_stx(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->memoria->escrever(endereco, cpu->x);
    }

    //! Guarda o valor do registrador 'y' na memoria
    static void instrucao_sty(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->memoria->escrever(endereco, cpu->y);
    }

    //! Atribui o valor do acumulador ao registrador 'x'
    static void instrucao_tax(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->x = cpu->a;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Atribui o valor do acumulador ao registrador 'y'
    static void instrucao_tay(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->y = cpu->a;

        // atualizar flags
        cpu->set_n(cpu->y);
        cpu->set_z(cpu->y);
    }

    //! Atribui o valor do ponteiro da stack ao registrador 'x'
    static void instrucao_tsx(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->x = cpu->sp;

        // atualizar flags
        cpu->set_n(cpu->x);
        cpu->set_z(cpu->x);
    }

    //! Atribui o valor do registrador 'x' ao acumulador
    static void instrucao_txa(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->a = cpu->x;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    //! Atribui o valor do registrador 'x' ao ponteiro da stack
    static void instrucao_txs(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->sp = cpu->x;
    }

    //! Atribui o valor do registrador 'y' ao acumulador
    static void instrucao_tya(Instrucao* instrucao, Cpu* cpu, uint16_t endereco)
    {
        cpu->a = cpu->y;

        // atualizar flags
        cpu->set_n(cpu->a);
        cpu->set_z(cpu->a);
    }

    array< optional<Instrucao>, 256 > carregar_instrucoes()
    {
        // cria um array com 0x100 (256 em decimal) ponteiros para instruções
        array< optional<Instrucao>, 256 > instrucoes;
        for (auto& instrucao : instrucoes)
        {
            instrucao = std::nullopt;
        }

        // modos da instrução ADC
        instrucoes[0x69] = Instrucao("ADC", 2, 2, 0, InstrucaoModo::IMED, instrucao_adc);
        instrucoes[0x65] = Instrucao("ADC", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_adc);
        instrucoes[0x75] = Instrucao("ADC", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_adc);
        instrucoes[0x6D] = Instrucao("ADC", 3, 4, 0, InstrucaoModo::ABS, instrucao_adc);
        instrucoes[0x7D] = Instrucao("ADC", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_adc);
        instrucoes[0x79] = Instrucao("ADC", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_adc);
        instrucoes[0x61] = Instrucao("ADC", 2, 6, 0, InstrucaoModo::IND_X, instrucao_adc);
        instrucoes[0x71] = Instrucao("ADC", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_adc);

        // modos da instrução AND
        instrucoes[0x29] = Instrucao("AND", 2, 2, 0, InstrucaoModo::IMED, instrucao_and);
        instrucoes[0x25] = Instrucao("AND", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_and);
        instrucoes[0x35] = Instrucao("AND", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_and);
        instrucoes[0x2D] = Instrucao("AND", 3, 4, 0, InstrucaoModo::ABS, instrucao_and);
        instrucoes[0x3D] = Instrucao("AND", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_and);
        instrucoes[0x39] = Instrucao("AND", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_and);
        instrucoes[0x21] = Instrucao("AND", 2, 6, 0, InstrucaoModo::IND_X, instrucao_and);
        instrucoes[0x31] = Instrucao("AND", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_and);

        // modos da instrução ASL
        instrucoes[0x0A] = Instrucao("ASL", 1, 2, 0, InstrucaoModo::ACM, instrucao_asl);
        instrucoes[0x06] = Instrucao("ASL", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_asl);
        instrucoes[0x16] = Instrucao("ASL", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_asl);
        instrucoes[0x0E] = Instrucao("ASL", 3, 6, 0, InstrucaoModo::ABS, instrucao_asl);
        instrucoes[0x1E] = Instrucao("ASL", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_asl);

        // modos da instrução BCC
        instrucoes[0x90] = Instrucao("BCC", 2, 2, 0, InstrucaoModo::REL, instrucao_bcc);

        // modos da instrução BCS
        instrucoes[0xB0] = Instrucao("BCS", 2, 2, 0, InstrucaoModo::REL, instrucao_bcs);

        // modos da instrução BEQ
        instrucoes[0xF0] = Instrucao("BEQ", 2, 2, 0, InstrucaoModo::REL, instrucao_beq);

        // modos da instrução BIT
        instrucoes[0x24] = Instrucao("BIT", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_bit);
        instrucoes[0x2C] = Instrucao("BIT", 3, 4, 0, InstrucaoModo::ABS, instrucao_bit);

        // modos da instrução BMI
        instrucoes[0x30] = Instrucao("BMI", 2, 2, 0, InstrucaoModo::REL, instrucao_bmi);

        // modos da instrução BNE
        instrucoes[0xD0] = Instrucao("BNE", 2, 2, 0, InstrucaoModo::REL, instrucao_bne);

        // modos da instrução BPL
        instrucoes[0x10] = Instrucao("BPL", 2, 2, 0, InstrucaoModo::REL, instrucao_bpl);

        // modos da instrução BRK
        instrucoes[0x00] = Instrucao("BRK", 1, 7, 0, InstrucaoModo::IMPL, instrucao_brk);

        // modos da instrução BVC
        instrucoes[0x50] = Instrucao("BVC", 2, 2, 0, InstrucaoModo::REL, instrucao_bvc);

        // modos da instrução BVS
        instrucoes[0x70] = Instrucao("BVS", 2, 2, 0, InstrucaoModo::REL, instrucao_bvs);

        // modos da instrução CLC
        instrucoes[0x18] = Instrucao("CLC", 1, 2, 0, InstrucaoModo::IMPL, instrucao_clc);

        // modos da instrução CLD
        instrucoes[0xD8] = Instrucao("CLD", 1, 2, 0, InstrucaoModo::IMPL, instrucao_cld);

        // modos da instrução CLI
        instrucoes[0x58] = Instrucao("CLI", 1, 2, 0, InstrucaoModo::IMPL, instrucao_cli);

        // modos da instrução CLV
        instrucoes[0xB8] = Instrucao("CLV", 1, 2, 0, InstrucaoModo::IMPL, instrucao_clv);

        // modos da instrução CMP
        instrucoes[0xC9] = Instrucao("CMP", 2, 2, 0, InstrucaoModo::IMED, instrucao_cmp);
        instrucoes[0xC5] = Instrucao("CMP", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_cmp);
        instrucoes[0xD5] = Instrucao("CMP", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_cmp);
        instrucoes[0xCD] = Instrucao("CMP", 3, 4, 0, InstrucaoModo::ABS, instrucao_cmp);
        instrucoes[0xDD] = Instrucao("CMP", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_cmp);
        instrucoes[0xD9] = Instrucao("CMP", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_cmp);
        instrucoes[0xC1] = Instrucao("CMP", 2, 6, 0, InstrucaoModo::IND_X, instrucao_cmp);
        instrucoes[0xD1] = Instrucao("CMP", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_cmp);

        // modos da instrução CPX
        instrucoes[0xE0] = Instrucao("CPX", 2, 2, 0, InstrucaoModo::IMED, instrucao_cpx);
        instrucoes[0xE4] = Instrucao("CPX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_cpx);
        instrucoes[0xEC] = Instrucao("CPX", 3, 4, 0, InstrucaoModo::ABS, instrucao_cpx);

        // modos da instrução CPY
        instrucoes[0xC0] = Instrucao("CPY", 2, 2, 0, InstrucaoModo::IMED, instrucao_cpy);
        instrucoes[0xC4] = Instrucao("CPY", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_cpy);
        instrucoes[0xCC] = Instrucao("CPY", 3, 4, 0, InstrucaoModo::ABS, instrucao_cpy);

        // modos da instrução DEC
        instrucoes[0xC6] = Instrucao("DEC", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_dec);
        instrucoes[0xD6] = Instrucao("DEC", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_dec);
        instrucoes[0xCE] = Instrucao("DEC", 3, 6, 0, InstrucaoModo::ABS, instrucao_dec);
        instrucoes[0xDE] = Instrucao("DEC", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_dec);

        // modos da instrução DEX
        instrucoes[0xCA] = Instrucao("DEX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_dex);

        // modos da instrução DEY
        instrucoes[0x88] = Instrucao("DEX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_dey);

        // modos da instrução EOR
        instrucoes[0x49] = Instrucao("EOR", 2, 2, 0, InstrucaoModo::IMED, instrucao_eor);
        instrucoes[0x45] = Instrucao("EOR", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_eor);
        instrucoes[0x55] = Instrucao("EOR", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_eor);
        instrucoes[0x4D] = Instrucao("EOR", 3, 4, 0, InstrucaoModo::ABS, instrucao_eor);
        instrucoes[0x5D] = Instrucao("EOR", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_eor);
        instrucoes[0x59] = Instrucao("EOR", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_eor);
        instrucoes[0x41] = Instrucao("EOR", 2, 6, 0, InstrucaoModo::IND_X, instrucao_eor);
        instrucoes[0x51] = Instrucao("EOR", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_eor);

        // modos da instrução INC
        instrucoes[0xE6] = Instrucao("INC", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_inc);
        instrucoes[0xF6] = Instrucao("INC", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_inc);
        instrucoes[0xEE] = Instrucao("INC", 3, 6, 0, InstrucaoModo::ABS, instrucao_inc);
        instrucoes[0xFE] = Instrucao("INC", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_inc);

        // modos da instrução INX
        instrucoes[0xE8] = Instrucao("INX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_inx);

        // modos da instrução INY
        instrucoes[0xC8] = Instrucao("INY", 1, 2, 0, InstrucaoModo::IMPL, instrucao_iny);

        // modos da instrução JMP
        instrucoes[0x4C] = Instrucao("JMP", 3, 3, 0, InstrucaoModo::ABS, instrucao_jmp);
        instrucoes[0x6C] = Instrucao("JMP", 3, 5, 0, InstrucaoModo::IND, instrucao_jmp);

        // modos da instrução JSR
        instrucoes[0x20] = Instrucao("JSR", 3, 6, 0, InstrucaoModo::ABS, instrucao_jsr);

        // modos da instrução LDA
        instrucoes[0xA9] = Instrucao("LDA", 2, 2, 0, InstrucaoModo::IMED, instrucao_lda);
        instrucoes[0xA5] = Instrucao("LDA", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_lda);
        instrucoes[0xB5] = Instrucao("LDA", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_lda);
        instrucoes[0xAD] = Instrucao("LDA", 3, 4, 0, InstrucaoModo::ABS, instrucao_lda);
        instrucoes[0xBD] = Instrucao("LDA", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_lda);
        instrucoes[0xB9] = Instrucao("LDA", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_lda);
        instrucoes[0xA1] = Instrucao("LDA", 2, 6, 0, InstrucaoModo::IND_X, instrucao_lda);
        instrucoes[0xB1] = Instrucao("LDA", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_lda);

        // modos da instrução LDX
        instrucoes[0xA2] = Instrucao("LDX", 2, 2, 0, InstrucaoModo::IMED, instrucao_ldx);
        instrucoes[0xA6] = Instrucao("LDX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_ldx);
        instrucoes[0xB6] = Instrucao("LDX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, instrucao_ldx);
        instrucoes[0xAE] = Instrucao("LDX", 3, 4, 0, InstrucaoModo::ABS, instrucao_ldx);
        instrucoes[0xBE] = Instrucao("LDX", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_ldx);

        // modos da instrução LDY
        instrucoes[0xA0] = Instrucao("LDY", 2, 2, 0, InstrucaoModo::IMED, instrucao_ldy);
        instrucoes[0xA4] = Instrucao("LDY", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_ldy);
        instrucoes[0xB4] = Instrucao("LDY", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_ldy);
        instrucoes[0xAC] = Instrucao("LDY", 3, 4, 0, InstrucaoModo::ABS, instrucao_ldy);
        instrucoes[0xBC] = Instrucao("LDY", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_ldy);

        // modos da instrução LSR
        instrucoes[0x4A] = Instrucao("LSR", 1, 2, 0, InstrucaoModo::ACM, instrucao_lsr);
        instrucoes[0x46] = Instrucao("LSR", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_lsr);
        instrucoes[0x56] = Instrucao("LSR", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_lsr);
        instrucoes[0x4E] = Instrucao("LSR", 3, 6, 0, InstrucaoModo::ABS, instrucao_lsr);
        instrucoes[0x5E] = Instrucao("LSR", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_lsr);

        // modos da instrução NOP
        instrucoes[0xEA] = Instrucao("NOP", 1, 2, 0, InstrucaoModo::IMPL, instrucao_nop);

        // modos da instrução ORA
        instrucoes[0x09] = Instrucao("ORA", 2, 2, 0, InstrucaoModo::IMED, instrucao_ora);
        instrucoes[0x05] = Instrucao("ORA", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_ora);
        instrucoes[0x15] = Instrucao("ORA", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_ora);
        instrucoes[0x0D] = Instrucao("ORA", 3, 4, 0, InstrucaoModo::ABS, instrucao_ora);
        instrucoes[0x1D] = Instrucao("ORA", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_ora);
        instrucoes[0x19] = Instrucao("ORA", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_ora);
        instrucoes[0x01] = Instrucao("ORA", 2, 6, 0, InstrucaoModo::IND_X, instrucao_ora);
        instrucoes[0x11] = Instrucao("ORA", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_ora);

        // modos da instrução PHA
        instrucoes[0x48] = Instrucao("PHA", 1, 3, 0, InstrucaoModo::IMPL, instrucao_pha);

        // modos da instrução PHP
        instrucoes[0x08] = Instrucao("PHP", 1, 3, 0, InstrucaoModo::IMPL, instrucao_php);

        // modos da instrução PLA
        instrucoes[0x68] = Instrucao("PLA", 1, 4, 0, InstrucaoModo::IMPL, instrucao_pla);

        // modos da instrução PLP
        instrucoes[0x28] = Instrucao("PLP", 1, 4, 0, InstrucaoModo::IMPL, instrucao_plp);

        // modos da instrução ROL
        instrucoes[0x2A] = Instrucao("ROL", 1, 2, 0, InstrucaoModo::ACM, instrucao_rol);
        instrucoes[0x26] = Instrucao("ROL", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_rol);
        instrucoes[0x36] = Instrucao("ROL", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_rol);
        instrucoes[0x2E] = Instrucao("ROL", 3, 6, 0, InstrucaoModo::ABS, instrucao_rol);
        instrucoes[0x3E] = Instrucao("ROL", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_rol);

        // modos da instrução ROR
        instrucoes[0x6A] = Instrucao("ROR", 1, 2, 0, InstrucaoModo::ACM, instrucao_ror);
        instrucoes[0x66] = Instrucao("ROR", 2, 5, 0, InstrucaoModo::P_ZERO, instrucao_ror);
        instrucoes[0x76] = Instrucao("ROR", 2, 6, 0, InstrucaoModo::P_ZERO_X, instrucao_ror);
        instrucoes[0x6E] = Instrucao("ROR", 3, 6, 0, InstrucaoModo::ABS, instrucao_ror);
        instrucoes[0x7E] = Instrucao("ROR", 3, 7, 0, InstrucaoModo::ABS_X, instrucao_ror);

        // modos da instrução RTI
        instrucoes[0x40] = Instrucao("RTI", 1, 6, 0, InstrucaoModo::IMPL, instrucao_rti);

        // modos da instrução RTS
        instrucoes[0x60] = Instrucao("RTS", 1, 6, 0, InstrucaoModo::IMPL, instrucao_rts);

        // modos da instrução SBC
        instrucoes[0xE9] = Instrucao("SBC", 2, 2, 0, InstrucaoModo::IMED, instrucao_sbc);
        instrucoes[0xE5] = Instrucao("SBC", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sbc);
        instrucoes[0xF5] = Instrucao("SBC", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_sbc);
        instrucoes[0xED] = Instrucao("SBC", 3, 4, 0, InstrucaoModo::ABS, instrucao_sbc);
        instrucoes[0xFD] = Instrucao("SBC", 3, 4, 1, InstrucaoModo::ABS_X, instrucao_sbc);
        instrucoes[0xF9] = Instrucao("SBC", 3, 4, 1, InstrucaoModo::ABS_Y, instrucao_sbc);
        instrucoes[0xE1] = Instrucao("SBC", 2, 6, 0, InstrucaoModo::IND_X, instrucao_sbc);
        instrucoes[0xF1] = Instrucao("SBC", 2, 5, 1, InstrucaoModo::IND_Y, instrucao_sbc);

        // modos da instrução SEC
        instrucoes[0x38] = Instrucao("SEC", 1, 2, 0, InstrucaoModo::IMPL, instrucao_sec);

        // modos da instrução SED
        instrucoes[0xF8] = Instrucao("SED", 1, 2, 0, InstrucaoModo::IMPL, instrucao_sed);

        // modos da instrução SEI
        instrucoes[0x78] = Instrucao("SEI", 1, 2, 0, InstrucaoModo::IMPL, instrucao_sei);

        // modos da instrução STA
        instrucoes[0x85] = Instrucao("STA", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sta);
        instrucoes[0x95] = Instrucao("STA", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_sta);
        instrucoes[0x8D] = Instrucao("STA", 3, 4, 0, InstrucaoModo::ABS, instrucao_sta);
        instrucoes[0x9D] = Instrucao("STA", 3, 5, 0, InstrucaoModo::ABS_X, instrucao_sta);
        instrucoes[0x99] = Instrucao("STA", 3, 5, 0, InstrucaoModo::ABS_Y, instrucao_sta);
        instrucoes[0x81] = Instrucao("STA", 2, 6, 0, InstrucaoModo::IND_X, instrucao_sta);
        instrucoes[0x91] = Instrucao("STA", 2, 6, 0, InstrucaoModo::IND_Y, instrucao_sta);

        // modos da instrução STX
        instrucoes[0x86] = Instrucao("STX", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_stx);
        instrucoes[0x96] = Instrucao("STX", 2, 4, 0, InstrucaoModo::P_ZERO_Y, instrucao_stx);
        instrucoes[0x8E] = Instrucao("STX", 3, 4, 0, InstrucaoModo::ABS, instrucao_stx);

        // modos da instrução STY
        instrucoes[0x84] = Instrucao("STY", 2, 3, 0, InstrucaoModo::P_ZERO, instrucao_sty);
        instrucoes[0x94] = Instrucao("STY", 2, 4, 0, InstrucaoModo::P_ZERO_X, instrucao_sty);
        instrucoes[0x8C] = Instrucao("STY", 3, 4, 0, InstrucaoModo::ABS, instrucao_sty);

        // modos da instrução TAX
        instrucoes[0xAA] = Instrucao("TAX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tax);

        // modos da instrução TAY
        instrucoes[0xA8] = Instrucao("TAY", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tay);

        // modos da instrução TSX
        instrucoes[0xBA] = Instrucao("TSX", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tsx);

        // modos da instrução TXA
        instrucoes[0x8A] = Instrucao("TXA", 1, 2, 0, InstrucaoModo::IMPL, instrucao_txa);

        // modos da instrução TXS
        instrucoes[0x9A] = Instrucao("TXS", 1, 2, 0, InstrucaoModo::IMPL, instrucao_txs);

        // modos da instrução TYA
        instrucoes[0x98] = Instrucao("TYA", 1, 2, 0, InstrucaoModo::IMPL, instrucao_tya);

        return instrucoes;
    }

}