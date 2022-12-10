
from __future__ import annotations
from typing import Any, Iterable, Tuple

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

  def __hash__(self) -> int:
    return hash((self.x, self.y))

  def __repr__(self) -> str:
    return str((self.x, self.y))


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
