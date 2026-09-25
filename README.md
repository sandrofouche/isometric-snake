# Isometric Snake

A playable C++20 prototype of Snake rendered as a colorful 18×18 isometric board. The game rules are independent from the renderer, making later additions—levels, hazards, power-ups, enemies, or alternate cameras—straightforward.

New to Git or GitHub? Start with the [Student Setup Guide](STUDENT_SETUP.md).

## Play

- **Left/A**: turn left relative to the snake's current heading
- **Right/D**: turn right relative to the snake's current heading
- **Up/W**: continue forward (the snake moves automatically)
- **Down/S**: no reverse; reversing into the snake is intentionally disabled
- **Space**: fire the head-mounted blaster
- **P**: pause
- **R**: restart
- **Esc**: quit

Eat an apple for 10 points and growth, or shoot it for 5 points without growing. Avoid the walls and your own body. The game speeds up as the score increases.

The head-mounted blaster fires fast projectiles that stop at the board edge. Firing, hits, eating, and crashes have procedurally generated sound effects, so no external audio assets are required.

## Build

You need a C++20 compiler and raylib. You can build directly with GNU Make, or use CMake 3.20 or newer. The CMake build downloads raylib during its first build.

### macOS or Linux

On macOS with Homebrew, install the graphical dependency and build tools:

```sh
brew install raylib pkg-config
```

Then build and run directly with Make:

```sh
make
make run
```

The gameplay tests do not need raylib:

```sh
make test
```

Alternatively, use the CMake build:

```sh
cmake -S . -B build
cmake --build build --config Release
./build/isometric_snake
```

On Debian/Ubuntu, raylib's build may first require:

```sh
sudo apt install build-essential cmake git libasound2-dev libx11-dev libxrandr-dev \
  libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
```

### Windows

From a Visual Studio Developer PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\isometric_snake.exe
```

## Tests

```sh
cmake --build build --target snake_tests
ctest --test-dir build --output-on-failure
```
