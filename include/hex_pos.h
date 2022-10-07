#pragma once

namespace Onoro {
/*
 * HexPos is a class for the coordinates of a hexagonal grid, with +x being at a
 * 120 degree angle with +y.
 */

struct HexPos {
  int32_t x, y;

  static constexpr HexPos origin();

  /*
   * Returns the sectant this point lies in, treating (0, 0) as the origin. The
   * first sectant (0) is only the origin tile. The second (1) is every hex with
   * (x >= 0, y >= 0, y < x). The third sectant (2) is the second sectant with
   * c_R1 applied, etc. (up to sectant 6)
   */
  uint32_t c_sec() const;

  /*
   * Rotates the point 60, 120, and 180 degrees (R1, R2, R3).
   *
   * c_R1 rotates 60 degrees about the center of the origin tile.
   * v_R2 rotates 120 degrees about the top right vertex of the origin tile.
   * e_R3 rotates 180 degrees about the center of the right edge of the origin
   * tile.
   *
   * Note: these algorithms are incompatible with each other, i.e.
   * p.c_R1().c_R1() != p.v_R2().
   */
  HexPos c_R1();
  HexPos c_R2();
  HexPos c_R3();
  HexPos c_R4();
  HexPos c_R5();
  HexPos v_R2();
  HexPos v_R4();
  HexPos e_R3();

  /*
   * [cve]_r<n>: Reflects the point across a line at angle n*30 degrees, passing
   * through:
   *  - c: the center of the origin hex
   *  - v: the top right vertex of the origin hex
   *  - e: the center of the right edge of the origin hex
   */
  HexPos c_r0();
  HexPos c_r1();
  HexPos c_r2();
  HexPos c_r3();
  HexPos c_r4();
  HexPos c_r5();
  HexPos v_r1();
  HexPos v_r3();
  HexPos v_r5();
  HexPos e_r0();
  HexPos e_r3();
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

constexpr HexPos HexPos::origin() {
  return { 0, 0 };
}

uint32_t HexPos::c_sec() const {
  if (x == 0 && y == 0) {
    return 0;
  }

  if (y < 0 || (x < 0 && y == 0)) {
    return (x < y) ? 4 : (x < 0) ? 5 : 6;
  } else {
    return (y < x) ? 1 : (x > 0) ? 2 : 3;
  }
}

HexPos HexPos::c_R1() {
  return { x - y, x };
}

HexPos HexPos::c_R2() {
  return c_R1().c_R1();
}

HexPos HexPos::c_R3() {
  return c_R2().c_R1();
}

HexPos HexPos::c_R4() {
  return c_R3().c_R1();
}

HexPos HexPos::c_R5() {
  return c_R4().c_R1();
}

HexPos HexPos::v_R2() {
  return { 1 - y, x - y };
}

HexPos HexPos::v_R4() {
  return v_R2().v_R2();
}

HexPos HexPos::e_R3() {
  return { -x, -y };
}

HexPos HexPos::c_r0() {
  return { x - y, -y };
}

HexPos HexPos::c_r1() {
  return c_r0().c_R1();
}

HexPos HexPos::c_r2() {
  return c_r0().c_R2();
}

HexPos HexPos::c_r3() {
  return c_r0().c_R3();
}

HexPos HexPos::c_r4() {
  return c_r0().c_R4();
}

HexPos HexPos::c_r5() {
  return c_r0().c_R5();
}

HexPos HexPos::v_r1() {
  return c_r1();
}

HexPos HexPos::v_r3() {
  return v_r1().v_R2();
}

HexPos HexPos::v_r5() {
  return v_r1().v_R4();
}

HexPos HexPos::e_r0() {
  return { x - y, -y };
}

HexPos HexPos::e_r3() {
  return e_r0().e_R3();
}

}  // namespace Onoro
