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
};

template <uint32_t NPawns>
bool GameEq<NPawns>::operator()(const GameView<NPawns>& view1,
                                const GameView<NPawns>& view2) const noexcept {
  using SymmState = typename Game<NPawns>::BoardSymmetryState;

  const Game<NPawns>& g1 = view1.game();
  const Game<NPawns>& g2 = view2.game();

  SymmState s1 = g1.calcSymmetryState();
  SymmState s2 = g2.calcSymmetryState();

  if (s1.symm_class != s2.symm_class) {
    return false;
  }

  // TODO compare positions of pawns
  // need to write iterator over board tiles which applies view.op() (D6, about
  // origin()) and s1.op() (type derived from symm_class, about origin()) to
  // each position, and then checks if each is a pawn in the other view (need to
  // invert s2.op() and view2.op() and getTile(), accounting for color
  // symmetries).

  return true;
}

}  // namespace onoro
