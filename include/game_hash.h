#pragma once

#include "hex_pos.h"
#include "random.h"

namespace Onoro {

typedef std::size_t game_hash_t;

template <uint32_t NPawns>
class Game;

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

  // Returns the origin tile for the D6 group.
  static constexpr HexPos getCenterD6();

  /*
   * Rotates the hash 60, 120, and 180 degrees (R1, R2, R3).
   *
   * Note: these algorithms are incompatible with each other, i.e.
   * c_R1(rot60(x)) != v_R2(x).
   */
  static game_hash_t c_R1(game_hash_t h);
  static game_hash_t v_R2(game_hash_t h);
  static game_hash_t e_R3(game_hash_t h);

  /*
   * [cve]_r<n>: Reflects the hash across a line at angle n*30 degrees, passing
   * through:
   *  - c: the center of the origin hex
   *  - v: the top right vertex of the origin hex
   *  - e: the center of the right edge of the origin hex
   */
  static game_hash_t c_r0(game_hash_t h);
  static game_hash_t c_r1(game_hash_t h);
  static game_hash_t c_r2(game_hash_t h);
  static game_hash_t c_r3(game_hash_t h);
  static game_hash_t c_r4(game_hash_t h);
  static game_hash_t c_r5(game_hash_t h);
  static game_hash_t v_r1(game_hash_t h);
  static game_hash_t v_r3(game_hash_t h);
  static game_hash_t v_r5(game_hash_t h);
  static game_hash_t e_r0(game_hash_t h);
  static game_hash_t e_r3(game_hash_t h);

  /*
   * Given a hash, compresses it to make it symmetric via rotation of 60, 120,
   * or 180 degrees (R1, R2, or R3).
   */
  static game_hash_t make_c_R1(game_hash_t h);
  static game_hash_t make_v_R2(game_hash_t h);
  static game_hash_t make_e_R3(game_hash_t h);

  /*
   * Given a hash, compress it to make it symmetric via reflection across the
   * x-axis (r0).
   */
  static game_hash_t make_refl(game_hash_t h);

  void initSymmTables();

  /*
   * The infinite hexagonal plane centered at a fixed point forms a dihedral
   * group D6, which has group operations R1 (Rn = rotate by n*60 degrees about
   * the fixed point) and r0 (rn = reflect about a line at angle n*pi/6 through
   * the fixed point).
   *
   * We are interested in seven subgroups of this group, for seven cases that
   * the game board can be in, categorized by where the "center of mass" of the
   * board lies:
   *  - D6: (R[1-5] r[0-5]) the center of mass lies exactly in the center of a
   *      hexagonal cell
   *  - D3: (R2 R4 r1 r3 r5) the center of mass lies on a vertex of a hexagonal
   *      cell.
   *  - C2 + C2: (R3 r0 r3) the center of mass lies on the midpoint of an edge
   *      of a hexagonal cell.
   *  - C2: (r1) the center of mass lies along a line extending from the middle
   *      of a hexagonal cell to one of its vertices.
   *  - C2: (r0) the center of mass lies along a line extending from the middle
   *      of a hexagonal cell to the center of one of its edges.
   *  - C2: (r4 + translation) the center of mass lies on an edge of a hexagonal
   *      cell.
   *  - trivial: all other cases
   *
   * By choosing a preferred orientation of the center of mass (i.e. let the
   * tile containing the center of mass be (0, 0), and rotate/reflect the plane
   * about the new origin until the center of mass lies in the triangle formed
   * by the origin (center of the hexagon at (0, 0)), the vertex in the +x
   * direction of the origin tile, and the midpoint of the edge extending in the
   * +y direction from this vertex), we can ensure that all symmetries of the
   * board will be reachable under the operations in the corresponding
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
constexpr uint32_t GameHash<NPawns>::getSymmTableLen() {
  return Game<NPawns>::getBoardLen() * 2 + 1;
}

template <uint32_t NPawns>
constexpr uint32_t GameHash<NPawns>::getSymmTableSize() {
  return getSymmTableLen() * getSymmTableLen();
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_R1(game_hash_t h) {
  return ((h << 10) | (h >> 50)) & UINT64_C(0x0fffffffffffffff);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_R2(game_hash_t h) {
  return ((h << 21) | (h >> 42)) & UINT64_C(0x7fffffffffffffff);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::e_R3(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r0(game_hash_t h) {
  uint64_t b14 = h & 0x000000ffc00003ff;
  uint64_t b26 = h & 0x0ffc0000000ffc00;
  uint64_t b35 = h & 0x0003ff003ff00000;

  b26 = (b26 << 40) | (b26 >> 40);
  b35 = ((b35 << 20) | (b35 >> 20)) & 0x0003ff003ff00000;
  return b14 | b26 | b35;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r1(game_hash_t h) {
  uint64_t b12 = h & 0x00000000000fffff;
  uint64_t b36 = h & 0x0ffc00003ff00000;
  uint64_t b45 = h & 0x0003ffffc0000000;

  b12 = ((b12 << 10) | (b12 >> 10)) & 0x00000000000fffff;
  b36 = (b36 << 30) | (b36 >> 30);
  b45 = ((b45 << 10) | (b45 >> 10)) & 0x0003ffffc0000000;
  return b12 | b36 | b45;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r2(game_hash_t h) {
  uint64_t b13 = h & 0x000000003ff003ff;
  uint64_t b25 = h & 0x0003ff00000ffc00;
  uint64_t b46 = h & 0x0ffc00ffc0000000;

  b13 = ((b13 << 20) | (b13 >> 20)) & 0x000000003ff003ff;
  b46 = ((b46 << 20) | (b46 >> 20)) & 0x0ffc00ffc0000000;
  return b13 | b25 | b46;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r3(game_hash_t h) {
  uint64_t b14 = h & 0x000000ffc00003ff;
  uint64_t b23 = h & 0x000000003ffffc00;
  uint64_t b56 = h & 0x0fffff0000000000;

  b14 = ((b14 << 30) | (b14 >> 30)) & 0x000000ffc00003ff;
  b23 = ((b23 << 10) | (b23 >> 10)) & 0x000000003ffffc00;
  b56 = ((b56 << 10) | (b56 >> 10)) & 0x0fffff0000000000;
  return b14 | b23 | b56;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r4(game_hash_t h) {
  uint64_t b15 = h & 0x0003ff00000003ff;
  uint64_t b24 = h & 0x000000ffc00ffc00;
  uint64_t b36 = h & 0x0ffc00003ff00000;

  b15 = (b15 << 40) | (b15 >> 40);
  b24 = ((b24 << 20) | (b24 >> 20)) & 0x000000ffc00ffc00;
  return b15 | b24 | b36;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::c_r5(game_hash_t h) {
  uint64_t b16 = h & 0x0ffc0000000003ff;
  uint64_t b25 = h & 0x0003ff00000ffc00;
  uint64_t b34 = h & 0x000000fffff00000;

  b16 = (b16 << 50) | (b16 >> 50);
  b25 = (b25 << 30) | (b25 >> 30);
  b34 = ((b34 << 10) | (b34 >> 10)) & 0x000000fffff00000;
  return b16 | b25 | b34;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_r1(game_hash_t h) {
  uint64_t b1 = h & 0x00000000001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;
  uint64_t b3 = h & 0x7ffffc0000000000;

  b2 = b2 << 21;
  b3 = b3 >> 21;
  return b1 | b2 | b3;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_r3(game_hash_t h) {
  uint64_t b1 = h & 0x00000000001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;
  uint64_t b3 = h & 0x7ffffc0000000000;

  b1 = b1 << 21;
  b2 = b2 >> 21;
  return b1 | b2 | b3;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::v_r5(game_hash_t h) {
  uint64_t b13 = h & 0x7ffffc00001fffff;
  uint64_t b2 = h & 0x000003ffffe00000;

  b13 = (b13 << 42) | (b13 >> 42);
  return b13 | b2;
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::e_r0(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::e_r3(game_hash_t h) {
  return (h << 32) | (h >> 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_c_R1(game_hash_t h) {
  // Repeat the first 10 bits across the remaining 50 bits, leaving the end
  // zeroed out.
  game_hash_t b = h & UINT64_C(0x3ff);
  b = b | (b << 10);
  return b | (b << 20) | (b << 40);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_v_R2(game_hash_t h) {
  // Repeat the first 21 bits across the remaining 42 bits, leaving the end
  // zeroed out.
  game_hash_t b = h & UINT64_C(0x1fffff);
  return b | (b << 21) | (b << 42);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_e_R3(game_hash_t h) {
  // Repat the first 32 bits across the remaining 30 bits.
  game_hash_t b = h & UINT64_C(0xffffffff);
  return b | (b << 32);
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::make_refl(game_hash_t h) {
  return 0;
}

template <uint32_t NPawns>
void GameHash<NPawns>::initSymmTables() {
  seed_rand(0, 0);

  // Initialize D6 table
  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    uint32_t x = i % getSymmTableLen();
    uint32_t y = i / getSymmTableLen();

    // coordinates of (0, 0) in this coordinate space
    constexpr uint32_t x_origin = getSymmTableLen() / 2;
    constexpr uint32_t y_origin = getSymmTableLen() / 2;

    if (x == x_origin && y == y_origin) {
    }
  }
}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::operator()(const Game<NPawns>& g) const noexcept {
  return 0;
}
}  // namespace Onoro
