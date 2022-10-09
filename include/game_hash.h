#pragma once

#include <iomanip>

#include "dihedral_group.h"
#include "hex_pos.h"
#include "onoro.h"
#include "random.h"

namespace Onoro {

typedef std::size_t game_hash_t;

typedef DihedralEl<6> D6;
typedef DihedralEl<3> D3;

/*
 * GameHash has one of five forms, depending on the board group structure:
 * - D6 (12 symmetries):
 *   - divided into 6 contiguous regions of 10 bits, with remaining 4 bits being
 *     zero
 * - D3 (6 symmetries):
 *   - divided into 3 contiguous regions of 21 bits, with last bit being 0
 * - C2+C2 (4 symmetries):
 *   - divided into 4 contiguous regions of 16 bits
 * - C2 (2 symmetries):
 *   - divided into 2 contiguous regions of 32 bits
 * - trivial:
 *   - entirely random
 *
 * Group operations correspond to shuffling these regions of bits around,
 * analogous to how the group operations on board tiles shuffle around tiles in
 * groups of <n> tiles (<n> is the number of contiguous regions in the hashes)
 *
 * For tiles which map to themselves under certain group operations, those group
 * operations must have no effect on the hash of those tiles. This will mean
 * repeating bit regions across the hash in some way.
 */
template <uint32_t NPawns>
class GameHash {
 public:
  GameHash();

  game_hash_t operator()(const Game<NPawns>& g) const noexcept;

  constexpr bool validate() const;

  static std::string printD6Hash(game_hash_t);
  static std::string printD3Hash(game_hash_t);

 private:
  struct HashEl {
    // hash to use for black pawn in this tile.
    game_hash_t black_hash;
    // hash to use for white pawn in this tile.
    game_hash_t white_hash;
  };

  // Returns the length of the symm tables in one dimension.
  static constexpr uint32_t getSymmTableLen();

  // Returns the total number of tiles in each symm table.
  static constexpr uint32_t getSymmTableSize();

  // Returns the tile designated as the origin tile for this board.
  static constexpr HexPos getCenter();

  // Translates from an absolute index to an index object.
  static constexpr idx_t toIdx(uint32_t i);

  static constexpr uint32_t fromIdx(idx_t idx);

  static constexpr bool inBounds(idx_t idx);

  static constexpr game_hash_t apply_d6(D6 op, game_hash_t h);
  static constexpr game_hash_t apply_d3(D3 op, game_hash_t h);

  static constexpr game_hash_t make_invariant_d6(D6 op, game_hash_t h);
  static constexpr game_hash_t make_invariant_d3(D3 op, game_hash_t h);

  static constexpr game_hash_t C_MASK = UINT64_C(0x0fffffffffffffff);
  static constexpr game_hash_t V_MASK = UINT64_C(0x7fffffffffffffff);
  static constexpr game_hash_t E_MASK = UINT64_C(0xffffffffffffffff);

  /*
   * Rotates the hash 60, 120, and 180 degrees (R1, R2, R3).
   *
   * c_r1 rotates 60 degrees about the center of the origin tile.
   * v_r2 rotates 120 degrees about the top right vertex of the origin tile.
   * e_r3 rotates 180 degrees about the center of the right edge of the origin
   * tile.
   *
   * Note: these algorithms are incompatible with each other, i.e.
   * c_r1(c_r1(x)) != v_r2(x).
   */
  static game_hash_t c_r1(game_hash_t h);
  static game_hash_t c_r2(game_hash_t h);
  static game_hash_t c_r3(game_hash_t h);
  static game_hash_t c_r4(game_hash_t h);
  static game_hash_t c_r5(game_hash_t h);
  static game_hash_t v_r2(game_hash_t h);
  static game_hash_t v_r4(game_hash_t h);
  static game_hash_t e_r3(game_hash_t h);

  /*
   * [cve]_r<n>: Reflects the hash across a line at angle n*30 degrees,
   * passing through:
   *  - c: the center of the origin hex
   *  - v: the top right vertex of the origin hex
   *  - e: the center of the right edge of the origin hex
   */
  static game_hash_t c_s0(game_hash_t h);
  static game_hash_t c_s1(game_hash_t h);
  static game_hash_t c_s2(game_hash_t h);
  static game_hash_t c_s3(game_hash_t h);
  static game_hash_t c_s4(game_hash_t h);
  static game_hash_t c_s5(game_hash_t h);
  static game_hash_t v_s1(game_hash_t h);
  static game_hash_t v_s3(game_hash_t h);
  static game_hash_t v_s5(game_hash_t h);
  static game_hash_t e_s0(game_hash_t h);
  static game_hash_t e_s3(game_hash_t h);

  /*
   * Given a hash, compresses it to make it invariant under the corresponding
   * group operation.
   */
  static game_hash_t make_c_r1(game_hash_t h);
  static game_hash_t make_v_r2(game_hash_t h);
  static game_hash_t make_e_r3(game_hash_t h);
  static game_hash_t make_c_s0(game_hash_t h);
  static game_hash_t make_c_s1(game_hash_t h);
  static game_hash_t make_c_s2(game_hash_t h);
  static game_hash_t make_c_s3(game_hash_t h);
  static game_hash_t make_c_s4(game_hash_t h);
  static game_hash_t make_c_s5(game_hash_t h);
  static game_hash_t make_v_s1(game_hash_t h);
  static game_hash_t make_v_s3(game_hash_t h);
  static game_hash_t make_v_s5(game_hash_t h);
  static game_hash_t make_e_s0(game_hash_t h);
  static game_hash_t make_e_s3(game_hash_t h);

  void initSymmTables();

  /*
   * The infinite hexagonal plane centered at a fixed point forms a dihedral
   * group D6, which has group operations R1 (Rn = rotate by n*60 degrees
   * about the fixed point) and r0 (rn = reflect about a line at angle n*pi/6
   * through the fixed point).
   *
   * We are interested in seven subgroups of this group, for seven cases that
   * the game board can be in, categorized by where the "center of mass" of
   * the board lies:
   *  - D6: (R[1-5] r[0-5]) the center of mass lies exactly in the center of a
   *      hexagonal cell
   *  - D3: (R2 R4 r1 r3 r5) the center of mass lies on a vertex of a
   * hexagonal cell.
   *  - C2 + C2: (R3 r0 r3) the center of mass lies on the midpoint of an edge
   *      of a hexagonal cell.
   *  - C2: (r1) the center of mass lies along a line extending from the
   * middle of a hexagonal cell to one of its vertices.
   *  - C2: (r0) the center of mass lies along a line extending from the
   * middle of a hexagonal cell to the center of one of its edges.
   *  - C2: (r4 + translation) the center of mass lies on an edge of a
   * hexagonal cell.
   *  - trivial: all other cases
   *
   * By choosing a preferred orientation of the center of mass (i.e. let the
   * tile containing the center of mass be (0, 0), and rotate/reflect the
   * plane about the new origin until the center of mass lies in the triangle
   * formed by the origin (center of the hexagon at (0, 0)), the vertex in the
   * +x direction of the origin tile, and the midpoint of the edge extending
   * in the +y direction from this vertex), we can ensure that all symmetries
   * of the board will be reachable under the operations in the corresponding
   * subgroup depending on where the center of mass lies.
   */
  HashEl d6_table_[getSymmTableSize()];
  HashEl d3_table_[getSymmTableSize()];
  HashEl c2_c2_table_[getSymmTableSize()];
  HashEl c2_cv_table_[getSymmTableSize()];
  HashEl c2_ce_table_[getSymmTableSize()];
  HashEl c2_ev_table_[getSymmTableSize()];
  HashEl trivial_table_[getSymmTableSize()];
};

template <uint32_t NPawns>
GameHash<NPawns>::GameHash() {
  initSymmTables();
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::operator()(const Game<NPawns>& g) const noexcept {
  return 0;
}

template <uint32_t NPawns>
constexpr bool GameHash<NPawns>::validate() const {
  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    p -= getCenter();

    const HashEl& h = d6_table_[i];

    HexPos s = p;
    D6 op;
    for (uint32_t _i = 0; _i < 5; _i++) {
      s = s.c_r1();
      op = D6(D6::Action::ROT, 1) * op;

      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            d6_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_d6(op, h.black_hash) != hs.black_hash ||
            apply_d6(op, h.white_hash) != hs.white_hash) {
          printf(
              "D6 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD6Hash(h.black_hash).c_str(),
              printD6Hash(hs.black_hash).c_str(),
              printD6Hash(h.white_hash).c_str(),
              printD6Hash(hs.white_hash).c_str());
          return false;
        }
      }
    }

    s = p.c_s0();
    op = D6(D6::Action::REFL, 0);
    for (uint32_t _i = 0; _i < 6; _i++) {
      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            d6_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_d6(op, h.black_hash) != hs.black_hash ||
            apply_d6(op, h.white_hash) != hs.white_hash) {
          printf(
              "D6 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD6Hash(h.black_hash).c_str(),
              printD6Hash(hs.black_hash).c_str(),
              printD6Hash(h.white_hash).c_str(),
              printD6Hash(hs.white_hash).c_str());
          return false;
        }
      }

      s = s.c_r1();
      op = D6(D6::Action::ROT, 1) * op;
    }
  }

  // Check D3 table
  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    p -= getCenter();

    const HashEl& h = d3_table_[i];

    HexPos s = p;
    D3 op;
    for (uint32_t _i = 0; _i < 2; _i++) {
      s = s.v_r2();
      op = D3(D3::Action::ROT, 1) * op;

      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            d3_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_d3(op, h.black_hash) != hs.black_hash ||
            apply_d3(op, h.white_hash) != hs.white_hash) {
          printf(
              "D3 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD3Hash(h.black_hash).c_str(),
              printD3Hash(hs.black_hash).c_str(),
              printD3Hash(h.white_hash).c_str(),
              printD3Hash(hs.white_hash).c_str());
          return false;
        }
      }
    }

    s = p.v_s1();
    op = D3(D3::Action::REFL, 0);
    for (uint32_t _i = 0; _i < 3; _i++) {
      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            d3_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_d3(op, h.black_hash) != hs.black_hash ||
            apply_d3(op, h.white_hash) != hs.white_hash) {
          printf(
              "D3 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD3Hash(h.black_hash).c_str(),
              printD3Hash(hs.black_hash).c_str(),
              printD3Hash(h.white_hash).c_str(),
              printD3Hash(hs.white_hash).c_str());
          return false;
        }
      }

      s = s.v_r2();
      op = D3(D3::Action::ROT, 1) * op;
    }
  }
  return true;
}

template <uint32_t NPawns>
std::string GameHash<NPawns>::printD6Hash(game_hash_t h) {
  std::ostringstream ostr;

  ostr << std::hex << "0x" << std::setfill('0') << std::setw(3) << (h & 0x3ff);
  ostr << " 0x" << std::setfill('0') << std::setw(3) << ((h >> 10) & 0x3ff);
  ostr << " 0x" << std::setfill('0') << std::setw(3) << ((h >> 20) & 0x3ff);
  ostr << " 0x" << std::setfill('0') << std::setw(3) << ((h >> 30) & 0x3ff);
  ostr << " 0x" << std::setfill('0') << std::setw(3) << ((h >> 40) & 0x3ff);
  ostr << " 0x" << std::setfill('0') << std::setw(3) << ((h >> 50) & 0x3ff);

  return ostr.str();
}

template <uint32_t NPawns>
std::string GameHash<NPawns>::printD3Hash(game_hash_t h) {
  std::ostringstream ostr;

  ostr << std::hex << "0x" << std::setfill('0') << std::setw(6)
       << (h & 0x1fffff);
  ostr << " 0x" << std::setfill('0') << std::setw(6) << ((h >> 21) & 0x1fffff);
  ostr << " 0x" << std::setfill('0') << std::setw(6) << ((h >> 42) & 0x1fffff);

  return ostr.str();
}

template <uint32_t NPawns>
constexpr uint32_t GameHash<NPawns>::getSymmTableLen() {
  return Game<NPawns>::getBoardLen() * 2 + 1;
}

template <uint32_t NPawns>
constexpr uint32_t GameHash<NPawns>::getSymmTableSize() {
  return getSymmTableLen() * getSymmTableLen();
}

template <uint32_t NPawns>
constexpr HexPos GameHash<NPawns>::getCenter() {
  return { getSymmTableLen() / 2 + getSymmTableLen() / 4,
           getSymmTableLen() / 2 };
}

template <uint32_t NPawns>
constexpr idx_t GameHash<NPawns>::toIdx(uint32_t i) {
  return { i % getSymmTableLen(), i / getSymmTableLen() };
}

template <uint32_t NPawns>
constexpr uint32_t GameHash<NPawns>::fromIdx(idx_t idx) {
  return idx.first + idx.second * getSymmTableLen();
}

template <uint32_t NPawns>
constexpr bool GameHash<NPawns>::inBounds(idx_t idx) {
  return idx.first >= 0 && idx.second >= 0 && idx.first < getSymmTableLen() &&
         idx.second < getSymmTableLen();
}

template <uint32_t NPawns>
constexpr game_hash_t GameHash<NPawns>::apply_d6(D6 op, game_hash_t h) {
  switch (op.ordinal()) {
    case D6(D6::Action::ROT, 0).ordinal(): {
      return h;
    }
    case D6(D6::Action::ROT, 1).ordinal(): {
      return c_r1(h);
    }
    case D6(D6::Action::ROT, 2).ordinal(): {
      return c_r2(h);
    }
    case D6(D6::Action::ROT, 3).ordinal(): {
      return c_r3(h);
    }
    case D6(D6::Action::ROT, 4).ordinal(): {
      return c_r4(h);
    }
    case D6(D6::Action::ROT, 5).ordinal(): {
      return c_r5(h);
    }
    case D6(D6::Action::REFL, 0).ordinal(): {
      return c_s0(h);
    }
    case D6(D6::Action::REFL, 1).ordinal(): {
      return c_s1(h);
    }
    case D6(D6::Action::REFL, 2).ordinal(): {
      return c_s2(h);
    }
    case D6(D6::Action::REFL, 3).ordinal(): {
      return c_s3(h);
    }
    case D6(D6::Action::REFL, 4).ordinal(): {
      return c_s4(h);
    }
    case D6(D6::Action::REFL, 5).ordinal(): {
      return c_s5(h);
    }
    default: {
      __builtin_unreachable();
    }
  }
}

template <uint32_t NPawns>
constexpr game_hash_t GameHash<NPawns>::apply_d3(D3 op, game_hash_t h) {
  switch (op.ordinal()) {
    case D3(D3::Action::ROT, 0).ordinal(): {
      return h;
    }
    case D3(D3::Action::ROT, 1).ordinal(): {
      return v_r2(h);
    }
    case D3(D3::Action::ROT, 2).ordinal(): {
      return v_r4(h);
    }
    case D3(D3::Action::REFL, 0).ordinal(): {
      return v_s1(h);
    }
    case D3(D3::Action::REFL, 1).ordinal(): {
      return v_s3(h);
    }
    case D3(D3::Action::REFL, 2).ordinal(): {
      return v_s5(h);
    }
    default: {
      __builtin_unreachable();
    }
  }
}

template <uint32_t NPawns>
constexpr game_hash_t GameHash<NPawns>::make_invariant_d6(D6 op,
                                                          game_hash_t h) {
  switch (op.ordinal()) {
    case D6(D6::Action::ROT, 0).ordinal(): {
      return h;
    }
    case D6(D6::Action::ROT, 1).ordinal(): {
      return make_c_r1(h);
    }
    case D6(D6::Action::REFL, 0).ordinal(): {
      return make_c_s0(h);
    }
    case D6(D6::Action::REFL, 1).ordinal(): {
      return make_c_s1(h);
    }
    case D6(D6::Action::REFL, 2).ordinal(): {
      return make_c_s2(h);
    }
    case D6(D6::Action::REFL, 3).ordinal(): {
      return make_c_s3(h);
    }
    case D6(D6::Action::REFL, 4).ordinal(): {
      return make_c_s4(h);
    }
    case D6(D6::Action::REFL, 5).ordinal(): {
      return make_c_s5(h);
    }

    // Don't support making invariant under rotations other than the basic
    // rotation.
    case D6(D6::Action::ROT, 2).ordinal():
    case D6(D6::Action::ROT, 3).ordinal():
    case D6(D6::Action::ROT, 4).ordinal():
    case D6(D6::Action::ROT, 5).ordinal():
    default: {
      __builtin_unreachable();
    }
  }
}

template <uint32_t NPawns>
constexpr game_hash_t GameHash<NPawns>::make_invariant_d3(D3 op,
                                                          game_hash_t h) {
  switch (op.ordinal()) {
    case D3(D3::Action::ROT, 0).ordinal(): {
      return h;
    }
    case D3(D3::Action::ROT, 1).ordinal(): {
      return make_v_r2(h);
    }
    case D3(D3::Action::REFL, 0).ordinal(): {
      return make_v_s1(h);
    }
    case D3(D3::Action::REFL, 1).ordinal(): {
      return make_v_s3(h);
    }
    case D3(D3::Action::REFL, 2).ordinal(): {
      return make_v_s5(h);
    }

    // Don't support making invariant under rotations other than the basic
    // rotation.
    case D3(D3::Action::ROT, 2).ordinal():
    default: {
      __builtin_unreachable();
    }
  }
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r1(game_hash_t h) {
  return ((h << 10) | (h >> 50)) & C_MASK;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r2(game_hash_t h) {
  return c_r1(c_r1(h));
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r3(game_hash_t h) {
  return c_r1(c_r2(h));
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r4(game_hash_t h) {
  return c_r1(c_r3(h));
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r5(game_hash_t h) {
  return c_r1(c_r4(h));
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_r2(game_hash_t h) {
  return ((h << 21) | (h >> 42)) & V_MASK;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_r4(game_hash_t h) {
  return v_r2(v_r2(h));
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::e_r3(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_s0(game_hash_t h) {
  uint64_t b14 = h & 0x000000ffc00003ff;
  uint64_t b26 = h & 0x0ffc0000000ffc00;
  uint64_t b35 = h & 0x0003ff003ff00000;

  b26 = (b26 << 40) | (b26 >> 40);
  b35 = ((b35 << 20) | (b35 >> 20)) & 0x0003ff003ff00000;
  return b14 | b26 | b35;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_s1(game_hash_t h) {
  uint64_t b12 = h & 0x00000000000fffff;
  uint64_t b36 = h & 0x0ffc00003ff00000;
  uint64_t b45 = h & 0x0003ffffc0000000;

  b12 = ((b12 << 10) | (b12 >> 10)) & 0x00000000000fffff;
  b36 = (b36 << 30) | (b36 >> 30);
  b45 = ((b45 << 10) | (b45 >> 10)) & 0x0003ffffc0000000;
  return b12 | b36 | b45;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_s2(game_hash_t h) {
  uint64_t b13 = h & 0x000000003ff003ff;
  uint64_t b25 = h & 0x0003ff00000ffc00;
  uint64_t b46 = h & 0x0ffc00ffc0000000;

  b13 = ((b13 << 20) | (b13 >> 20)) & 0x000000003ff003ff;
  b46 = ((b46 << 20) | (b46 >> 20)) & 0x0ffc00ffc0000000;
  return b13 | b25 | b46;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_s3(game_hash_t h) {
  uint64_t b14 = h & 0x000000ffc00003ff;
  uint64_t b23 = h & 0x000000003ffffc00;
  uint64_t b56 = h & 0x0fffff0000000000;

  b14 = ((b14 << 30) | (b14 >> 30)) & 0x000000ffc00003ff;
  b23 = ((b23 << 10) | (b23 >> 10)) & 0x000000003ffffc00;
  b56 = ((b56 << 10) | (b56 >> 10)) & 0x0fffff0000000000;
  return b14 | b23 | b56;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_s4(game_hash_t h) {
  uint64_t b15 = h & 0x0003ff00000003ff;
  uint64_t b24 = h & 0x000000ffc00ffc00;
  uint64_t b36 = h & 0x0ffc00003ff00000;

  b15 = (b15 << 40) | (b15 >> 40);
  b24 = ((b24 << 20) | (b24 >> 20)) & 0x000000ffc00ffc00;
  return b15 | b24 | b36;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_s5(game_hash_t h) {
  uint64_t b16 = h & 0x0ffc0000000003ff;
  uint64_t b25 = h & 0x0003ff00000ffc00;
  uint64_t b34 = h & 0x000000fffff00000;

  b16 = (b16 << 50) | (b16 >> 50);
  b25 = (b25 << 30) | (b25 >> 30);
  b34 = ((b34 << 10) | (b34 >> 10)) & 0x000000fffff00000;
  return b16 | b25 | b34;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_s1(game_hash_t h) {
  uint64_t b1 = h & 0x00000000001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;
  uint64_t b3 = h & 0x7ffffc0000000000;

  b2 = b2 << 21;
  b3 = b3 >> 21;
  return b1 | b2 | b3;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_s3(game_hash_t h) {
  uint64_t b1 = h & 0x00000000001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;
  uint64_t b3 = h & 0x7ffffc0000000000;

  b1 = b1 << 21;
  b2 = b2 >> 21;
  return b1 | b2 | b3;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_s5(game_hash_t h) {
  uint64_t b13 = h & 0x7ffffc00001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;

  b13 = (b13 << 42) | (b13 >> 42);
  return b13 | b2;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::e_s0(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::e_s3(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_r1(game_hash_t h) {
  // Repeat the first 10 bits across the remaining 50 bits, leaving the end
  // zeroed out.
  game_hash_t b = h & UINT64_C(0x3ff);
  b = b | (b << 10);
  return b | (b << 20) | (b << 40);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_v_r2(game_hash_t h) {
  // Repeat the first 21 bits across the remaining 42 bits, leaving the end
  // zeroed out.
  game_hash_t b = h & UINT64_C(0x1fffff);
  return b | (b << 21) | (b << 42);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_e_r3(game_hash_t h) {
  // Repat the first 32 bits across the remaining 30 bits.
  game_hash_t b = h & UINT64_C(0xffffffff);
  return b | (b << 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_s0(game_hash_t h) {
  uint64_t b14 = h & 0x000000ffc00003ff;
  uint64_t b26 = h & 0x00000000000ffc00;
  uint64_t b35 = h & 0x000000003ff00000;

  b26 = b26 | (b26 << 40);
  b35 = b35 | (b35 << 20);
  return b14 | b26 | b35;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_s1(game_hash_t h) {
  uint64_t b12 = h & 0x00000000000003ff;
  uint64_t b36 = h & 0x000000003ff00000;
  uint64_t b45 = h & 0x000000ffc0000000;

  b12 = b12 | (b12 << 10);
  b36 = b36 | (b36 << 30);
  b45 = b45 | (b45 << 10);
  return b12 | b36 | b45;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_s2(game_hash_t h) {
  uint64_t b13 = h & 0x00000000000003ff;
  uint64_t b25 = h & 0x0003ff00000ffc00;
  uint64_t b46 = h & 0x000000ffc0000000;

  b13 = b13 | (b13 << 20);
  b46 = b46 | (b46 << 20);
  return b13 | b25 | b46;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_s3(game_hash_t h) {
  uint64_t b14 = h & 0x00000000000003ff;
  uint64_t b23 = h & 0x00000000000ffc00;
  uint64_t b56 = h & 0x0003ff0000000000;

  b14 = b14 | (b14 << 30);
  b23 = b23 | (b23 << 10);
  b56 = b56 | (b56 << 10);
  return b14 | b23 | b56;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_s4(game_hash_t h) {
  uint64_t b15 = h & 0x00000000000003ff;
  uint64_t b24 = h & 0x00000000000ffc00;
  uint64_t b36 = h & 0x0ffc00003ff00000;

  b15 = b15 | (b15 << 40);
  b24 = b24 | (b24 << 20);
  return b15 | b24 | b36;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_s5(game_hash_t h) {
  uint64_t b16 = h & 0x00000000000003ff;
  uint64_t b25 = h & 0x00000000000ffc00;
  uint64_t b34 = h & 0x000000003ff00000;

  b16 = b16 | (b16 << 50);
  b25 = b25 | (b25 << 30);
  b34 = b34 | (b34 << 10);
  return b16 | b25 | b34;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_v_s1(game_hash_t h) {
  uint64_t b1 = h & 0x00000000001fffff;
  uint64_t b23 = h & 0x000003ffffe00000;

  b23 = b23 | (b23 << 21);
  return b1 | b23;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_v_s3(game_hash_t h) {
  uint64_t b12 = h & 0x00000000001fffff;
  uint64_t b3 = h & 0x7ffffc0000000000;

  b12 = b12 | (b12 << 21);
  return b12 | b3;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_v_s5(game_hash_t h) {
  uint64_t b13 = h & 0x00000000001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;

  b13 = b13 | (b13 << 42);
  return b13 | b2;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_e_s0(game_hash_t h) {
  uint64_t b12 = h & 0x00000000ffffffff;

  return b12 | (b12 << 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_e_s3(game_hash_t h) {
  uint64_t b12 = h & 0x00000000ffffffff;

  return b12 | (b12 << 32);
}

template <uint32_t NPawns>
void GameHash<NPawns>::initSymmTables() {
  seed_rand(0, 0);

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    // Returns true if the symmetry position symm of p_idx (given as an
    // absolute index) has already been calculated (meaning we should derive
    // our hashes from it).
    std::function<bool(uint32_t, HexPos)> shouldReuse = [](uint32_t p_idx,
                                                           HexPos symm) {
      idx_t idx = Game<NPawns>::posToIdx(symm + getCenter());
      uint32_t i = fromIdx(idx);
      return i < p_idx && inBounds(idx);
    };

    // Initialize D6 table
    game_hash_t d6b = gen_rand64() & C_MASK;
    game_hash_t d6w = gen_rand64() & C_MASK;

    if (p == HexPos::origin()) {
      d6_table_[i] = { make_c_s0(make_c_r1(d6b)), make_c_s0(make_c_r1(d6w)) };
    } else {
      // Try the other 5 rotational symmetries
      HexPos s = p;
      D6 op;
      for (uint32_t _i = 0; _i < 5; _i++) {
        s = s.c_r1();
        // Accumulate the inverses of the operations we have been doing.
        op = op * D6(D6::Action::ROT, 5);

        if (shouldReuse(i, s)) {
          const HashEl& el =
              d6_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
          d6_table_[i] = { apply_d6(op, el.black_hash),
                           apply_d6(op, el.white_hash) };
          goto d6_hash_calc_done;
        }
      }

      // Try the 6 reflected symmetries
      s = p.c_s0();
      op = D6(D6::Action::REFL, 0);
      for (uint32_t _i = 0; _i < 6; _i++) {
        if (s == p) {
          // This tile is symmetric to itself under some reflection
          d6_table_[i] = { make_invariant_d6(op, d6b),
                           make_invariant_d6(op, d6w) };
          goto d6_hash_calc_done;
        }

        if (shouldReuse(i, s)) {
          const HashEl& el =
              d6_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
          d6_table_[i] = { apply_d6(op, el.black_hash),
                           apply_d6(op, el.white_hash) };
          goto d6_hash_calc_done;
        }

        s = s.c_r1();
        // Accumulate the inverses of the operations we have been doing.
        op = op * D6(D6::Action::ROT, 5);
      }

      // Otherwise, create a new hash value
      d6_table_[i] = { d6b, d6w };

d6_hash_calc_done:;
    }

    game_hash_t d3b = gen_rand64() & V_MASK;
    game_hash_t d3w = gen_rand64() & V_MASK;

    {
      // Try the 2 rotational symmetries
      HexPos s = p;
      D3 op;
      for (uint32_t _i = 0; _i < 2; _i++) {
        s = s.v_r2();
        op = op * D3(D3::Action::ROT, 2);

        if (shouldReuse(i, s)) {
          const HashEl& el =
              d3_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
          d3_table_[i] = { apply_d3(op, el.black_hash),
                           apply_d3(op, el.white_hash) };
          goto d3_hash_calc_done;
        }
      }

      // Try the 3 reflected symmetries
      s = p.v_s1();
      op = D3(D3::Action::REFL, 0);
      for (uint32_t _i = 0; _i < 3; _i++) {
        if (s == p) {
          // This tile is symmetric to itself under some reflection
          d3_table_[i] = { make_invariant_d3(op, d3b),
                           make_invariant_d3(op, d3w) };
          goto d3_hash_calc_done;
        }

        if (shouldReuse(i, s)) {
          const HashEl& el =
              d3_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
          d3_table_[i] = { apply_d3(op, el.black_hash),
                           apply_d3(op, el.white_hash) };
          goto d3_hash_calc_done;
        }

        s = s.v_r2();
        op = op * D3(D3::Action::ROT, 2);
      }

      // Otherwise, create a new hash value
      d3_table_[i] = { d3b, d3w };

d3_hash_calc_done:;
    }

    /*
    game_hash_t c2c2b = gen_rand64();
    game_hash_t c2c2w = gen_rand64();
    game_hash_t c2cvb = gen_rand64();
    game_hash_t c2cvw = gen_rand64();
    game_hash_t c2ceb = gen_rand64();
    game_hash_t c2cew = gen_rand64();
    game_hash_t c2evb = gen_rand64();
    game_hash_t c2evw = gen_rand64();
    game_hash_t trb = gen_rand64();
    game_hash_t trw = gen_rand64();
    */
  }
}
}  // namespace Onoro
