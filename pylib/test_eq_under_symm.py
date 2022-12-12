
import copy
import random
from typing import Set

from onoro import Onoro, deserialize
from game_state_pb2 import GameState, GameStates

import test_symmetries_cc

Pawn = GameState.Pawn


def gen_starting_game(n_pawns: int) -> Onoro:
  pawns = (
      Pawn(x=1, y=1, black=True),
      Pawn(x=1, y=2, black=False),
      Pawn(x=2, y=2, black=True),
    )
  return Onoro(n_pawns, pawns, False)


def insert_symm(cache: Set[Onoro], game: Onoro) -> bool:
  g_copy = copy.deepcopy(game)
  for color_invert in (False, True):
    for orientation in range(6):
      if g_copy in cache:
        return False

      g_copy.rotate_60()
    g_copy.invert_colors()

  assert(game == g_copy)
  cache.add(g_copy)
  return True


def test_random_moves(game: Onoro, max_moves: int) -> bool:
  prev = None
  cache = set()

  def print_game() -> None:
    if prev is not None:
      print(game.__repr__(diff=prev))
    else:
      print(game)

  for i in range(max_moves):
    print_game()
    prev = copy.deepcopy(game)

    if game.HasWinner():
      print('someone won after %d moves!' % i)
      break
    move = random.choice(list(game.Moves()))
    game.MakeMove(move)

    insert_symm(cache, game)

  print_game()
  print(cache)
  return True


def main():
  random.seed(3)
  num_pawns = 16
  game = gen_starting_game(num_pawns)

  print(test_random_moves(game, 5))


if __name__ == '__main__':
  main()
