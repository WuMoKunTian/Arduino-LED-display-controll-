# Algorithm Notes

This project preserves the original 2016 Arduino behavior. The code was renamed and formatted, but the animation logic remains the same.

## Shock Input

The sketch reads the shock sensor on `D8` with `pulseIn(SHOCK_PIN, HIGH)` after a short `10ms` delay.

The pulse value controls the display:

| Pulse measurement | Behavior |
| --- | --- |
| `0` | Clear the LED matrix |
| `> 1000` and `<= 10000` | Render one expanding circle frame |
| `> 10000` | Play the fixed frame animation |

## Circle Rendering

The circle animation is procedural. It does not use predefined circle sprites.

### Random Center

When the radius reaches the end of an 8-step cycle, the sketch chooses a new center:

```cpp
centerX = 2 + random(0, 4);
centerY = 2 + random(0, 4);
```

This means the center is always in the middle 4x4 area of the 8x8 matrix:

```text
x = 2, 3, 4, or 5
y = 2, 3, 4, or 5
```

This matches the original sketch and keeps the expanding rings visually centered.

### Radius Expansion

The radius step starts at `0` and increases by `1` after each rendered frame. When the radius step is divisible by the matrix width (`8`), the circle is reset and a new center is selected.

The mathematical radius used for drawing is:

```text
radius = radiusStep - 0.5
```

The `-0.5` offset is part of the original implementation. It aligns the circle boundary with the square LED cells rather than only the LED center points.

### LED Cell Boundary Test

Each LED is treated as a square cell. For a cell at `(row, column)`, the four corners are:

```text
(row - 0.5, column - 0.5)
(row - 0.5, column + 0.5)
(row + 0.5, column - 0.5)
(row + 0.5, column + 0.5)
```

A point `(x, y)` is inside the circle when:

```text
(x - centerX)^2 + (y - centerY)^2 < radius^2
```

The LED turns on when the circle boundary intersects that square cell. In code, this is tested as:

```text
not(all four corners are inside the circle)
and
not(all four corners are outside the circle)
```

If all four corners are inside, the circle has already passed over the cell. If all four corners are outside, the boundary does not touch the cell. If the corners are mixed, the boundary crosses the cell, so the LED is turned on.

### Matrix Refresh

For each rendered circle frame:

1. The sketch computes one byte per matrix row.
2. It shifts one bit into the row byte for each column.
3. It writes the row with `matrix.setRow()`.
4. It waits `21ms`.
5. It increments the radius step.

## Fixed Frame Animation

The strong-shock animation is the original 11-frame byte animation. Each frame is written row by row to the MAX7219 module, followed by a `42ms` delay.

The modernization extracts this repeated row-writing logic into helper functions, but the frame data and timing are unchanged.
