
import random

from onoro import Onoro, deserialize
from game_state_pb2 import GameState

Pawn = GameState.Pawn

def main():
  random.seed(1)
  n_pawns = 16
  pawns = (
      Pawn(x=1, y=1, black=True),
      Pawn(x=1, y=2, black=False),
      Pawn(x=2, y=2, black=True),
    )
  gs = GameState(pawns=pawns,
                 black_turn=False, turn_num=2, finished=False)
  game = deserialize(gs, n_pawns)
  print(game)

  for i in range(len(pawns), n_pawns + 5):
    g = deserialize(gs, n_pawns)
    moves = list(g.Moves())
    move = random.choice(moves)

    g.MakeMove(move)
    print(g)

    gs = g.serialize()

if __name__ == '__main__':
  main()
