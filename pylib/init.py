
import random

from onoro import Onoro, deserialize
from game_state_pb2 import GameState

Pawn = GameState.Pawn

def main():
  random.seed(1)
  n_pawns = 6
  pawns = (
      Pawn(x=1, y=1, black=True),
      Pawn(x=1, y=2, black=False),
      Pawn(x=2, y=2, black=True),
    )
  gs = GameState(pawns=pawns,
                 black_turn=False, turn_num=2, finished=False)
  game = deserialize(gs, n_pawns)
  print(game)

  for i in range(3, n_pawns):
    g = deserialize(gs, n_pawns)
    moves = list(g.P1Moves())
    move = random.choice(moves)

    g.MakeP1Move(move)
    print(g)

    gs = g.serialize()

  for i in range(n_pawns, n_pawns + 5):
    g = deserialize(gs, n_pawns)
    moves = list(g.P2Moves())
    move = random.choice(moves)

    g.MakeP2Move(move)
    print(g)

    gs = g.serialize()

if __name__ == '__main__':
  main()
