#pragma once

#include "hash_group.h"

namespace onoro {

using namespace hash_group;

/*
 * _HexPos is a class for the coordinates of a hexagonal grid, with +x being at
 * a 120 degree angle with +y.
 */

template <typename idx_t>
struct _HexPos {
  idx_t x, y;

  constexpr _HexPos(idx_t x, idx_t y);

  template <typename T>
  explicit constexpr operator _HexPos<T>() const;

  static constexpr _HexPos origin();

  /*
   * Returns the sectant this point lies in, treating (0, 0) as the origin. The
   * first sectant (0) is only the origin tile. The second (1) is every hex with
   * (x >= 0, y >= 0, y < x). The third sectant (2) is the second sectant with
   * c_r1 applied, etc. (up to sectant 6)
   */
  constexpr uint32_t c_sec() const;

  // The group of symmetries about the midpoint of a hex tile (c)
  constexpr _HexPos apply_d6_c(D6 op) const;
  // The group of symmetries about the vertex of a hex tile (v)
  constexpr _HexPos apply_d3_v(D3 op) const;
  // The group of symmetries about the center of an edge (e) (C2 x C2 = { c_r0,
  // c_s0 } x { c_r0, e_s3 })
  constexpr _HexPos apply_k4_e(K4 op) const;
  // The group of symmetries about the line from the center of a hex tile to a
  // vertex.
  constexpr _HexPos apply_c2_cv(C2 op) const;
  // The group of symmetries about the line from the center of a hex tile to the
  // midpoint of an edge.
  constexpr _HexPos apply_c2_ce(C2 op) const;
  // The group of symmetries about an edge.
  constexpr _HexPos apply_c2_ev(C2 op) const;

  // Applies the corresponding group operation for the given symmetry class (C,
  // V, E, CV, ...) given the ordinal of the group operation.
  // TODO remove if decide not to use
  constexpr _HexPos apply(uint32_t op_ordinal, SymmetryClass symm_class) const;

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
  constexpr _HexPos c_r1() const;
  constexpr _HexPos c_r2() const;
  constexpr _HexPos c_r3() const;
  constexpr _HexPos c_r4() const;
  constexpr _HexPos c_r5() const;
  constexpr _HexPos v_r2() const;
  constexpr _HexPos v_r4() const;
  constexpr _HexPos e_r3() const;

  /*
   * [cve]_r<n>: Reflects the point across a line at angle n*30 degrees, passing
   * through:
   *  - c: the center of the origin hex
   *  - v: the top right vertex of the origin hex
   *  - e: the center of the right edge of the origin hex
   */
  constexpr _HexPos c_s0() const;
  constexpr _HexPos c_s1() const;
  constexpr _HexPos c_s2() const;
  constexpr _HexPos c_s3() const;
  constexpr _HexPos c_s4() const;
  constexpr _HexPos c_s5() const;
  constexpr _HexPos v_s1() const;
  constexpr _HexPos v_s3() const;
  constexpr _HexPos v_s5() const;
  constexpr _HexPos e_s0() const;
  constexpr _HexPos e_s3() const;
};

using HexPos = _HexPos<int32_t>;
using HexPos16 = _HexPos<int16_t>;

template <typename idx_t>
constexpr bool operator==(const _HexPos<idx_t>& a, const _HexPos<idx_t>& b) {
  return a.x == b.x && a.y == b.y;
}

template <typename idx_t>
constexpr bool operator!=(const _HexPos<idx_t>& a, const _HexPos<idx_t>& b) {
  return a.x != b.x || a.y != b.y;
}

template <typename idx_t, typename T>
constexpr _HexPos<idx_t> operator*(const T& a, const _HexPos<idx_t>& b) {
  return { static_cast<idx_t>(a * b.x), static_cast<idx_t>(a * b.y) };
}

template <typename idx_t, typename T>
constexpr _HexPos<idx_t> operator/(const _HexPos<idx_t>& a, const T& b) {
  return { static_cast<idx_t>(a.x / b), static_cast<idx_t>(a.y / b) };
}

template <typename idx_t>
constexpr _HexPos<idx_t> operator+(const _HexPos<idx_t>& a,
                                   const _HexPos<idx_t>& b) {
  return { static_cast<idx_t>(a.x + b.x), static_cast<idx_t>(a.y + b.y) };
}

template <typename idx_t>
constexpr _HexPos<idx_t> operator+=(_HexPos<idx_t>& a,
                                    const _HexPos<idx_t>& b) {
  a = { static_cast<idx_t>(a.x + b.x), static_cast<idx_t>(a.y + b.y) };
  return a;
}

template <typename idx_t>
constexpr _HexPos<idx_t> operator-(const _HexPos<idx_t>& a,
                                   const _HexPos<idx_t>& b) {
  return { static_cast<idx_t>(a.x - b.x), static_cast<idx_t>(a.y - b.y) };
}

template <typename idx_t>
constexpr _HexPos<idx_t> operator-=(_HexPos<idx_t>& a,
                                    const _HexPos<idx_t>& b) {
  a = { static_cast<idx_t>(a.x - b.x), static_cast<idx_t>(a.y - b.y) };
  return a;
}

template <typename idx_t>
constexpr _HexPos<idx_t>::_HexPos(idx_t x, idx_t y) : x(x), y(y) {}

template <typename idx_t>
template <typename T>
constexpr _HexPos<idx_t>::operator _HexPos<T>() const {
  return _HexPos<T>(static_cast<T>(x), static_cast<T>(y));
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::origin() {
  return { 0, 0 };
}

template <typename idx_t>
constexpr uint32_t _HexPos<idx_t>::c_sec() const {
  if (x == 0 && y == 0) {
    return 0;
  }

  if (y < 0 || (x < 0 && y == 0)) {
    return (x < y) ? 4 : (x < 0) ? 5 : 6;
  } else {
    return (y < x) ? 1 : (x > 0) ? 2 : 3;
  }
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply_d6_c(D6 op) const {
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply_d3_v(D3 op) const {
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply_k4_e(K4 op) const {
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply_c2_cv(C2 op) const {
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply_c2_ce(C2 op) const {
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply_c2_ev(C2 op) const {
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::apply(uint32_t op_ordinal,
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

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_r1() const {
  return { x - y, x };
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_r2() const {
  return c_r1().c_r1();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_r3() const {
  return c_r2().c_r1();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_r4() const {
  return c_r3().c_r1();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_r5() const {
  return c_r4().c_r1();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::v_r2() const {
  return { 1 - y, x - y };
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::v_r4() const {
  return v_r2().v_r2();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::e_r3() const {
  return { 1 - x, -y };
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_s0() const {
  return { x - y, -y };
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_s1() const {
  return c_s0().c_r1();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_s2() const {
  return c_s0().c_r2();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_s3() const {
  return c_s0().c_r3();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_s4() const {
  return c_s0().c_r4();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::c_s5() const {
  return c_s0().c_r5();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::v_s1() const {
  return c_s1();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::v_s3() const {
  return v_s1().v_r2();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::v_s5() const {
  return v_s1().v_r4();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::e_s0() const {
  return c_s0();
}

template <typename idx_t>
constexpr _HexPos<idx_t> _HexPos<idx_t>::e_s3() const {
  return e_s0().e_r3();
}

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
    case onoro::SymmetryClass::C: {                        \
      return fn<onoro::D6COp>(__VA_ARGS__);                \
    }                                                      \
    case onoro::SymmetryClass::V: {                        \
      return fn<onoro::D3VOp>(__VA_ARGS__);                \
    }                                                      \
    case onoro::SymmetryClass::E: {                        \
      return fn<onoro::K4EOp>(__VA_ARGS__);                \
    }                                                      \
    case onoro::SymmetryClass::CV: {                       \
      return fn<onoro::C2CVOp>(__VA_ARGS__);               \
    }                                                      \
    case onoro::SymmetryClass::CE: {                       \
      return fn<onoro::C2CEOp>(__VA_ARGS__);               \
    }                                                      \
    case onoro::SymmetryClass::EV: {                       \
      return fn<onoro::C2EVOp>(__VA_ARGS__);               \
    }                                                      \
    case onoro::SymmetryClass::TRIVIAL: {                  \
      return fn<onoro::TrivialOp>(__VA_ARGS__);            \
    }                                                      \
    default: {                                             \
      __builtin_unreachable();                             \
    }                                                      \
  }

}  // namespace onoro
