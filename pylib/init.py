
from game_state_pb2 import GameState

def main():
  gs = GameState(pawns=(GameState.Pawn(x=1, y=2, black=True),), black_turn=False, turn_num=0, finished=False)
  print(gs)

if __name__ == '__main__':
  main()

