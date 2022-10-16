#pragma once

#include <utils/math/group/cyclic.h>
#include <utils/math/group/dihedral.h>
#include <utils/math/group/direct_product.h>

namespace onoro {

namespace hash_group {

typedef std::size_t game_hash_t;

typedef util::math::group::Dihedral<6> D6;
typedef util::math::group::Dihedral<3> D3;
typedef util::math::group::Cyclic<2> C2;
typedef util::math::group::DirectProduct<C2, C2> K4;

static constexpr game_hash_t apply_d6(D6 op, game_hash_t h);
static constexpr game_hash_t apply_d3(D3 op, game_hash_t h);
static constexpr game_hash_t apply_k4(K4 op, game_hash_t h);
static constexpr game_hash_t apply_c2(C2 op, game_hash_t h);

template <class Group>
static constexpr game_hash_t apply(Group op, game_hash_t h);

static constexpr game_hash_t make_invariant_d6(D6 op, game_hash_t h);
static constexpr game_hash_t make_invariant_d3(D3 op, game_hash_t h);
static constexpr game_hash_t make_invariant_k4(K4 op, game_hash_t h);
static constexpr game_hash_t make_invariant_c2(C2 op, game_hash_t h);

static constexpr game_hash_t C_MASK = UINT64_C(0x0fffffffffffffff);
static constexpr game_hash_t V_MASK = UINT64_C(0x0fffffffffffffff);
static constexpr game_hash_t E_MASK = UINT64_C(0xffffffffffffffff);

/*
 * Performs group operations on hashes commutative with xor. The four groups
 * represented are D6, D3, K4 (C2 x C2), and C2.
 */
static constexpr game_hash_t d6_r1(game_hash_t h);
static constexpr game_hash_t d6_r2(game_hash_t h);
static constexpr game_hash_t d6_r3(game_hash_t h);
static constexpr game_hash_t d6_r4(game_hash_t h);
static constexpr game_hash_t d6_r5(game_hash_t h);
static constexpr game_hash_t d6_s0(game_hash_t h);
static constexpr game_hash_t d6_s1(game_hash_t h);
static constexpr game_hash_t d6_s2(game_hash_t h);
static constexpr game_hash_t d6_s3(game_hash_t h);
static constexpr game_hash_t d6_s4(game_hash_t h);
static constexpr game_hash_t d6_s5(game_hash_t h);

static constexpr game_hash_t d3_r1(game_hash_t h);
static constexpr game_hash_t d3_r2(game_hash_t h);
static constexpr game_hash_t d3_s0(game_hash_t h);
static constexpr game_hash_t d3_s1(game_hash_t h);
static constexpr game_hash_t d3_s2(game_hash_t h);

static constexpr game_hash_t k4_a(game_hash_t h);
static constexpr game_hash_t k4_b(game_hash_t h);
static constexpr game_hash_t k4_c(game_hash_t h);

static constexpr game_hash_t c2_a(game_hash_t h);

static constexpr game_hash_t color_swap(game_hash_t h);

/*
 * Given a hash, compresses it to make it invariant under the corresponding
 * group operation.
 */
static constexpr game_hash_t make_d6_r1(game_hash_t h);
static constexpr game_hash_t make_d6_s0(game_hash_t h);
static constexpr game_hash_t make_d6_s1(game_hash_t h);
static constexpr game_hash_t make_d6_s2(game_hash_t h);
static constexpr game_hash_t make_d6_s3(game_hash_t h);
static constexpr game_hash_t make_d6_s4(game_hash_t h);
static constexpr game_hash_t make_d6_s5(game_hash_t h);

static constexpr game_hash_t make_d3_r1(game_hash_t h);
static constexpr game_hash_t make_d3_s0(game_hash_t h);
static constexpr game_hash_t make_d3_s1(game_hash_t h);
static constexpr game_hash_t make_d3_s2(game_hash_t h);

static constexpr game_hash_t make_k4_a(game_hash_t h);
static constexpr game_hash_t make_k4_b(game_hash_t h);
static constexpr game_hash_t make_k4_c(game_hash_t h);

static constexpr game_hash_t make_c2_a(game_hash_t h);

constexpr game_hash_t apply_d6(D6 op, game_hash_t h) {
  switch (op.ordinal()) {
    case D6(D6::Action::ROT, 0).ordinal(): {
      return h;
    }
    case D6(D6::Action::ROT, 1).ordinal(): {
      return d6_r1(h);
    }
    case D6(D6::Action::ROT, 2).ordinal(): {
      return d6_r2(h);
    }
    case D6(D6::Action::ROT, 3).ordinal(): {
      return d6_r3(h);
    }
    case D6(D6::Action::ROT, 4).ordinal(): {
      return d6_r4(h);
    }
    case D6(D6::Action::ROT, 5).ordinal(): {
      return d6_r5(h);
    }
    case D6(D6::Action::REFL, 0).ordinal(): {
      return d6_s0(h);
    }
    case D6(D6::Action::REFL, 1).ordinal(): {
      return d6_s1(h);
    }
    case D6(D6::Action::REFL, 2).ordinal(): {
      return d6_s2(h);
    }
    case D6(D6::Action::REFL, 3).ordinal(): {
      return d6_s3(h);
    }
    case D6(D6::Action::REFL, 4).ordinal(): {
      return d6_s4(h);
    }
    case D6(D6::Action::REFL, 5).ordinal(): {
      return d6_s5(h);
    }
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr game_hash_t apply_d3(D3 op, game_hash_t h) {
  switch (op.ordinal()) {
    case D3(D3::Action::ROT, 0).ordinal(): {
      return h;
    }
    case D3(D3::Action::ROT, 1).ordinal(): {
      return d3_r1(h);
    }
    case D3(D3::Action::ROT, 2).ordinal(): {
      return d3_r2(h);
    }
    case D3(D3::Action::REFL, 0).ordinal(): {
      return d3_s0(h);
    }
    case D3(D3::Action::REFL, 1).ordinal(): {
      return d3_s1(h);
    }
    case D3(D3::Action::REFL, 2).ordinal(): {
      return d3_s2(h);
    }
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr game_hash_t apply_k4(K4 op, game_hash_t h) {
  switch (op.ordinal()) {
    case K4(C2(0), C2(0)).ordinal(): {
      return h;
    }
    case K4(C2(1), C2(0)).ordinal(): {
      return k4_a(h);
    }
    case K4(C2(0), C2(1)).ordinal(): {
      return k4_b(h);
    }
    case K4(C2(1), C2(1)).ordinal(): {
      return k4_c(h);
    }
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr game_hash_t apply_c2(C2 op, game_hash_t h) {
  switch (op.ordinal()) {
    case C2(0).ordinal(): {
      return h;
    }
    case C2(1).ordinal(): {
      return c2_a(h);
    }
    default: {
      __builtin_unreachable();
    }
  }
}

template <>
constexpr game_hash_t apply<D6>(D6 op, game_hash_t h) {
  return apply_d6(op, h);
}

template <>
constexpr game_hash_t apply<D3>(D3 op, game_hash_t h) {
  return apply_d3(op, h);
}

template <>
constexpr game_hash_t apply<K4>(K4 op, game_hash_t h) {
  return apply_k4(op, h);
}

template <>
constexpr game_hash_t apply<C2>(C2 op, game_hash_t h) {
  return apply_c2(op, h);
}

constexpr game_hash_t make_invariant_d6(D6 op, game_hash_t h) {
  switch (op.ordinal()) {
    case D6(D6::Action::ROT, 1).ordinal(): {
      return make_d6_r1(h);
    }
    case D6(D6::Action::REFL, 0).ordinal(): {
      return make_d6_s0(h);
    }
    case D6(D6::Action::REFL, 1).ordinal(): {
      return make_d6_s1(h);
    }
    case D6(D6::Action::REFL, 2).ordinal(): {
      return make_d6_s2(h);
    }
    case D6(D6::Action::REFL, 3).ordinal(): {
      return make_d6_s3(h);
    }
    case D6(D6::Action::REFL, 4).ordinal(): {
      return make_d6_s4(h);
    }
    case D6(D6::Action::REFL, 5).ordinal(): {
      return make_d6_s5(h);
    }

    // Don't support making invariant under rotations other than the basic
    // rotation.
    case D6(D6::Action::ROT, 0).ordinal():
    case D6(D6::Action::ROT, 2).ordinal():
    case D6(D6::Action::ROT, 3).ordinal():
    case D6(D6::Action::ROT, 4).ordinal():
    case D6(D6::Action::ROT, 5).ordinal():
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr game_hash_t make_invariant_d3(D3 op, game_hash_t h) {
  switch (op.ordinal()) {
    case D3(D3::Action::ROT, 1).ordinal(): {
      return make_d3_r1(h);
    }
    case D3(D3::Action::REFL, 0).ordinal(): {
      return make_d3_s0(h);
    }
    case D3(D3::Action::REFL, 1).ordinal(): {
      return make_d3_s1(h);
    }
    case D3(D3::Action::REFL, 2).ordinal(): {
      return make_d3_s2(h);
    }

    // Don't support making invariant under rotations other than the basic
    // rotation.
    case D3(D3::Action::ROT, 0).ordinal():
    case D3(D3::Action::ROT, 2).ordinal():
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr game_hash_t make_invariant_k4(K4 op, game_hash_t h) {
  switch (op.ordinal()) {
    case K4(C2(1), C2(0)).ordinal(): {
      return make_k4_a(h);
    }
    case K4(C2(0), C2(1)).ordinal(): {
      return make_k4_b(h);
    }
    case K4(C2(1), C2(1)).ordinal(): {
      return make_k4_c(h);
    }

    // Don't support making invariant under identity.
    case K4(C2(0), C2(0)).ordinal():
    default: {
      __builtin_unreachable();
    }
  }
}

constexpr game_hash_t make_invariant_c2(C2 op, game_hash_t h) {
  // Only one symmetry operation is possible.
  return make_c2_a(h);
}

constexpr game_hash_t d6_r1(game_hash_t h) {
  return ((h << 10) | (h >> 50)) & C_MASK;
}

constexpr game_hash_t d6_r2(game_hash_t h) {
  return d6_r1(d6_r1(h));
}

constexpr game_hash_t d6_r3(game_hash_t h) {
  return d6_r1(d6_r2(h));
}

constexpr game_hash_t d6_r4(game_hash_t h) {
  return d6_r1(d6_r3(h));
}

constexpr game_hash_t d6_r5(game_hash_t h) {
  return d6_r1(d6_r4(h));
}

constexpr game_hash_t d6_s0(game_hash_t h) {
  game_hash_t b14 = h & 0x000000ffc00003ff;
  game_hash_t b26 = h & 0x0ffc0000000ffc00;
  game_hash_t b35 = h & 0x0003ff003ff00000;

  b26 = (b26 << 40) | (b26 >> 40);
  b35 = ((b35 << 20) | (b35 >> 20)) & 0x0003ff003ff00000;
  return b14 | b26 | b35;
}

constexpr game_hash_t d6_s1(game_hash_t h) {
  game_hash_t b12 = h & 0x00000000000fffff;
  game_hash_t b36 = h & 0x0ffc00003ff00000;
  game_hash_t b45 = h & 0x0003ffffc0000000;

  b12 = ((b12 << 10) | (b12 >> 10)) & 0x00000000000fffff;
  b36 = (b36 << 30) | (b36 >> 30);
  b45 = ((b45 << 10) | (b45 >> 10)) & 0x0003ffffc0000000;
  return b12 | b36 | b45;
}

constexpr game_hash_t d6_s2(game_hash_t h) {
  game_hash_t b13 = h & 0x000000003ff003ff;
  game_hash_t b25 = h & 0x0003ff00000ffc00;
  game_hash_t b46 = h & 0x0ffc00ffc0000000;

  b13 = ((b13 << 20) | (b13 >> 20)) & 0x000000003ff003ff;
  b46 = ((b46 << 20) | (b46 >> 20)) & 0x0ffc00ffc0000000;
  return b13 | b25 | b46;
}

constexpr game_hash_t d6_s3(game_hash_t h) {
  game_hash_t b14 = h & 0x000000ffc00003ff;
  game_hash_t b23 = h & 0x000000003ffffc00;
  game_hash_t b56 = h & 0x0fffff0000000000;

  b14 = ((b14 << 30) | (b14 >> 30)) & 0x000000ffc00003ff;
  b23 = ((b23 << 10) | (b23 >> 10)) & 0x000000003ffffc00;
  b56 = ((b56 << 10) | (b56 >> 10)) & 0x0fffff0000000000;
  return b14 | b23 | b56;
}

constexpr game_hash_t d6_s4(game_hash_t h) {
  game_hash_t b15 = h & 0x0003ff00000003ff;
  game_hash_t b24 = h & 0x000000ffc00ffc00;
  game_hash_t b36 = h & 0x0ffc00003ff00000;

  b15 = (b15 << 40) | (b15 >> 40);
  b24 = ((b24 << 20) | (b24 >> 20)) & 0x000000ffc00ffc00;
  return b15 | b24 | b36;
}

constexpr game_hash_t d6_s5(game_hash_t h) {
  game_hash_t b16 = h & 0x0ffc0000000003ff;
  game_hash_t b25 = h & 0x0003ff00000ffc00;
  game_hash_t b34 = h & 0x000000fffff00000;

  b16 = (b16 << 50) | (b16 >> 50);
  b25 = (b25 << 30) | (b25 >> 30);
  b34 = ((b34 << 10) | (b34 >> 10)) & 0x000000fffff00000;
  return b16 | b25 | b34;
}

constexpr game_hash_t d3_r1(game_hash_t h) {
  return ((h << 20) | (h >> 40)) & V_MASK;
}

constexpr game_hash_t d3_r2(game_hash_t h) {
  return d3_r1(d3_r1(h));
}

constexpr game_hash_t d3_s0(game_hash_t h) {
  game_hash_t b1 = h & 0x00000000000fffff;
  game_hash_t b2 = h & 0x000000fffff00000;
  game_hash_t b3 = h & 0x0fffff0000000000;

  b2 = b2 << 20;
  b3 = b3 >> 20;
  return b1 | b2 | b3;
}

constexpr game_hash_t d3_s1(game_hash_t h) {
  game_hash_t b1 = h & 0x00000000000fffff;
  game_hash_t b2 = h & 0x000000fffff00000;
  game_hash_t b3 = h & 0x0fffff0000000000;

  b1 = b1 << 20;
  b2 = b2 >> 20;
  return b1 | b2 | b3;
}

constexpr game_hash_t d3_s2(game_hash_t h) {
  game_hash_t b13 = h & 0x0fffff00000fffff;
  game_hash_t b2 = h & 0x000000fffff00000;

  b13 = (b13 << 40) | (b13 >> 40);
  return b13 | b2;
}

constexpr game_hash_t k4_a(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

constexpr game_hash_t k4_b(game_hash_t h) {
  game_hash_t b13 = h & 0x0000ffff0000ffff;
  game_hash_t b24 = h & 0xffff0000ffff0000;

  return (b13 << 16) | (b24 >> 16);
}

constexpr game_hash_t k4_c(game_hash_t h) {
  game_hash_t b = __builtin_bswap64(h);
  game_hash_t b1357 = b & 0x00ff00ff00ff00ff;
  game_hash_t b2468 = b & 0xff00ff00ff00ff00;

  return (b1357 << 8) | (b2468 >> 8);
}

constexpr game_hash_t c2_a(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

constexpr game_hash_t color_swap(game_hash_t h) {
  game_hash_t hl = h & UINT64_C(0x5555555555555555);
  game_hash_t hr = h & UINT64_C(0xaaaaaaaaaaaaaaaa);

  return (hl << 1) | (hr >> 1);
}

constexpr game_hash_t make_d6_r1(game_hash_t h) {
  // Repeat the first 10 bits across the remaining 50 bits, leaving the end
  // zeroed out.
  game_hash_t b = h & UINT64_C(0x3ff);
  b = b | (b << 10);
  return b | (b << 20) | (b << 40);
}

constexpr game_hash_t make_d6_s0(game_hash_t h) {
  game_hash_t b14 = h & 0x000000ffc00003ff;
  game_hash_t b26 = h & 0x00000000000ffc00;
  game_hash_t b35 = h & 0x000000003ff00000;

  b26 = b26 | (b26 << 40);
  b35 = b35 | (b35 << 20);
  return b14 | b26 | b35;
}

constexpr game_hash_t make_d6_s1(game_hash_t h) {
  game_hash_t b12 = h & 0x00000000000003ff;
  game_hash_t b36 = h & 0x000000003ff00000;
  game_hash_t b45 = h & 0x000000ffc0000000;

  b12 = b12 | (b12 << 10);
  b36 = b36 | (b36 << 30);
  b45 = b45 | (b45 << 10);
  return b12 | b36 | b45;
}

constexpr game_hash_t make_d6_s2(game_hash_t h) {
  game_hash_t b13 = h & 0x00000000000003ff;
  game_hash_t b25 = h & 0x0003ff00000ffc00;
  game_hash_t b46 = h & 0x000000ffc0000000;

  b13 = b13 | (b13 << 20);
  b46 = b46 | (b46 << 20);
  return b13 | b25 | b46;
}

constexpr game_hash_t make_d6_s3(game_hash_t h) {
  game_hash_t b14 = h & 0x00000000000003ff;
  game_hash_t b23 = h & 0x00000000000ffc00;
  game_hash_t b56 = h & 0x0003ff0000000000;

  b14 = b14 | (b14 << 30);
  b23 = b23 | (b23 << 10);
  b56 = b56 | (b56 << 10);
  return b14 | b23 | b56;
}

constexpr game_hash_t make_d6_s4(game_hash_t h) {
  game_hash_t b15 = h & 0x00000000000003ff;
  game_hash_t b24 = h & 0x00000000000ffc00;
  game_hash_t b36 = h & 0x0ffc00003ff00000;

  b15 = b15 | (b15 << 40);
  b24 = b24 | (b24 << 20);
  return b15 | b24 | b36;
}

constexpr game_hash_t make_d6_s5(game_hash_t h) {
  game_hash_t b16 = h & 0x00000000000003ff;
  game_hash_t b25 = h & 0x00000000000ffc00;
  game_hash_t b34 = h & 0x000000003ff00000;

  b16 = b16 | (b16 << 50);
  b25 = b25 | (b25 << 30);
  b34 = b34 | (b34 << 10);
  return b16 | b25 | b34;
}

constexpr game_hash_t make_d3_r1(game_hash_t h) {
  // Repeat the first 21 bits across the remaining 42 bits, leaving the end
  // zeroed out.
  game_hash_t b = h & UINT64_C(0xfffff);
  return b | (b << 20) | (b << 40);
}

constexpr game_hash_t make_d3_s0(game_hash_t h) {
  game_hash_t b1 = h & 0x00000000000fffff;
  game_hash_t b23 = h & 0x000000fffff00000;

  b23 = b23 | (b23 << 20);
  return b1 | b23;
}

constexpr game_hash_t make_d3_s1(game_hash_t h) {
  game_hash_t b12 = h & 0x00000000000fffff;
  game_hash_t b3 = h & 0x0fffff0000000000;

  b12 = b12 | (b12 << 20);
  return b12 | b3;
}

constexpr game_hash_t make_d3_s2(game_hash_t h) {
  game_hash_t b13 = h & 0x00000000000fffff;
  game_hash_t b2 = h & 0x000000fffff00000;

  b13 = b13 | (b13 << 40);
  return b13 | b2;
}

constexpr game_hash_t make_k4_a(game_hash_t h) {
  game_hash_t b12 = h & 0x00000000ffffffff;

  return b12 | (b12 << 32);
}

constexpr game_hash_t make_k4_b(game_hash_t h) {
  game_hash_t b13 = h & 0x0000ffff0000ffff;

  return b13 | (b13 << 16);
}

constexpr game_hash_t make_k4_c(game_hash_t h) {
  game_hash_t b1 = h & UINT64_C(0xffff);
  game_hash_t b2 = h & UINT64_C(0xffff0000);
  return b1 | b2 | (b2 << 16) | (b1 << 48);
}

constexpr game_hash_t make_c2_a(game_hash_t h) {
  game_hash_t b12 = h & 0x00000000ffffffff;

  return b12 | (b12 << 32);
}

}  // namespace hash_group

}  // namespace onoro
