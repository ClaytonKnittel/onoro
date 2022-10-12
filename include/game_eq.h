#pragma once

#include "game.h"

namespace onoro {

template <uint32_t NPawns>
class GameEq {
 public:
  GameEq() = default;

  bool operator()(const Game<NPawns>& game1,
                  const Game<NPawns>& game2) const noexcept;
};

template <uint32_t NPawns>
bool GameEq<NPawns>::operator()(const Game<NPawns>& game1,
                                const Game<NPawns>& game2) const noexcept {
  return true;
}

}  // namespace onoro
