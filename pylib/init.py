
from typing import Iterable, List

from game_state_pb2 import GameState

Pawn = GameState.Pawn

def pawn_neighbors(pawn: Pawn) -> Iterable[Pawn]:
  return (
      Pawn(x=pawn.x + 1, y=pawn.y),
      Pawn(x=pawn.x + 1, y=pawn.y + 1),
      Pawn(x=pawn.x, y=pawn.y + 1),
      Pawn(x=pawn.x - 1, y=pawn.y),
      Pawn(x=pawn.x - 1, y=pawn.y - 1),
      Pawn(x=pawn.x, y=pawn.y - 1),
    )

class Onoro:

  def __init__(self, num_pawns: int, pawns: List[GameState.Pawn], black_turn: bool):
    self.num_pawns = num_pawns
    self.black_turn = black_turn
    self.pawns = list(pawns)

  def serialize(self) -> GameState:
    black_pawns = [piece for piece in self.pawns if piece.black]
    white_pawns = [piece for piece in self.pawns if not piece.black]

    if len(black_pawns) < len(white_pawns):
      raise RuntimeError('Fewer black pawns than white (%d vs %d)' %
                         (len(black_pawns), len(white_pawns)))
    if len(black_pawns) > len(white_pawns) + 1:
      raise RuntimeError('Too many black pawns (%d vs %d)' %
                         (len(black_pawns), len(white_pawns)))
    if len(self.pawns) > self.num_pawns:
      raise RuntimeError('Too many pawns (have %d, expect %d)' %
                         (len(self.pawns), self.num_pawns))

    gs = GameState(
        pawns=self.pawns,
        black_turn=self.black_turn,
        turn_num=len(self.pawns) - 1,
        finished=False
      )
    return gs

  def __repr__(self) -> str:
    minx = min((pawn.x for pawn in self.pawns))
    maxx = max((pawn.x for pawn in self.pawns))
    miny = min((pawn.y for pawn in self.pawns))
    maxy = max((pawn.y for pawn in self.pawns))

    midx = (minx + maxx) // 2
    midy = (miny + maxy) // 2

    offx = self.num_pawns // 2 - 1 - midx
    offy = self.num_pawns // 2 - 1 - midy

    board = ['.'] * (self.num_pawns * self.num_pawns)
    for pawn in self.pawns:
      x = pawn.x + offx
      y = pawn.y + offy
      board[x + y * self.num_pawns] = 'B' if pawn.black else 'W'

    res = ''
    for line in range(self.num_pawns):
      r = self.num_pawns - line - 1
      res += ''.join([' '] * line) + ' '.join(board[(self.num_pawns * r):(self.num_pawns * (r + 1))])
      if line < self.num_pawns - 1:
        res += '\n'
    return res

  def P1Moves(self) -> List[Pawn]:
    if len(self.pawns) >= self.num_pawns:
      raise RuntimeError('Not phase 1, %d pawns in play' % (len(self.num_pawns)))

    pawns = {(pawn.x, pawn.y) for pawn in self.pawns}
    counts = dict()
    for pawn in self.pawns:
      for loc in pawn_neighbors(pawn):
        coord = (loc.x, loc.y)
        if coord in pawns:
          continue
        if coord not in counts:
          counts[coord] = 0
        counts[coord] += 1
    counts = {loc: count for loc, count in counts.items() if count >= 2}
    return [Pawn(x=pawn[0], y=pawn[1]) for pawn in counts.keys()]

  def MakeP1Move(self, move: Pawn) -> None:
    self.pawns += [Pawn(x=move.x, y=move.y, black=self.black_turn),]
    self.black_turn = not self.black_turn

def deserialize(gs: GameState, num_pawns: int) -> Onoro:
  if gs.turn_num != len(gs.pawns) - 1:
    raise RuntimeError('Turn num != num_pawns - 1 (%d vs %d)' %
                       (gs.turn_num, len(gs.pawns)))
  if gs.turn_num < num_pawns - 1 and gs.black_turn != ((gs.turn_num % 2) == 1):
    raise RuntimeError('Expect %s player on turn %d' %
                       ('black' if not gs.black_turn else 'white', gs.turn_num))
  return Onoro(num_pawns, gs.pawns, gs.black_turn)

def main():
  pawns = (
      Pawn(x=1, y=1, black=True),
      Pawn(x=1, y=2, black=False),
      Pawn(x=2, y=2, black=True),
    )
  gs = GameState(pawns=pawns,
                 black_turn=False, turn_num=2, finished=False)
  game = deserialize(gs, 16)
  print(game)

  for move in game.P1Moves():
    g = deserialize(gs, 16)
    g.MakeP1Move(move)
    print(g)

if __name__ == '__main__':
  main()
