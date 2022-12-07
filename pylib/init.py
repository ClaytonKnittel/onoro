
from __future__ import annotations
import random
from typing import Any, Iterable, List, Set, Tuple

from game_state_pb2 import GameState

Pawn = GameState.Pawn


class Coord:

  def __init__(self, x, y):
    self.x = x
    self.y = y

  def __eq__(self, coord: Any) -> Coord:
    if isinstance(coord, Coord):
      return self.x == coord.x and self.y == coord.y
    if isinstance(coord, Tuple):
      return self.x == coord[0] and self.y == coord[1]
    return NotImplemented()

  def __radd__(self, coord: Any) -> Coord:
    return self + coord

  def __add__(self, coord: Any) -> Coord:
    if isinstance(coord, Coord):
      return Coord(self.x + coord.x, self.y + coord.y)
    if isinstance(coord, Tuple):
      return Coord(self.x + coord[0], self.y + coord[1])
    return NotImplemented()

  def __radd__(self, coord: Any) -> Coord:
    return self + coord

  def __sub__(self, coord: Any) -> Coord:
    if isinstance(coord, Coord):
      return Coord(self.x - coord.x, self.y - coord.y)
    if isinstance(coord, Tuple):
      return Coord(self.x - coord[0], self.y - coord[1])
    return NotImplemented()

  def __rsub__(self, coord: Any) -> Coord:
    inv = self - coord
    return Coord(-inv.x, -inv.y)

  def __hash__(self) -> str:
    return hash((self.x, self.y))


def PawnToCoord(pawn: Pawn) -> Coord:
  return Coord(pawn.x, pawn.y)


def CoordToPawn(coord: Coord, black: bool) -> Pawn:
  return Pawn(x=coord.x, y=coord.y, black=black)


def coord_neighbors(pawn: Coord) -> Iterable[Coord]:
  return (
      pawn + (1, 0),
      pawn + (1, 1),
      pawn + (0, 1),
      pawn + (-1, 0),
      pawn + (-1, -1),
      pawn + (0, -1),
    )


class Onoro:

  _EMPTY = 0
  _BLACK = 1
  _WHITE = 2

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

  def PawnAt(self, coord: Coord) -> int:
    """Returns the color of the pawn at coord, or EMPTY if no pawn is there."""
    pawns_at = [pawn for pawn in self.pawns if (pawn.x, pawn.y) == coord]
    if not pawns_at:
      return self._EMPTY
    if len(pawns_at) == 1:
      return self._BLACK if pawns_at[0].black else self._WHITE
    raise RuntimeError('Multiple pawns at position (%d, %d)' % (coord.x, coord.y))

  def HasWinner(self) -> bool:
    """Checks if a player has won, only returning true if it is not the winning player's turn."""
    for pawn in pawns:
      color = self._BLACK if pawn.black else self._WHITE
      pawn_pos = Coord(pawn.x, pawn.y)

      for delta in ((1, 0), (1, 1), (0, 1)):
        n_in_row = 1

        pos = pawn_pos + delta
        while PawnAt(pos) == color:
          n_in_row + 1
          pos += delta

        pos = pawn_pos - delta
        while PawnAt(pos) == color:
          n_in_row + 1
          pos -= delta

        if n_in_row >= 4:
          if pawn.black == self.black_turn:
            raise RuntimeError('Cannot have current player winning')
          return True

    return False

  def PlayableSpots(self, pawns: Set[Coord]) -> Iterable[Coord]:
    counts = dict()
    for pawn in pawns:
      for coord in coord_neighbors(pawn):
        if coord in pawns:
          continue
        if coord not in counts:
          counts[coord] = 0
        counts[coord] += 1
    return (loc for loc, count in counts.items() if count >= 2)

  def _RemoveAdjacent(self, pawn: Coord, pawns: Set[Coord]) -> Set[Coord]:
    for neighbor in coord_neighbors(pawn):
      if neighbor in pawns:
        pawns = pawns - {neighbor}
        pawns = self._RemoveAdjacent(neighbor, pawns)
    return pawns

  def Connected(self, pawns: Set[Coord]) -> bool:
    if not pawns:
      return True
    pawn = pawns.pop()
    return not self._RemoveAdjacent(pawn, pawns)

  def TwoNeighborsEach(self, pawns: Set[Coord]) -> bool:
    for pawn in pawns:
      n_neighbors = len([neighbor for neighbor in coord_neighbors(pawn) if neighbor in pawns])
      if n_neighbors < 2:
        return False
    return True

  def P1Moves(self) -> Iterable[Pawn]:
    if len(self.pawns) >= self.num_pawns:
      raise RuntimeError('Not phase 1, %d pawns in play' % (len(self.num_pawns)))

    coords = {PawnToCoord(pawn) for pawn in self.pawns}
    return (CoordToPawn(coord, self.black_turn) for coord in self.PlayableSpots(coords))

  def MakeP1Move(self, move: Pawn) -> None:
    self.pawns += [move,]
    self.black_turn = not self.black_turn

  def P2Moves(self) -> Iterable[Tuple[Pawn, Pawn]]:
    if len(self.pawns) != self.num_pawns:
      raise RuntimeError('Not phase 2, %d pawns in play' % (len(self.num_pawns)))

    pawns = {PawnToCoord(pawn) for pawn in self.pawns}
    for pawn in self.pawns:
      if pawn.black != self.black_turn:
        continue

      pawn_coord = PawnToCoord(pawn)
      rem_pawns = pawns - {pawn_coord}
      for coord in self.PlayableSpots(rem_pawns):
        if coord == pawn_coord:
          continue

        new_pawns = rem_pawns | {coord}
        if self.Connected(new_pawns) and self.TwoNeighborsEach(new_pawns):
          yield (pawn, CoordToPawn(coord, pawn.black))

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
