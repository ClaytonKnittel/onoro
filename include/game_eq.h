#pragma once

#include "game.h"
#include "game_view.h"

namespace onoro {

template <uint32_t NPawns>
class GameEq {
 public:
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

  switch (s1.symm_class) {
    case SymmState::C: {
      return compareViews<D6COp>(view1, view2, s1, s2);
    }
  }
}

template <uint32_t NPawns>
template <class SymmetryClassOp>
bool GameEq<NPawns>::compareViews(
    const GameView<NPawns>& view1, const GameView<NPawns>& view2,
    typename Game<NPawns>::BoardSymmetryState s1,
    typename Game<NPawns>::BoardSymmetryState s2) {
  using Group = typename SymmetryClassOp::group;

  const Game<NPawns>& g1 = view1.game();
  const Game<NPawns>& g2 = view2.game();

  bool same_color = !(view1.invertColors() ^ view2.invertColors());

  if (s1.symm_class != s2.symm_class) {
    return false;
  }

  // TODO compare positions of pawns
  // need to write iterator over board tiles which applies view.op() (D6, about
  // origin()) and s1.op() (type derived from symm_class, about origin()) to
  // each position, and then checks if each is a pawn in the other view (need to
  // invert s2.op() and view2.op() and getTile(), accounting for color
  // symmetries).

  if (g1.nPawnsInPlay() != g2.nPawnsInPlay()) {
    return false;
  }

  if (g1.blackTurn() ^ g2.blackTurn() ^ same_color) {
    return false;
  }

  Group view_op1 = view1.template op<Group>();
  Group view_op2 = view2.template op<Group>();

  // Group operation to translate a point in view 2 to a point in view 1.
  Group to_view1 = view_op1 * view_op2.inverse();

  return g1.forEachPawn([&g1, &g2, same_color, to_view1](idx_t idx) {
    HexPos p2 = Game<NPawns>::idxToPos(idx);
    p2 = SymmetryClassOp::apply_fn(p2, to_view1);
    idx_t idx2 = Game<NPawns>::posToIdx(p2);

    if (g2.getTile(idx2) == Game<NPawns>::TileState::TILE_EMPTY) {
      return false;
    }

    return (g1.getTile(idx) != g2.getTile(idx2)) ^ same_color;
  });
}

}  // namespace onoro
