
import subprocess

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

def main():
  proc = subprocess.Popen(['./test_next_moves'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

  game = gen_starting_game(16)
  state_bytes = bytes(game.serialize().SerializeToString())
  print(state_bytes)
  proto_in = len(state_bytes).to_bytes(4, byteorder='big') + state_bytes
  proc.stdin.write(proto_in)
  proc.stdin.flush()

  proc.wait(timeout=1)
  assert(proc.returncode == 0)
  res = proc.stdout.read()
  res = res[4:]

  gs = GameStates()
  gs.ParseFromString(res)
  print(gs)


if __name__ == '__main__':
  main()
