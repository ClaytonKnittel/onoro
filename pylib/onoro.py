
from typing import Iterable, List, Set, Tuple, Union

from coord import Coord, PawnToCoord, CoordToPawn, coord_neighbors
from game_state_pb2 import GameState

Pawn = GameState.Pawn


class Onoro:

  _EMPTY = 0
  _BLACK = 1
  _WHITE = 2

  _N_IN_ROW_TO_WIN = 4

  SHOW_NEXT_MOVES = False

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
        finished=self.HasWinner()
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
    if self.HasWinner():
      res += 'WINNER: %s\n' % ('white' if self.black_turn else 'black')

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
    for pawn in self.pawns:
      color = self._BLACK if pawn.black else self._WHITE
      pawn_pos = Coord(pawn.x, pawn.y)

      # Check for a win in all 3 possible directions
      for delta in ((1, 0), (1, 1), (0, 1)):
        n_in_row = 1

        pos = pawn_pos + delta
        while self.PawnAt(pos) == color:
          n_in_row += 1
          pos += delta

        pos = pawn_pos - delta
        while self.PawnAt(pos) == color:
          n_in_row += 1
          pos -= delta

        if n_in_row >= self._N_IN_ROW_TO_WIN:
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

  def Moves(self) -> Union[Iterable[Pawn], Iterable[Tuple[Pawn, Pawn]]]:
    if len(self.pawns) == self.num_pawns:
      return self.P2Moves()
    else:
      return self.P1Moves()

  def MakeMove(self, move: Union[Pawn, Tuple[Pawn, Pawn]]) -> None:
    if len(self.pawns) == self.num_pawns:
      assert(isinstance(move, Tuple))
      self.MakeP2Move(move)
    else:
      assert(isinstance(move, Pawn))
      self.MakeP1Move(move)


def deserialize(gs: GameState, num_pawns: int) -> Onoro:
  if gs.turn_num != len(gs.pawns) - 1:
    raise RuntimeError('Turn num != num_pawns - 1 (%d vs %d)' %
                       (gs.turn_num, len(gs.pawns)))
  if gs.turn_num < num_pawns - 1 and gs.black_turn != ((gs.turn_num % 2) == 1):
    raise RuntimeError('Expect %s player on turn %d' %
                       ('black' if not gs.black_turn else 'white', gs.turn_num))
  game = Onoro(num_pawns, gs.pawns, gs.black_turn)

  if gs.finished != game.HasWinner():
    raise RuntimeError('Game state reports %s, but game has %s'
                       % ('winner' if gs.finished else 'no winner',
                          'a winner' if game.HasWinner() else 'no winner'))

  return game
