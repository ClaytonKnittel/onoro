#pragma once

#include "game.h"
#include "game_view.h"

namespace onoro {

template <uint32_t NPawns>
class GameEq {
 public:
  using is_transparent = void;

  GameEq() = default;

  bool operator()(const GameView<NPawns>& view1,
                  const GameView<NPawns>& view2) const noexcept;

 private:
  template <class SymmetryClassOp>
  static bool compareViews(const GameView<NPawns>& view1,
                           const GameView<NPawns>& view2,
                           typename Game<NPawns>::BoardSymmetryState s1,
                           typename Game<NPawns>::BoardSymmetryState s2);
};

template <uint32_t NPawns>
bool GameEq<NPawns>::operator()(const GameView<NPawns>& view1,
                                const GameView<NPawns>& view2) const noexcept {
  using SymmState = typename Game<NPawns>::BoardSymmetryState;

  SymmState s1 = view1.game().calcSymmetryState();
  SymmState s2 = view2.game().calcSymmetryState();

  if (s1.symm_class != s2.symm_class) {
    return false;
  }

  SymmetryClassOpApplyAndReturn(s1.symm_class, compareViews, view1, view2, s1,
                                s2);
}

template <uint32_t NPawns>
template <class SymmetryClassOp>
bool GameEq<NPawns>::compareViews(
    const GameView<NPawns>& view1, const GameView<NPawns>& view2,
    typename Game<NPawns>::BoardSymmetryState s1,
    typename Game<NPawns>::BoardSymmetryState s2) {
  typedef typename SymmetryClassOp::Group Group;

  const Game<NPawns>& g1 = view1.game();
  const Game<NPawns>& g2 = view2.game();

  if (s1.symm_class != s2.symm_class) {
    // printf("DIFF SYMM CLASSES!\n");
    return false;
  }

  if (g1.nPawnsInPlay() != g2.nPawnsInPlay()) {
    // printf("DIF N PAWNS IN PLAY!!!!! %u vs %u\n", g1.nPawnsInPlay(),
    //        g2.nPawnsInPlay());
    return false;
  }

  Group view_op1 = view1.template op<Group>();
  Group view_op2 = view2.template op<Group>();

  // Group operation to translate a point in view 2 to a point in view 1.
  Group to_view1 = view_op1 * view_op2.inverse();

  HexPos origin1 = g1.originTile(s1);
  HexPos origin2 = g2.originTile(s2);

  // Canonicalizing/decanonicalizing group ops to apply to points before
  // transforming them according to the symmetry matching op.
  D6 canon1 = s1.op;
  D6 decanon2 = s2.op.inverse();

  return g1.forEachPawn(
      [&g2, to_view1, origin1, origin2, canon1, decanon2](idx_t idx) {
        HexPos p2 = (Game<NPawns>::idxToPos(idx) - origin1).apply_d6_c(canon1);
        // printf("p2 from (%d, %d) to ", p2.x, p2.y);
        p2 = SymmetryClassOp::apply_fn(p2, to_view1);
        // printf("(%d, %d) after applying %s\n", p2.x, p2.y,
        //        to_view1.toString().c_str());
        idx_t idx2 = Game<NPawns>::posToIdx(p2.apply_d6_c(decanon2) + origin2);

        // printf("Pos (%d, %d) translated to (%d, %d)\n", idx.x(), idx.y(),
        // idx2.x(),
        //        idx2.y());

        if (g2.getTile(idx2) == Game<NPawns>::TileState::TILE_EMPTY) {
          // printf("EMPTY TILE!!!\n");
          return false;
        }

        // printf("Colors same? %s\n", same_color ? "true" : "False");
        // printf("Tile 1: %d\n", g1.getTile(idx));
        // printf("Tile 2: %d\n", g2.getTile(idx2));
        // if (!bool((g1.getTile(idx) != g2.getTile(idx2)) ^ same_color)) {
        //   printf("UNEQUAL TILES!!!!\n");
        // }

        return true;
      });
}

}  // namespace onoro
