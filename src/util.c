#include "util.h"

inline bool
buscar_bit(uint8_t byte, uint8_t pos)
{
  // dar a volta quando a posição do bit for maior que 7
  pos = pos % 8;

  const uint8_t tmp = byte & (1 << pos);
  return (tmp >> pos) == 0;
}
