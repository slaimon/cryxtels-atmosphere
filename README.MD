# Cryxtels Atmosphere Tester

This program is a minimal reproduction of the Cryxtels rendering environment.

I developed it for prototyping in order to fix a bug with atmospheric rendering, but it might be a good starting point for future **refactoring** of the codebase, too. The file `renderer.cc` and the **Renderer class** could be especially useful. There's also a Player class but it's still very crude.

The new atmospheric rendering is working pretty good: it allows **texture scrolling at higher resolution**, but it seems to "lag" a bit on my machine. Maybe floating point arithmetic? (always blame floating point arithmetic)

This code still doesn't integrate the new rendering changes to make textures 8-bit indexed and simplify the palette code [(#38)](https://github.com/Enet4/cryxtels/pull/38).