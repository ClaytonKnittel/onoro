
import random
from typing import Iterable, List, Set, Tuple

from game_state_pb2 import GameState

Pawn = GameState.Pawn

def coord_neighbors(pawn: Tuple[int, int]) -> Iterable[Tuple[int, int]]:
  return (
      (pawn[0] + 1, pawn[1]),
      (pawn[0] + 1, pawn[1] + 1),
      (pawn[0], pawn[1] + 1),
      (pawn[0] - 1, pawn[1]),
      (pawn[0] - 1, pawn[1] - 1),
      (pawn[0], pawn[1] - 1),
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

  def PlayableSpots(self, pawns: Set[Tuple[int, int]]) -> Iterable[Tuple[int, int]]:
    counts = dict()
    for pawn in pawns:
      for coord in coord_neighbors(pawn):
        if coord in pawns:
          continue
        if coord not in counts:
          counts[coord] = 0
        counts[coord] += 1
    return (loc for loc, count in counts.items() if count >= 2)

  def _RemoveAdjacent(self, pawn: Tuple[int, int], pawns: Set[Tuple[int, int]]) -> Set[Tuple[int, int]]:
    for neighbor in coord_neighbors(pawn):
      if neighbor in pawns:
        pawns = pawns - {neighbor}
        pawns = self._RemoveAdjacent(neighbor, pawns)
    return pawns

  def Connected(self, pawns: Set[Tuple[int, int]]) -> bool:
    if not pawns:
      return True
    pawn = pawns.pop()
    return not self._RemoveAdjacent(pawn, pawns)

  def TwoNeighborsEach(self, pawns: Set[Tuple[int, int]]) -> bool:
    for pawn in pawns:
      n_neighbors = len([neighbor for neighbor in coord_neighbors(pawn) if neighbor in pawns])
      if n_neighbors < 2:
        return False
    return True

  def P1Moves(self) -> Iterable[Pawn]:
    if len(self.pawns) >= self.num_pawns:
      raise RuntimeError('Not phase 1, %d pawns in play' % (len(self.num_pawns)))

    pawns = {(pawn.x, pawn.y) for pawn in self.pawns}
    return (Pawn(x=pawn[0], y=pawn[1], black=self.black_turn) for pawn in self.PlayableSpots(pawns))

  def MakeP1Move(self, move: Pawn) -> None:
    self.pawns += [move,]
    self.black_turn = not self.black_turn

  def P2Moves(self) -> Iterable[Tuple[Pawn, Pawn]]:
    if len(self.pawns) != self.num_pawns:
      raise RuntimeError('Not phase 2, %d pawns in play' % (len(self.num_pawns)))

    pawns = {(pawn.x, pawn.y) for pawn in self.pawns}
    for pawn in self.pawns:
      if pawn.black != self.black_turn:
        continue

      pawn_coord = (pawn.x, pawn.y)
      rem_pawns = pawns - {pawn_coord}
      for coord in self.PlayableSpots(rem_pawns):
        if coord == pawn_coord:
          continue

        new_pawns = rem_pawns | {coord}
        if self.Connected(new_pawns) and self.TwoNeighborsEach(new_pawns):
          yield (pawn, Pawn(x=coord[0], y=coord[1], black=pawn.black))

  def MakeP2Move(self, move: Tuple[Pawn, Pawn]) -> None:
    self.pawns.remove(move[0])
    self.pawns += (move[1],)
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
