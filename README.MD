# Cryxtels Atmosphere Tester

This program is a minimal reproduction of the Cryxtels rendering environment.

I developed it for prototyping in order to fix a bug with atmospheric rendering, but it might be a good starting point for future **refactoring** of the codebase, too. The file `renderer.cc` and the **Renderer class** could be especially useful. There's also a Player class but it's still very crude.

The new atmospheric rendering is working pretty good: it allows **texture scrolling at higher resolution**, but it seems to "lag" a bit on my machine. Maybe floating point arithmetic? (always blame floating point arithmetic)

This code still doesn't integrate the new rendering changes to make textures 8-bit indexed and simplify the palette code [(#38)](https://github.com/Enet4/cryxtels/pull/38).

## How to use it

You can see the new algorithm in action by running the following:

```bash
g++ atm.cc -o atm -lSDL3
./atm <path to atmosphere file>
```

Make sure to give it the path to an `.ATM` file from Crystal Pixels, like `NEBULA.ATM`.

The program will create two windows, one at low resolution (320x200, 2x pixel scaling) and one at double resolution (640x400, 1x pixel scaling). You can place them side by side and rotate the player by using the **arrow keys**. The two windows should display the same content at all times.