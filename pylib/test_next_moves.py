
import copy
import random
import subprocess
from typing import Iterable

from onoro import Onoro, deserialize
from game_state_pb2 import GameState, GameStates

Pawn = GameState.Pawn


def gen_starting_game(n_pawns: int) -> Onoro:
  pawns = (
      Pawn(x=1, y=1, black=True),
      Pawn(x=1, y=2, black=False),
      Pawn(x=2, y=2, black=True),
    )
  return Onoro(n_pawns, pawns, False)


# Prints the echo command that can be used in shell
def print_echo_cmd(game: Onoro) -> None:
  state_bytes = bytes(game.serialize().SerializeToString())
  proto_in = len(state_bytes).to_bytes(4, byteorder='big') + state_bytes

  s = 'echo -n -e "'
  for byte in proto_in:
    s += '\\x%02x' % int(byte)

  s += '"'
  print(s)


def run_cc(game: Onoro) -> None:
  proc = subprocess.Popen(['./test_next_moves'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  state_bytes = bytes(game.serialize().SerializeToString())
  proto_in = len(state_bytes).to_bytes(4, byteorder='big') + state_bytes
  proc.stdin.write(proto_in)
  proc.stdin.flush()

  proc.wait(timeout=1)
  print(proc.stderr.read().decode('utf-8'))
  print('returned', proc.returncode)


def get_next_moves_cc(game: Onoro) -> Iterable[Onoro]:
  proc = subprocess.Popen(['./test_next_moves'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  state_bytes = bytes(game.serialize().SerializeToString())
  proto_in = len(state_bytes).to_bytes(4, byteorder='big') + state_bytes
  proc.stdin.write(proto_in)
  proc.stdin.flush()

  proc.wait(timeout=1)
  if proc.returncode != 0:
    print(proc.stdout.read().decode('utf-8'))
    print(proc.stderr.read().decode('utf-8'))
    assert(proc.returncode == 0)

  res = proc.stdout.read()
  res = res[4:]

  gs = GameStates()
  gs.ParseFromString(res)

  for g in gs.state:
    try:
      yield deserialize(g, game.num_pawns)
    except RuntimeError as e:
      print(game.serialize().SerializeToString())
      print(game)
      print(deserialize(g, game.num_pawns, check_errors=False).__repr__(check_errors=False))
      raise e

def get_next_moves_py(game: Onoro) -> Iterable[Onoro]:
  for move in game.Moves():
    g = copy.deepcopy(game)
    g.MakeMove(move)
    yield g


def test_next_moves(game: Onoro) -> bool:
  s_cc = set()
  s_py = set()
  for g in get_next_moves_cc(game):
    assert(g not in s_cc)
    s_cc.add(g)
  for g in get_next_moves_py(game):
    assert(g not in s_py)
    s_py.add(g)

  cc_only = s_cc - s_py
  py_only = s_py - s_cc

  if len(cc_only) + len(py_only) != 0:
    print(game)

  res = True
  if len(cc_only) != 0:
    print('Found moves in C++ version only:')
    for g in cc_only:
      print(g.__repr__(diff=game))
    res = False
  if len(py_only) != 0:
    print('Found moves in python version only:')
    for g in py_only:
      print(g.__repr__(diff=game))
    res = False

  return res


def test_random_moves(game: Onoro, max_moves: int) -> bool:
  for i in range(max_moves):
    if not test_next_moves(game):
      print('Failed after', i, 'moves')
      return False
    move = random.choice(list(game.Moves()))
    game.MakeMove(move)

  return True


def main():
  random.seed(1)
  num_pawns = 16
  game = gen_starting_game(num_pawns)

  # gstr = b''
  # gs = GameState()
  # gs.ParseFromString(gstr)
  # game = deserialize(gs, num_pawns)

  print(test_random_moves(game, 100))


if __name__ == '__main__':
  main()
