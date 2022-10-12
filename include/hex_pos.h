#pragma once

#include <utils/math/group/cyclic.h>
#include <utils/math/group/dihedral.h>
#include <utils/math/group/direct_product.h>

namespace onoro {

typedef util::math::group::Dihedral<6> D6;
typedef util::math::group::Dihedral<3> D3;
typedef util::math::group::Cyclic<2> C2;
typedef util::math::group::DirectProduct<C2, C2> K4;

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
   * c_r1 applied, etc. (up to sectant 6)
   */
  uint32_t c_sec() const;

  // The group of symmetries about the midpoint of a hex tile (c)
  HexPos apply_d6_c(D6 op) const;
  // The group of symmetries about the vertex of a hex tile (v)
  HexPos apply_d3_v(D3 op) const;
  // The group of symmetries about the center of an edge (e) (C2 x C2 = { c_r0,
  // c_s0 } x { c_r0, e_s3 })
  HexPos apply_k4_e(K4 op) const;
  // The group of symmetries about the line from the center of a hex tile to a
  // vertex.
  HexPos apply_c2_cv(C2 op) const;
  // The group of symmetries about the line from the center of a hex tile to the
  // midpoint of an edge.
  HexPos apply_c2_ce(C2 op) const;
  // The group of symmetries about an edge.
  HexPos apply_c2_ev(C2 op) const;

  /*
   * Rotates the point 60, 120, and 180 degrees (R1, R2, R3).
   *
   * c_r1 rotates 60 degrees about the center of the origin tile.
   * v_r2 rotates 120 degrees about the top right vertex of the origin tile.
   * e_r3 rotates 180 degrees about the center of the right edge of the origin
   * tile.
   *
   * Note: these algorithms are incompatible with each other, i.e.
   * p.c_r1().c_r1() != p.v_r2().
   */
  HexPos c_r1() const;
  HexPos c_r2() const;
  HexPos c_r3() const;
  HexPos c_r4() const;
  HexPos c_r5() const;
  HexPos v_r2() const;
  HexPos v_r4() const;
  HexPos e_r3() const;

  /*
   * [cve]_r<n>: Reflects the point across a line at angle n*30 degrees, passing
   * through:
   *  - c: the center of the origin hex
   *  - v: the top right vertex of the origin hex
   *  - e: the center of the right edge of the origin hex
   */
  HexPos c_s0() const;
  HexPos c_s1() const;
  HexPos c_s2() const;
  HexPos c_s3() const;
  HexPos c_s4() const;
  HexPos c_s5() const;
  HexPos v_s1() const;
  HexPos v_s3() const;
  HexPos v_s5() const;
  HexPos e_s0() const;
  HexPos e_s3() const;
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

HexPos HexPos::apply_d6_c(D6 op) const {
  switch (op.ordinal()) {
    case D6(D6::Action::ROT, 0).ordinal(): {
      return *this;
    }
    case D6(D6::Action::ROT, 1).ordinal(): {
      return c_r1();
    }
    case D6(D6::Action::ROT, 2).ordinal(): {
      return c_r2();
    }
    case D6(D6::Action::ROT, 3).ordinal(): {
      return c_r3();
    }
    case D6(D6::Action::ROT, 4).ordinal(): {
      return c_r4();
    }
    case D6(D6::Action::ROT, 5).ordinal(): {
      return c_r5();
    }
    case D6(D6::Action::REFL, 0).ordinal(): {
      return c_s0();
    }
    case D6(D6::Action::REFL, 1).ordinal(): {
      return c_s1();
    }
    case D6(D6::Action::REFL, 2).ordinal(): {
      return c_s2();
    }
    case D6(D6::Action::REFL, 3).ordinal(): {
      return c_s3();
    }
    case D6(D6::Action::REFL, 4).ordinal(): {
      return c_s4();
    }
    case D6(D6::Action::REFL, 5).ordinal(): {
      return c_s5();
    }
    default: {
      __builtin_unreachable();
    }
  }
}

HexPos HexPos::apply_d3_v(D3 op) const {
  switch (op.ordinal()) {
    case D3(D3::Action::ROT, 0).ordinal(): {
      return *this;
    }
    case D3(D3::Action::ROT, 1).ordinal(): {
      return v_r2();
    }
    case D3(D3::Action::ROT, 2).ordinal(): {
      return v_r4();
    }
    case D3(D3::Action::REFL, 0).ordinal(): {
      return v_s1();
    }
    case D3(D3::Action::REFL, 1).ordinal(): {
      return v_s3();
    }
    case D3(D3::Action::REFL, 2).ordinal(): {
      return v_s5();
    }
    default: {
      __builtin_unreachable();
    }
  }
}

HexPos HexPos::apply_k4_e(K4 op) const {
  switch (op.ordinal()) {
    case K4(C2(0), C2(0)).ordinal(): {
      return *this;
    }
    case K4(C2(1), C2(0)).ordinal(): {
      return e_s0();
    }
    case K4(C2(0), C2(1)).ordinal(): {
      return e_s3();
    }
    case K4(C2(1), C2(1)).ordinal(): {
      return e_r3();
    }
    default: {
      __builtin_unreachable();
    }
  }
}

HexPos HexPos::apply_c2_cv(C2 op) const {
  switch (op.ordinal()) {
    case C2(0).ordinal(): {
      return *this;
    }
    case C2(1).ordinal(): {
      return c_s1();
    }
    default: {
      __builtin_unreachable();
    }
  }
}

HexPos HexPos::apply_c2_ce(C2 op) const {
  switch (op.ordinal()) {
    case C2(0).ordinal(): {
      return *this;
    }
    case C2(1).ordinal(): {
      return c_s0();
    }
    default: {
      __builtin_unreachable();
    }
  }
}

HexPos HexPos::apply_c2_ev(C2 op) const {
  switch (op.ordinal()) {
    case C2(0).ordinal(): {
      return *this;
    }
    case C2(1).ordinal(): {
      return e_s3();
    }
    default: {
      __builtin_unreachable();
    }
  }
}

HexPos HexPos::c_r1() const {
  return { x - y, x };
}

HexPos HexPos::c_r2() const {
  return c_r1().c_r1();
}

HexPos HexPos::c_r3() const {
  return c_r2().c_r1();
}

HexPos HexPos::c_r4() const {
  return c_r3().c_r1();
}

HexPos HexPos::c_r5() const {
  return c_r4().c_r1();
}

HexPos HexPos::v_r2() const {
  return { 1 - y, x - y };
}

HexPos HexPos::v_r4() const {
  return v_r2().v_r2();
}

HexPos HexPos::e_r3() const {
  return { 1 - x, -y };
}

HexPos HexPos::c_s0() const {
  return { x - y, -y };
}

HexPos HexPos::c_s1() const {
  return c_s0().c_r1();
}

HexPos HexPos::c_s2() const {
  return c_s0().c_r2();
}

HexPos HexPos::c_s3() const {
  return c_s0().c_r3();
}

HexPos HexPos::c_s4() const {
  return c_s0().c_r4();
}

HexPos HexPos::c_s5() const {
  return c_s0().c_r5();
}

HexPos HexPos::v_s1() const {
  return c_s1();
}

HexPos HexPos::v_s3() const {
  return v_s1().v_r2();
}

HexPos HexPos::v_s5() const {
  return v_s1().v_r4();
}

HexPos HexPos::e_s0() const {
  return c_s0();
}

HexPos HexPos::e_s3() const {
  return e_s0().e_r3();
}

}  // namespace onoro
