
# Timings without hashing:

```
Num pawns = 8

 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . B B . . . .
. . . . . W . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
Move search time at depth 5: 0.000386 s
Move (5, 3), score 0 (2348 playouts)
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . W . . . .
 . . . . B B . . . .
. . . . . W . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
Move search time at depth 4: 0.000128 s
Move (4, 3), score 0 (3172 playouts)
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . B W . . . .
 . . . . B B . . . .
. . . . . W . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
Move search time at depth 3: 0.000030 s
Move (4, 2), score 0 (3377 playouts)
 . . . . . . . . . .
. . . . . . . . . .
 . . . . W . . . . .
. . . . B W . . . .
 . . . . B B . . . .
. . . . . W . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
Move search time at depth 2: 0.000006 s
Move (3, 2), score 0 (3421 playouts)
 . . . . . . . . . .
. . . . . . . . . .
 . . . B W . . . . .
. . . . B W . . . .
 . . . . B B . . . .
. . . . . W . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
Move search time at depth 1: 0.000001 s
Move (4, 1), score 0 (3428 playouts)
 . . . . . . . . . .
. . . . W . . . . .
 . . . B W . . . . .
. . . . B W . . . .
 . . . . B B . . . .
. . . . . W . . . .
 . . . . . . . . . .
. . . . . . . . . .
 . . . . . . . . . .
. . . . . . . . . .
```

```
Num pawns = 12

 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 9: 0.446546 s
Move (7, 5), score 0 (3457275 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 8: 0.201549 s
Move (6, 5), score 0 (5043469 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 7: 0.049840 s
Move (6, 4), score 0 (5436772 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . W . . . . . . .
. . . . . . B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 6: 0.012455 s
Move (5, 4), score 0 (5534854 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . B W . . . . . . .
. . . . . . B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 5: 0.001993 s
Move (5, 5), score 0 (5549145 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . B W . . . . . . .
. . . . . W B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 4: 0.000237 s
Move (6, 3), score 0 (5551034 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . B . . . . . . .
 . . . . . B W . . . . . . .
. . . . . W B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 3: 0.000013 s
Move (5, 3), score 0 (5551133 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . W B . . . . . . .
 . . . . . B W . . . . . . .
. . . . . W B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 2: 0.000008 s
Move (5, 2), score 0 (5551207 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . B . . . . . . . .
. . . . . W B . . . . . . .
 . . . . . B W . . . . . . .
. . . . . W B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
Move search time at depth 1: 0.000002 s
Move (4, 2), score 0 (5551216 playouts)
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . W B . . . . . . . .
. . . . . W B . . . . . . .
 . . . . . B W . . . . . . .
. . . . . W B W . . . . . .
 . . . . . . B B . . . . . .
. . . . . . . W . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
 . . . . . . . . . . . . . .
. . . . . . . . . . . . . .
```

```
Num pawns = 16

 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 13: 259.611394 s
Move (9, 7), score 0 (2100045969 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 12: 236.046740 s
Move (8, 7), score 0 (4024518049 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . B W . . . . . . . .
 . . . . . . . . B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 11: 50.062633 s
Move (7, 8), score 0 (4435189645 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 10: 36.393816 s
Move (7, 7), score 0 (4733021056 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 9: 8.058577 s
Move (7, 6), score 0 (4799542698 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 8: 1.833503 s
Move (6, 6), score 0 (4814718661 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 7: 0.359620 s
Move (7, 5), score 0 (4817703884 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . W . . . . . . . . . .
 . . . . . . B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 6: 0.065321 s
Move (6, 5), score 0 (4818242084 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . B W . . . . . . . . . .
 . . . . . . B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 5: 0.006926 s
Move (5, 6), score 0 (4818300908 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . B W . . . . . . . . . .
 . . . . . W B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 4: 0.000674 s
Move (6, 4), score 0 (4818306620 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . B . . . . . . . . . . .
. . . . . . B W . . . . . . . . . .
 . . . . . W B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 3: 0.000018 s
Move (5, 4), score 0 (4818306765 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . W B . . . . . . . . . . .
. . . . . . B W . . . . . . . . . .
 . . . . . W B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 2: 0.000013 s
Move (6, 3), score 0 (4818306879 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . B . . . . . . . . . . .
 . . . . . W B . . . . . . . . . . .
. . . . . . B W . . . . . . . . . .
 . . . . . W B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
Move search time at depth 1: 0.000002 s
Move (5, 3), score 0 (4818306890 playouts)
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . W B . . . . . . . . . . .
 . . . . . W B . . . . . . . . . . .
. . . . . . B W . . . . . . . . . .
 . . . . . W B W . . . . . . . . . .
. . . . . . . B B W . . . . . . . .
 . . . . . . . W B B . . . . . . . .
. . . . . . . . . W . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
 . . . . . . . . . . . . . . . . . .
. . . . . . . . . . . . . . . . . .
```

