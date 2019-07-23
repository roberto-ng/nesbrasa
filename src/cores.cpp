#include "cores.hpp"

namespace nesbrasa::nucleo::cores
{
    cor_rgb buscar_cor_rgb(byte valor)
    {
        return tabela_rgb.at(valor % 0x40);
    }

    array<cor_rgb, 0x40> tabela_rgb {
        //        R                         G                         B
        cor_rgb { static_cast<byte>(0x084), static_cast<byte>(0x084), static_cast<byte>(0x084), }, // $00
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x030), static_cast<byte>(0x116), }, // $01
        cor_rgb { static_cast<byte>(0x008), static_cast<byte>(0x016), static_cast<byte>(0x144), }, // $02
        cor_rgb { static_cast<byte>(0x048), static_cast<byte>(0x000), static_cast<byte>(0x136), }, // $03
        cor_rgb { static_cast<byte>(0x068), static_cast<byte>(0x000), static_cast<byte>(0x100), }, // $04
        cor_rgb { static_cast<byte>(0x092), static_cast<byte>(0x000), static_cast<byte>(0x048), }, // $05
        cor_rgb { static_cast<byte>(0x084), static_cast<byte>(0x004), static_cast<byte>(0x000), }, // $06
        cor_rgb { static_cast<byte>(0x060), static_cast<byte>(0x024), static_cast<byte>(0x000), }, // $07
        cor_rgb { static_cast<byte>(0x032), static_cast<byte>(0x042), static_cast<byte>(0x000), }, // $08
        cor_rgb { static_cast<byte>(0x008), static_cast<byte>(0x058), static_cast<byte>(0x000), }, // $09
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x064), static_cast<byte>(0x000), }, // $0A
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x060), static_cast<byte>(0x000), }, // $0B
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x050), static_cast<byte>(0x060), }, // $0C
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $0D
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $0E
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $0F
        cor_rgb { static_cast<byte>(0x152), static_cast<byte>(0x150), static_cast<byte>(0x152), }, // $10
        cor_rgb { static_cast<byte>(0x008), static_cast<byte>(0x076), static_cast<byte>(0x196), }, // $11
        cor_rgb { static_cast<byte>(0x040), static_cast<byte>(0x050), static_cast<byte>(0x236), }, // $12
        cor_rgb { static_cast<byte>(0x092), static_cast<byte>(0x030), static_cast<byte>(0x228), }, // $13
        cor_rgb { static_cast<byte>(0x136), static_cast<byte>(0x020), static_cast<byte>(0x176), }, // $14
        cor_rgb { static_cast<byte>(0x160), static_cast<byte>(0x020), static_cast<byte>(0x100), }, // $15
        cor_rgb { static_cast<byte>(0x152), static_cast<byte>(0x034), static_cast<byte>(0x032), }, // $16
        cor_rgb { static_cast<byte>(0x120), static_cast<byte>(0x060), static_cast<byte>(0x000), }, // $17
        cor_rgb { static_cast<byte>(0x084), static_cast<byte>(0x090), static_cast<byte>(0x000), }, // $18
        cor_rgb { static_cast<byte>(0x040), static_cast<byte>(0x114), static_cast<byte>(0x000), }, // $19
        cor_rgb { static_cast<byte>(0x008), static_cast<byte>(0x124), static_cast<byte>(0x000), }, // $1A
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x118), static_cast<byte>(0x040), }, // $1B
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x102), static_cast<byte>(0x120), }, // $1C
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $1D
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $1E
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $1F
        cor_rgb { static_cast<byte>(0x236), static_cast<byte>(0x238), static_cast<byte>(0x236), }, // $20
        cor_rgb { static_cast<byte>(0x076), static_cast<byte>(0x154), static_cast<byte>(0x236), }, // $21
        cor_rgb { static_cast<byte>(0x120), static_cast<byte>(0x124), static_cast<byte>(0x236), }, // $22
        cor_rgb { static_cast<byte>(0x176), static_cast<byte>(0x098), static_cast<byte>(0x236), }, // $23
        cor_rgb { static_cast<byte>(0x228), static_cast<byte>(0x084), static_cast<byte>(0x236), }, // $24
        cor_rgb { static_cast<byte>(0x236), static_cast<byte>(0x088), static_cast<byte>(0x180), }, // $25
        cor_rgb { static_cast<byte>(0x236), static_cast<byte>(0x106), static_cast<byte>(0x100), }, // $26
        cor_rgb { static_cast<byte>(0x212), static_cast<byte>(0x136), static_cast<byte>(0x032), }, // $27
        cor_rgb { static_cast<byte>(0x160), static_cast<byte>(0x170), static_cast<byte>(0x000), }, // $28
        cor_rgb { static_cast<byte>(0x116), static_cast<byte>(0x196), static_cast<byte>(0x000), }, // $29
        cor_rgb { static_cast<byte>(0x076), static_cast<byte>(0x208), static_cast<byte>(0x032), }, // $2A
        cor_rgb { static_cast<byte>(0x056), static_cast<byte>(0x204), static_cast<byte>(0x108), }, // $2B
        cor_rgb { static_cast<byte>(0x056), static_cast<byte>(0x180), static_cast<byte>(0x204), }, // $2C
        cor_rgb { static_cast<byte>(0x060), static_cast<byte>(0x060), static_cast<byte>(0x060), }, // $2D
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $2E
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $2F
        cor_rgb { static_cast<byte>(0x263), static_cast<byte>(0x238), static_cast<byte>(0x263), }, // $30
        cor_rgb { static_cast<byte>(0x168), static_cast<byte>(0x204), static_cast<byte>(0x236), }, // $31
        cor_rgb { static_cast<byte>(0x188), static_cast<byte>(0x188), static_cast<byte>(0x236), }, // $32
        cor_rgb { static_cast<byte>(0x212), static_cast<byte>(0x178), static_cast<byte>(0x236), }, // $33
        cor_rgb { static_cast<byte>(0x236), static_cast<byte>(0x174), static_cast<byte>(0x236), }, // $34
        cor_rgb { static_cast<byte>(0x236), static_cast<byte>(0x174), static_cast<byte>(0x212), }, // $35
        cor_rgb { static_cast<byte>(0x236), static_cast<byte>(0x180), static_cast<byte>(0x176), }, // $36
        cor_rgb { static_cast<byte>(0x228), static_cast<byte>(0x196), static_cast<byte>(0x144), }, // $37
        cor_rgb { static_cast<byte>(0x204), static_cast<byte>(0x210), static_cast<byte>(0x120), }, // $38
        cor_rgb { static_cast<byte>(0x180), static_cast<byte>(0x222), static_cast<byte>(0x120), }, // $39
        cor_rgb { static_cast<byte>(0x168), static_cast<byte>(0x226), static_cast<byte>(0x144), }, // $3A
        cor_rgb { static_cast<byte>(0x152), static_cast<byte>(0x226), static_cast<byte>(0x180), }, // $3B
        cor_rgb { static_cast<byte>(0x160), static_cast<byte>(0x214), static_cast<byte>(0x228), }, // $3C
        cor_rgb { static_cast<byte>(0x160), static_cast<byte>(0x162), static_cast<byte>(0x160), }, // $3D 
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $3E
        cor_rgb { static_cast<byte>(0x000), static_cast<byte>(0x000), static_cast<byte>(0x000), }, // $3F
    };
}