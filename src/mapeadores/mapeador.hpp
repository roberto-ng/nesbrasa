#pragma once

#include <cstdint>

namespace nesbrasa
{

class Cartucho;

class IMapeador
{
public:
    virtual ~IMapeador() = default;

    virtual uint8_t ler(Cartucho *cartucho, uint16_t endereco) = 0;

    virtual void escrever(Cartucho *cartucho, uint16_t endereco, uint8_t valor) = 0;
};

}