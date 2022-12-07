#pragma once

#include "hash_group.h"

namespace onoro {

using namespace hash_group;

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
  constexpr uint32_t c_sec() const;

  // The group of symmetries about the midpoint of a hex tile (c)
  constexpr HexPos apply_d6_c(D6 op) const;
  // The group of symmetries about the vertex of a hex tile (v)
  constexpr HexPos apply_d3_v(D3 op) const;
  // The group of symmetries about the center of an edge (e) (C2 x C2 = { c_r0,
  // c_s0 } x { c_r0, e_s3 })
  constexpr HexPos apply_k4_e(K4 op) const;
  // The group of symmetries about the line from the center of a hex tile to a
  // vertex.
  constexpr HexPos apply_c2_cv(C2 op) const;
  // The group of symmetries about the line from the center of a hex tile to the
  // midpoint of an edge.
  constexpr HexPos apply_c2_ce(C2 op) const;
  // The group of symmetries about an edge.
  constexpr HexPos apply_c2_ev(C2 op) const;

  // Applies the corresponding group operation for the given symmetry class (C,
  // V, E, CV, ...) given the ordinal of the group operation.
  // TODO remove if decide not to use
  constexpr HexPos apply(uint32_t op_ordinal, SymmetryClass symm_class) const;

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
  constexpr HexPos c_r1() const;
  constexpr HexPos c_r2() const;
  constexpr HexPos c_r3() const;
  constexpr HexPos c_r4() const;
  constexpr HexPos c_r5() const;
  constexpr HexPos v_r2() const;
  constexpr HexPos v_r4() const;
  constexpr HexPos e_r3() const;

  /*
   * [cve]_r<n>: Reflects the point across a line at angle n*30 degrees, passing
   * through:
   *  - c: the center of the origin hex
   *  - v: the top right vertex of the origin hex
   *  - e: the center of the right edge of the origin hex
   */
  constexpr HexPos c_s0() const;
  constexpr HexPos c_s1() const;
  constexpr HexPos c_s2() const;
  constexpr HexPos c_s3() const;
  constexpr HexPos c_s4() const;
  constexpr HexPos c_s5() const;
  constexpr HexPos v_s1() const;
  constexpr HexPos v_s3() const;
  constexpr HexPos v_s5() const;
  constexpr HexPos e_s0() const;
  constexpr HexPos e_s3() const;
};

template <SymmetryClass symm_class>
class SymmetryClassOp {};

template <>
class SymmetryClassOp<SymmetryClass::C> {
 public:
  typedef D6 Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos.apply_d6_c(op);
  };
};

template <>
class SymmetryClassOp<SymmetryClass::V> {
 public:
  typedef D3 Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos.apply_d3_v(op);
  };
};

template <>
class SymmetryClassOp<SymmetryClass::E> {
 public:
  typedef K4 Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos.apply_k4_e(op);
  };
};

template <>
class SymmetryClassOp<SymmetryClass::CV> {
 public:
  typedef C2 Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos.apply_c2_cv(op);
  };
};

template <>
class SymmetryClassOp<SymmetryClass::CE> {
 public:
  typedef C2 Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos.apply_c2_ce(op);
  };
};

template <>
class SymmetryClassOp<SymmetryClass::EV> {
 public:
  typedef C2 Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos.apply_c2_ev(op);
  };
};

template <>
class SymmetryClassOp<SymmetryClass::TRIVIAL> {
 public:
  typedef Trivial Group;
  typedef HexPos (*apply_fn_t)(HexPos, Group);

  static constexpr apply_fn_t apply_fn = [](HexPos pos, Group op) {
    return pos;
  };
};

typedef SymmetryClassOp<SymmetryClass::C> D6COp;
typedef SymmetryClassOp<SymmetryClass::V> D3VOp;
typedef SymmetryClassOp<SymmetryClass::E> K4EOp;
typedef SymmetryClassOp<SymmetryClass::CV> C2CVOp;
typedef SymmetryClassOp<SymmetryClass::CE> C2CEOp;
typedef SymmetryClassOp<SymmetryClass::EV> C2EVOp;
typedef SymmetryClassOp<SymmetryClass::TRIVIAL> TrivialOp;

/*
 * Calls fn templated with the corresponding symmetry class op based on
 * symm_class, forwarding all arguments following the first two arguments to the
 * macro. Returns the result of the corresponding call.
 */
#define SymmetryClassOpApplyAndReturn(symm_class, fn, ...) \
  switch (symm_class) {                                    \
    case SymmetryClass::C: {                               \
      return fn<D6COp>(__VA_ARGS__);                       \
    }                                                      \
    case SymmetryClass::V: {                               \
      return fn<D3VOp>(__VA_ARGS__);                       \
    }                                                      \
    case SymmetryClass::E: {                               \
      return fn<K4EOp>(__VA_ARGS__);                       \
    }                                                      \
    case SymmetryClass::CV: {                              \
      return fn<C2CVOp>(__VA_ARGS__);                      \
    }                                                      \
    case SymmetryClass::CE: {                              \
      return fn<C2CEOp>(__VA_ARGS__);                      \
    }                                                      \
    case SymmetryClass::EV: {                              \
      return fn<C2EVOp>(__VA_ARGS__);                      \
    }                                                      \
    case SymmetryClass::TRIVIAL: {                         \
      return fn<TrivialOp>(__VA_ARGS__);                   \
    }                                                      \
    default: {                                             \
      __builtin_unreachable();                             \
    }                                                      \
  }

constexpr bool operator==(const HexPos& a, const HexPos& b) {
  return a.x == b.x && a.y == b.y;
}

constexpr bool operator!=(const HexPos& a, const HexPos& b) {
  return a.x != b.x || a.y != b.y;
}

template <typename T>
constexpr HexPos operator*(const T& a, const HexPos& b) {
  return { static_cast<int32_t>(a * b.x), static_cast<int32_t>(a * b.y) };
}

template <typename T>
constexpr HexPos operator/(const HexPos& a, const T& b) {
  return { static_cast<int32_t>(a.x / b), static_cast<int32_t>(a.y / b) };
}

constexpr HexPos operator+(const HexPos& a, const HexPos& b) {
  return { a.x + b.x, a.y + b.y };
}

constexpr HexPos operator+=(HexPos& a, const HexPos& b) {
  a = { a.x + b.x, a.y + b.y };
  return a;
}

constexpr HexPos operator-(const HexPos& a, const HexPos& b) {
  return { a.x - b.x, a.y - b.y };
}

constexpr HexPos operator-=(HexPos& a, const HexPos& b) {
  a = { a.x - b.x, a.y - b.y };
  return a;
}

constexpr HexPos HexPos::origin() {
  return { 0, 0 };
}

constexpr uint32_t HexPos::c_sec() const {
  if (x == 0 && y == 0) {
    return 0;
  }

  if (y < 0 || (x < 0 && y == 0)) {
    return (x < y) ? 4 : (x < 0) ? 5 : 6;
  } else {
    return (y < x) ? 1 : (x > 0) ? 2 : 3;
  }
}

constexpr HexPos HexPos::apply_d6_c(D6 op) const {
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

constexpr HexPos HexPos::apply_d3_v(D3 op) const {
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

constexpr HexPos HexPos::apply_k4_e(K4 op) const {
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

constexpr HexPos HexPos::apply_c2_cv(C2 op) const {
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

constexpr HexPos HexPos::apply_c2_ce(C2 op) const {
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

constexpr HexPos HexPos::apply_c2_ev(C2 op) const {
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

constexpr HexPos HexPos::apply(uint32_t op_ordinal,
                               SymmetryClass symm_class) const {
  switch (symm_class) {
    case SymmetryClass::C: {
      return apply_d6_c(D6(op_ordinal));
    }
    case SymmetryClass::V: {
      return apply_d3_v(D3(op_ordinal));
    }
    case SymmetryClass::E: {
      return apply_k4_e(K4(op_ordinal));
    }
    case SymmetryClass::CV: {
      return apply_c2_cv(C2(op_ordinal));
    }
    case SymmetryClass::CE: {
      return apply_c2_ce(C2(op_ordinal));
    }
    case SymmetryClass::EV: {
      return apply_c2_ev(C2(op_ordinal));
    }
    case SymmetryClass::TRIVIAL: {
      return *this;
    }
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr HexPos HexPos::c_r1() const {
  return { x - y, x };
}

constexpr HexPos HexPos::c_r2() const {
  return c_r1().c_r1();
}

constexpr HexPos HexPos::c_r3() const {
  return c_r2().c_r1();
}

constexpr HexPos HexPos::c_r4() const {
  return c_r3().c_r1();
}

constexpr HexPos HexPos::c_r5() const {
  return c_r4().c_r1();
}

constexpr HexPos HexPos::v_r2() const {
  return { 1 - y, x - y };
}

constexpr HexPos HexPos::v_r4() const {
  return v_r2().v_r2();
}

constexpr HexPos HexPos::e_r3() const {
  return { 1 - x, -y };
}

constexpr HexPos HexPos::c_s0() const {
  return { x - y, -y };
}

constexpr HexPos HexPos::c_s1() const {
  return c_s0().c_r1();
}

constexpr HexPos HexPos::c_s2() const {
  return c_s0().c_r2();
}

constexpr HexPos HexPos::c_s3() const {
  return c_s0().c_r3();
}

constexpr HexPos HexPos::c_s4() const {
  return c_s0().c_r4();
}

constexpr HexPos HexPos::c_s5() const {
  return c_s0().c_r5();
}

constexpr HexPos HexPos::v_s1() const {
  return c_s1();
}

constexpr HexPos HexPos::v_s3() const {
  return v_s1().v_r2();
}

constexpr HexPos HexPos::v_s5() const {
  return v_s1().v_r4();
}

constexpr HexPos HexPos::e_s0() const {
  return c_s0();
}

constexpr HexPos HexPos::e_s3() const {
  return e_s0().e_r3();
}

}  // namespace onoro
