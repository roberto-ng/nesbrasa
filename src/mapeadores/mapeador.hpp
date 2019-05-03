#pragma once

#include <cstdint>
#include <string>

namespace nesbrasa::nucleo
{
    using std::string;
    
    class Cartucho;

    class Mapeador
    {
    public:
        virtual ~Mapeador() = default;

        virtual uint8_t ler(Cartucho *cartucho, uint16_t endereco) = 0;

        virtual void escrever(Cartucho *cartucho, uint16_t endereco, uint8_t valor) = 0;

        virtual string get_nome() = 0;
    };
}