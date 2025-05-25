# Q-Space Invaders

Q-Learning Space Invaders using the [Arcade Learning Environment (ALE)](https://ale.farama.org/)

## System Requirements

The following software is required for proper operation

  [GCC >= 14.1](https://gcc.gnu.org/releases.html)
  [CMake >= 3.20](https://cmake.org/download/)
  [libsdl >= 2.30](https://www.libsdl.org/)
  [ALE >= 0.11.0](https://ale.farama.org/)
  [OpenCV >= 4.11.0](https://opencv.org/releases/)

Requires Atari ROM `space_invaders.bin` which can be obtained from
[Atari Mania](https://www.atarimania.com/game-atari-2600-vcs-space-invaders_s6947.html)

## Build

Run the following commands in the root directory of the repository to compile
all executables. The base project uses cmake build system with default of make.

```bash
cmake ./
make
```

## Run

The primary executable is `qsi` multi-agent hedonic simulation environment.
The program is implemented using GNU Argp, and has available `--help` menu for
information on the arguments that each program accepts, which are required and
are optional.
