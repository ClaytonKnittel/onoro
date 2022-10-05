#pragma once

namespace Onoro {
/*
 * HexPos is a class for the coordinates of a hexagonal grid, with +x being at a
 * 120 degree angle with +y.
 */

struct HexPos {
  int32_t x, y;

  /*
   * Rotation operations for D6, Rn = rotate by n*60 degrees about the center of
   * the tile at the origin, rn = reflect across a line through the origin
   * angled n*30 degrees above the +x axis..
   */
  HexPos D6_R1() const;
  HexPos D6_R2() const;
  HexPos D6_R3() const;
  HexPos D6_R4() const;
  HexPos D6_R5() const;
  HexPos D6_r0() const;
  HexPos D6_r1() const;
  HexPos D6_r2() const;
  HexPos D6_r3() const;
  HexPos D6_r4() const;
  HexPos D6_r5() const;
};

bool operator==(const HexPos& a, const HexPos& b) {
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const HexPos& a, const HexPos& b) {
  return a.x != b.x || a.y != b.y;
}

template <typename T>
HexPos operator*(const T& a, const HexPos& b) {
  return { static_cast<int32_t>(a * b.x), static_cast<int32_t>(a * b.y) };
}

HexPos operator+(const HexPos& a, const HexPos& b) {
  return { a.x + b.x, a.y + b.y };
}

HexPos operator+=(HexPos& a, const HexPos& b) {
  a = { a.x + b.x, a.y + b.y };
  return a;
}

HexPos operator-(const HexPos& a, const HexPos& b) {
  return { a.x - b.x, a.y - b.y };
}

HexPos operator-=(HexPos& a, const HexPos& b) {
  a = { a.x - b.x, a.y - b.y };
  return a;
}

}  // namespace Onoro
