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
  return true;
}

}  // namespace onoro
