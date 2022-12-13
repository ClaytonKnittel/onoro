
import copy
import random
from typing import Iterable, Set

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


def apply_random_symm_ops(game: Onoro) -> None:
  color_invert = random.choice((False, True))
  reflected = random.choice((False, True))
  orientation = random.randrange(6)

  if len(game.pawns) < game.num_pawns:
    color_invert = False

  if reflected:
    game.refl()
  for i in range(orientation):
    game.rotate_60()
  if color_invert:
    game.invert_colors()


def each_symm(game: Onoro) -> Iterable[Onoro]:
  g = copy.deepcopy(game)
  for color_invert in ((False, True) if len(game.pawns) == game.num_pawns else (False,)):
    for reflected in (True, False):
      for orientation in range(6):
        yield g
        g.rotate_60()
      g.refl()
    g.invert_colors()


def are_symm_py(g1: Onoro, g2: Onoro) -> bool:
  for g in each_symm(g1):
    if g == g2:
      return True
  return False


def are_symm_cc(g1: Onoro, g2: Onoro) -> bool:
  gs_bytes = GameStates(state=(g1.serialize(), g2.serialize())).SerializeToString()
  return test_symmetries_cc.are_symmetries(gs_bytes)


def insert_symm(cache: Set[Onoro], game: Onoro, do_print=False) -> bool:
  for g in each_symm(game):
    if g in cache:
      if do_print:
        if not color_invert and not reflected and orientation == 0:
          print('found exact game')
        else:
          if reflected:
            r_str = 's' + str(orientation)
          else:
            r_str = 'r' + str(orientation)
          print('found game upon ' + r_str + \
              (' color inverted' if color_invert else ''))
        print(g)

      for g2 in each_symm(game):
        res1 = are_symm_cc(game, g2)
        res2 = are_symm_cc(g2, game)

        if res1 is None or res2 is None:
          print(game)
          print(g2)
          assert(False)

        assert(res1)
        assert(res2)
      return False

  g_copy = copy.deepcopy(game)
  apply_random_symm_ops(g_copy)
  cache.add(g_copy)
  return True


def test_random_moves(game: Onoro, max_moves: int, do_print=False) -> bool:
  cache = set()
  cache.add(game)

  for i in range(max_moves):
    while True:
      prev = random.choice(tuple(cache))
      g = copy.deepcopy(prev)
      if i % 500 == 0:
        print('turn', i)
        print(g)

      if g.HasWinner():
        continue
      else:
        break
    move = random.choice(tuple(g.Moves()))
    g.MakeMove(move)

    if not insert_symm(cache, g) and do_print:
      print(g)

  cache_list = list(cache)

  for t in range(10):
    print('trial', t)
    random.shuffle(cache_list)
    for i, g in enumerate(cache_list):
      if i % 1000 == 0:
        print('ineq checking', i)
      g2 = cache_list[(i + 1) % len(cache_list)]
      assert(are_symm_py(g, g2) == are_symm_cc(g, g2))

  # print(cache)
  return True


def main():
  random.seed(3)
  num_pawns = 16
  game = gen_starting_game(num_pawns)

  assert(test_random_moves(game, 5000))


if __name__ == '__main__':
  main()
