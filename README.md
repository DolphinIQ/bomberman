### A bomberman style game made ! **from scratch** ! (no libraries, frameworks, or engines) in C programming language. ###

It features:
- Raw `Win32` API to open a window, get mouse/keyboard input and create a buffer for drawing into.
- Raw `XAudio2` API to play a sine wave sound.
- C hot code reloading by:
  1. Separating platform and game code (also great for portability)
  2. Game code compiles into a dynamic library (DLL)
  3. The main executable is the platform main C file which loads the game code DLL
  4. Main provides the memory for game state and reloads DLL game logic whenever the DLL recompiles
  5. Meaning when app runs, game state stays persistent, while game logic can change, allowing quick changes without rerunning the game executable
- Software rendering directly on a frame buffer, allowing for arbitrary frame buffer size, always scaled up to the window size.
- `QueryPerformanceCounter()` to measure time and application performance.
- SIMD intrinsics for speed.
- Tile based world.

This project is meant to be educational only.

This game is a replica of a no longer available bomberman game, that I'm making for my mom.
