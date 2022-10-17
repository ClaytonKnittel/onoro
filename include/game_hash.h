#pragma once

#include <utils/math/group/cyclic.h>
#include <utils/math/group/dihedral.h>
#include <utils/math/group/direct_product.h>
#include <utils/math/random.h>

#include <iomanip>

#include "game.h"
#include "game_view.h"
#include "hash_group.h"
#include "hex_pos.h"

namespace onoro {

using namespace hash_group;

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

  game_hash_t operator()(const GameView<NPawns>& view) const noexcept;

  static constexpr game_hash_t calcHash(const Game<NPawns>& game) noexcept;

  bool validate() const;

  static std::string printD6Hash(game_hash_t);
  static std::string printD3Hash(game_hash_t);
  static std::string printK4Hash(game_hash_t);
  static std::string printC2Hash(game_hash_t);

 private:
  struct HashEl {
    // hash to use for black pawn in this tile.
    game_hash_t black_hash_;

    constexpr game_hash_t black_hash() const {
      return black_hash_;
    }

    constexpr game_hash_t white_hash() const {
      return color_swap(black_hash_);
    }
  };

  // Returns the length of the symm tables in one dimension.
  static constexpr uint32_t getSymmTableLen();

  // Returns the total number of tiles in each symm table.
  static constexpr uint32_t getSymmTableSize();

  using SymmTable = std::array<HashEl, getSymmTableSize()>;

  // Returns the tile designated as the origin tile for this board.
  static constexpr HexPos getCenter();

  // Translates from an absolute index to an index object.
  static constexpr idx_t toIdx(uint32_t i);

  static constexpr uint32_t fromIdx(idx_t idx);

  static constexpr bool inBounds(idx_t idx);

  /*
   * Returns the hash table associated with the given symmetry class.
   */
  static constexpr const SymmTable& getHashTable(SymmetryClass symm_class);

  static constexpr HashEl hashLookup(const SymmTable& table, HexPos pos);

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

  /*
   * Returns true if the symmetry position symm of p_idx (given as an
   * absolute index) has already been calculated (meaning we should derive
   * our hashes from it).
   */
  static constexpr bool symmTableShouldReuseTile(uint32_t p_idx, HexPos symm);

  static constexpr SymmTable initD6Table();
  static constexpr SymmTable initD3Table();
  static constexpr SymmTable initK4Table();
  static constexpr SymmTable initC2CVTable();
  static constexpr SymmTable initC2CETable();
  static constexpr SymmTable initC2EVTable();
  static constexpr SymmTable initTrivialTable();

  static constexpr SymmTable d6_table_ = initD6Table();
  static constexpr SymmTable d3_table_ = initD3Table();
  static constexpr SymmTable k4_table_ = initK4Table();
  static constexpr SymmTable c2_cv_table_ = initC2CVTable();
  static constexpr SymmTable c2_ce_table_ = initC2CETable();
  static constexpr SymmTable c2_ev_table_ = initC2EVTable();
  static constexpr SymmTable trivial_table_ = initTrivialTable();
};

template <uint32_t NPawns>
GameHash<NPawns>::GameHash() {}

template <uint32_t NPawns>
game_hash_t GameHash<NPawns>::operator()(
    const GameView<NPawns>& view) const noexcept {
  // TODO remove when not debugging
  assert(view.op<D6>().ordinal() == 0);
  return calcHash(view.game());
}

template <uint32_t NPawns>
constexpr game_hash_t GameHash<NPawns>::calcHash(
    const Game<NPawns>& game) noexcept {
  typename Game<NPawns>::BoardSymmetryState symm_state =
      game.calcSymmetryState();
  HexPos origin = game.originTile(symm_state);

  /*
  printf("Origin: (%d, %d) (%u)\n", origin.x, origin.y, game.nPawnsInPlay());
  printf("%s\n", game.Print().c_str());
  printf("%s\n", game.Print2().c_str());

  const char* states[7] = {
    "C", "V", "E", "CV", "CE", "EV", "trivial",
  };
  printf("symm state: %s\n",
         states[static_cast<uint32_t>(symm_state.symm_class)]);
  printf("op: %s\n", symm_state.op.toString().c_str());
  printf("trans: (%d, %d)\n", symm_state.center_offset.x,
         symm_state.center_offset.y);
  */

  const SymmTable& hash_table = getHashTable(symm_state.symm_class);

  game_hash_t h = 0;
  game.forEachPawn([&game, &h, symm_state, origin, hash_table](idx_t pawn_idx) {
    HexPos pawn_pos = Game<NPawns>::idxToPos(pawn_idx);

    // printf("Trans (%d, %d) -> ", (pawn_pos - origin).x, (pawn_pos -
    // origin).y);

    // transform pawn_pos according to symm_state.op
    pawn_pos = (pawn_pos - origin).apply_d6_c(symm_state.op) + getCenter();
    HashEl hash_el = hashLookup(hash_table, pawn_pos);

    /*
    printf("(%d, %d)\n", (pawn_pos - getCenter()).x,
           (pawn_pos - getCenter()).y);
    printf("hash: %s\n", printD3Hash(hash_el.black_hash()).c_str());
    */

    if (game.getTile(pawn_idx) == Game<NPawns>::TileState::TILE_BLACK) {
      h = h ^ hash_el.black_hash();
    } else {
      h = h ^ hash_el.white_hash();
    }
    return true;
  });

  return h;
}

template <uint32_t NPawns>
bool GameHash<NPawns>::validate() const {
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
        if (apply_d6(op, h.black_hash()) != hs.black_hash() ||
            apply_d6(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "D6 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD6Hash(h.black_hash()).c_str(),
              printD6Hash(hs.black_hash()).c_str(),
              printD6Hash(h.white_hash()).c_str(),
              printD6Hash(hs.white_hash()).c_str());
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
        if (apply_d6(op, h.black_hash()) != hs.black_hash() ||
            apply_d6(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "D6 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD6Hash(h.black_hash()).c_str(),
              printD6Hash(hs.black_hash()).c_str(),
              printD6Hash(h.white_hash()).c_str(),
              printD6Hash(hs.white_hash()).c_str());
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
        if (apply_d3(op, h.black_hash()) != hs.black_hash() ||
            apply_d3(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "D3 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD3Hash(h.black_hash()).c_str(),
              printD3Hash(hs.black_hash()).c_str(),
              printD3Hash(h.white_hash()).c_str(),
              printD3Hash(hs.white_hash()).c_str());
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
        if (apply_d3(op, h.black_hash()) != hs.black_hash() ||
            apply_d3(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "D3 hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printD3Hash(h.black_hash()).c_str(),
              printD3Hash(hs.black_hash()).c_str(),
              printD3Hash(h.white_hash()).c_str(),
              printD3Hash(hs.white_hash()).c_str());
          return false;
        }
      }

      s = s.v_r2();
      op = D3(D3::Action::ROT, 1) * op;
    }

    // Check K4 table
    for (uint32_t i = 0; i < getSymmTableSize(); i++) {
      HexPos p = Game<NPawns>::idxToPos(toIdx(i));

      p -= getCenter();

      const HashEl& h = k4_table_[i];

      for (K4 op : { K4(C2(1), C2(0)), K4(C2(0), C2(1)), K4(C2(1), C2(1)) }) {
        HexPos s = p.apply_k4_e(op);

        idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
        if (inBounds(idx)) {
          const HashEl& hs =
              k4_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
          if (apply_k4(op, h.black_hash()) != hs.black_hash() ||
              apply_k4(op, h.white_hash()) != hs.white_hash()) {
            printf(
                "K4 hashes not equal between position (%d, %d) and (%d, %d) "
                "under "
                "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
                p.x, p.y, s.x, s.y, op.toString().c_str(),
                printK4Hash(h.black_hash()).c_str(),
                printK4Hash(hs.black_hash()).c_str(),
                printK4Hash(h.white_hash()).c_str(),
                printK4Hash(hs.white_hash()).c_str());
            return false;
          }
        }
      }
    }

    // Check C2_cv table
    for (uint32_t i = 0; i < getSymmTableSize(); i++) {
      HexPos p = Game<NPawns>::idxToPos(toIdx(i));

      p -= getCenter();

      const HashEl& h = c2_cv_table_[i];

      C2 op = C2(1);
      HexPos s = p.apply_c2_cv(op);

      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            c2_cv_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_c2(op, h.black_hash()) != hs.black_hash() ||
            apply_c2(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "C2 cv hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printC2Hash(h.black_hash()).c_str(),
              printC2Hash(hs.black_hash()).c_str(),
              printC2Hash(h.white_hash()).c_str(),
              printC2Hash(hs.white_hash()).c_str());
          return false;
        }
      }
    }

    // Check C2_ce table
    for (uint32_t i = 0; i < getSymmTableSize(); i++) {
      HexPos p = Game<NPawns>::idxToPos(toIdx(i));

      p -= getCenter();

      const HashEl& h = c2_ce_table_[i];

      C2 op = C2(1);
      HexPos s = p.apply_c2_ce(op);

      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            c2_ce_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_c2(op, h.black_hash()) != hs.black_hash() ||
            apply_c2(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "C2 ce hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printC2Hash(h.black_hash()).c_str(),
              printC2Hash(hs.black_hash()).c_str(),
              printC2Hash(h.white_hash()).c_str(),
              printC2Hash(hs.white_hash()).c_str());
          return false;
        }
      }
    }

    // Check C2_ev table
    for (uint32_t i = 0; i < getSymmTableSize(); i++) {
      HexPos p = Game<NPawns>::idxToPos(toIdx(i));

      p -= getCenter();

      const HashEl& h = c2_ev_table_[i];

      C2 op = C2(1);
      HexPos s = p.apply_c2_ev(op);

      idx_t idx = Game<NPawns>::posToIdx(s + getCenter());
      if (inBounds(idx)) {
        const HashEl& hs =
            c2_ev_table_[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        if (apply_c2(op, h.black_hash()) != hs.black_hash() ||
            apply_c2(op, h.white_hash()) != hs.white_hash()) {
          printf(
              "C2 ev hashes not equal between position (%d, %d) and (%d, %d) "
              "under "
              "%s:\nblack:\n\t%s\n\t%s\nwhite\n\t%s\n\t%s\n",
              p.x, p.y, s.x, s.y, op.toString().c_str(),
              printC2Hash(h.black_hash()).c_str(),
              printC2Hash(hs.black_hash()).c_str(),
              printC2Hash(h.white_hash()).c_str(),
              printC2Hash(hs.white_hash()).c_str());
          return false;
        }
      }
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

  ostr << std::hex << "0x" << std::setfill('0') << std::setw(5)
       << (h & 0xfffff);
  ostr << " 0x" << std::setfill('0') << std::setw(5) << ((h >> 20) & 0xfffff);
  ostr << " 0x" << std::setfill('0') << std::setw(5) << ((h >> 40) & 0xfffff);

  return ostr.str();
}

template <uint32_t NPawns>
std::string GameHash<NPawns>::printK4Hash(game_hash_t h) {
  std::ostringstream ostr;

  ostr << std::hex << "0x" << std::setfill('0') << std::setw(4) << (h & 0xffff);
  ostr << " 0x" << std::setfill('0') << std::setw(4) << ((h >> 16) & 0xffff);
  ostr << " 0x" << std::setfill('0') << std::setw(4) << ((h >> 32) & 0xffff);
  ostr << " 0x" << std::setfill('0') << std::setw(4) << ((h >> 48) & 0xffff);

  return ostr.str();
}

template <uint32_t NPawns>
std::string GameHash<NPawns>::printC2Hash(game_hash_t h) {
  std::ostringstream ostr;

  ostr << std::hex << "0x" << std::setfill('0') << std::setw(8)
       << (h & 0xffffffff);
  ostr << " 0x" << std::setfill('0') << std::setw(8)
       << ((h >> 32) & 0xffffffff);

  return ostr.str();
}

template <uint32_t NPawns>
constexpr uint32_t GameHash<NPawns>::getSymmTableLen() {
  return 2 * NPawns + 1;
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
  return idx.first >= 0 && idx.second >= 0 &&
         idx.first < static_cast<int32_t>(getSymmTableLen()) &&
         idx.second < static_cast<int32_t>(getSymmTableLen());
}

template <uint32_t NPawns>
constexpr const typename GameHash<NPawns>::SymmTable&
GameHash<NPawns>::getHashTable(SymmetryClass symm_class) {
  switch (symm_class) {
    case SymmetryClass::C: {
      return d6_table_;
    }
    case SymmetryClass::V: {
      return d3_table_;
    }
    case SymmetryClass::E: {
      return k4_table_;
    }
    case SymmetryClass::CV: {
      return c2_cv_table_;
    }
    case SymmetryClass::CE: {
      return c2_ce_table_;
    }
    case SymmetryClass::EV: {
      return c2_ev_table_;
    }
    case SymmetryClass::TRIVIAL: {
      return trivial_table_;
    }
    default: {
      __builtin_unreachable();
    }
  }
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::HashEl GameHash<NPawns>::hashLookup(
    const SymmTable& table, HexPos pos) {
  return table[fromIdx(Game<NPawns>::posToIdx(pos))];
}

template <uint32_t NPawns>
constexpr bool GameHash<NPawns>::symmTableShouldReuseTile(uint32_t p_idx,
                                                          HexPos symm) {
  idx_t idx = Game<NPawns>::posToIdx(symm + getCenter());
  uint32_t i = fromIdx(idx);
  return i < p_idx && inBounds(idx);
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable GameHash<NPawns>::initD6Table() {
  util::Random random(1, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    bool found_hash = false;
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    game_hash_t d6b = random.rand64() & C_MASK;

    HexPos s = p;
    D6 op;

    if (p == HexPos::origin()) {
      table[i] = (HashEl){ make_d6_s0(make_d6_r1(d6b)) };
      continue;
    }

    // Try the other 5 rotational symmetries
    for (uint32_t _i = 0; _i < 5; _i++) {
      s = s.c_r1();
      // Accumulate the inverses of the operations we have been doing.
      op = op * D6(D6::Action::ROT, 5);

      if (symmTableShouldReuseTile(i, s)) {
        const HashEl& el =
            table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        table[i] = { apply_d6(op, el.black_hash()) };
        found_hash = true;
        break;
      }
    }

    if (found_hash) {
      continue;
    }

    // Try the 6 reflected symmetries
    s = p.c_s0();
    op = D6(D6::Action::REFL, 0);
    for (uint32_t _i = 0; _i < 6; _i++) {
      if (s == p) {
        // This tile is symmetric to itself under some reflection
        table[i] = { make_invariant_d6(op, d6b) };
        found_hash = true;
        break;
      }

      if (symmTableShouldReuseTile(i, s)) {
        const HashEl& el =
            table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        table[i] = { apply_d6(op, el.black_hash()) };
        found_hash = true;
        break;
      }

      s = s.c_r1();
      // Accumulate the inverses of the operations we have been doing.
      op = op * D6(D6::Action::ROT, 5);
    }

    // Otherwise, create a new hash value
    if (!found_hash) {
      table[i] = { d6b };
    }
  }

  return table;
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable GameHash<NPawns>::initD3Table() {
  util::Random random(3, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    bool found_hash = false;
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    game_hash_t d3b = random.rand64() & V_MASK;

    // Try the 2 rotational symmetries
    HexPos s = p;
    D3 op;
    for (uint32_t _i = 0; _i < 2; _i++) {
      s = s.v_r2();
      op = op * D3(D3::Action::ROT, 2);

      if (symmTableShouldReuseTile(i, s)) {
        const HashEl& el =
            table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        table[i] = { apply_d3(op, el.black_hash()) };
        found_hash = true;
        break;
      }
    }

    if (found_hash) {
      continue;
    }

    // Try the 3 reflected symmetries
    s = p.v_s1();
    op = D3(D3::Action::REFL, 0);
    for (uint32_t _i = 0; _i < 3; _i++) {
      if (s == p) {
        // This tile is symmetric to itself under some reflection
        table[i] = { make_invariant_d3(op, d3b) };
        found_hash = true;
        break;
      }

      if (symmTableShouldReuseTile(i, s)) {
        const HashEl& el =
            table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        table[i] = { apply_d3(op, el.black_hash()) };
        found_hash = true;
        break;
      }

      s = s.v_r2();
      op = op * D3(D3::Action::ROT, 2);
    }

    // Otherwise, create a new hash value
    if (!found_hash) {
      table[i] = { d3b };
    }
  }

  return table;
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable GameHash<NPawns>::initK4Table() {
  util::Random random(5, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    bool found_hash = false;
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    game_hash_t k4b = random.rand64();

    // Check the 3 symmetries for existing hashes.
    for (K4 op : { K4(C2(1), C2(0)), K4(C2(0), C2(1)), K4(C2(1), C2(1)) }) {
      HexPos s = p.apply_k4_e(op);

      if (symmTableShouldReuseTile(i, s)) {
        const HashEl& el =
            table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
        table[i] = { apply_k4(op, el.black_hash()) };
        found_hash = true;
        break;
      }
    }

    if (found_hash) {
      continue;
    }

    // Check the symmetries for self-mapping.
    for (K4 op : { K4(C2(1), C2(1)), K4(C2(1), C2(0)), K4(C2(0), C2(1)) }) {
      HexPos s = p.apply_k4_e(op);

      if (s == p) {
        table[i] = { make_invariant_k4(op, k4b) };
        found_hash = true;
        break;
      }
    }

    // Otherwise, create a new hash value
    if (!found_hash) {
      table[i] = { k4b };
    }
  }

  return table;
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable
GameHash<NPawns>::initC2CVTable() {
  util::Random random(7, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    game_hash_t c2cvb = random.rand64();

    // check the symmetry for existing hashes/self-mapping
    HexPos s = p.apply_c2_cv(C2(1));
    if (s == p) {
      table[i] = { make_invariant_c2(C2(1), c2cvb) };
    } else if (symmTableShouldReuseTile(i, s)) {
      const HashEl& el =
          table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
      table[i] = { apply_c2(C2(1), el.black_hash()) };
    } else {
      table[i] = { c2cvb };
    }
  }

  return table;
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable
GameHash<NPawns>::initC2CETable() {
  util::Random random(13, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    game_hash_t c2ceb = random.rand64();

    // check the symmetry for existing hashes/self-mapping
    HexPos s = p.apply_c2_ce(C2(1));
    if (s == p) {
      table[i] = { make_invariant_c2(C2(1), c2ceb) };
    } else if (symmTableShouldReuseTile(i, s)) {
      const HashEl& el =
          table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
      table[i] = { apply_c2(C2(1), el.black_hash()) };
    } else {
      table[i] = { c2ceb };
    }
  }

  return table;
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable
GameHash<NPawns>::initC2EVTable() {
  util::Random random(17, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    HexPos p = Game<NPawns>::idxToPos(toIdx(i));

    // Normalize coordinates to the center.
    p -= getCenter();

    game_hash_t c2evb = random.rand64();

    // check the symmetry for existing hashes/self-mapping
    HexPos s = p.apply_c2_ev(C2(1));
    if (s == p) {
      table[i] = { make_invariant_c2(C2(1), c2evb) };
    } else if (symmTableShouldReuseTile(i, s)) {
      const HashEl& el =
          table[fromIdx(Game<NPawns>::posToIdx(s + getCenter()))];
      table[i] = { apply_c2(C2(1), el.black_hash()) };
    } else {
      table[i] = { c2evb };
    }
  }

  return table;
}

template <uint32_t NPawns>
constexpr typename GameHash<NPawns>::SymmTable
GameHash<NPawns>::initTrivialTable() {
  util::Random random(23, 0);

  SymmTable table{ { { 0 } } };

  for (uint32_t i = 0; i < getSymmTableSize(); i++) {
    game_hash_t trb = random.rand64();
    table[i] = { trb };
  }

  return table;
}

}  // namespace onoro
