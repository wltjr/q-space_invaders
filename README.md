# Q-Space Invaders
[![License](http://img.shields.io/badge/license-MIT-blue.svg?colorB=9977bb&style=plastic)](https://github.com/wltjr/q-space_invaders/blob/master/LICENSE)
[![Code Quality](https://sonarcloud.io/api/project_badges/measure?project=wltjr_q-space_invaders&metric=alert_status)](https://sonarcloud.io/dashboard?id=wltjr_q-space_invaders)

Q-Learning Space Invaders using the [Arcade Learning Environment (ALE)](https://ale.farama.org/)

The following papers were used as reference for the implementation:
- Multi-agent reinforcement learning: independent versus cooperative agents [link](https://dl.acm.org/doi/10.5555/3091529.3091572)

## State Space
The state space consists of the laser cannon's X coordinate, a total of
160 potential values, reduced to a range of 38 to 120, for the game
playing bounds, invisible walls. There are 4 actions. The total q-table
size is 160 x 4, even though only ~82 are used, with some outliers.

### Action Space
The
[ALE Space Invaders Action Space](https://ale.farama.org/environments/space_invaders/#actions)
has been reduced from 6 to the following 4 actions.

| Value | Meaning |
|-------|---------|
| 0 | NOOP |
| 1 | FIRE |
| 2 | RIGHT |
| 3 | LEFT |
| 4 | RIGHT-FIRE |
| 5 | LEFT-FIRE |

## System Requirements

The following software is required for proper operation

- [GCC >= 14.1](https://gcc.gnu.org/releases.html)
- [CMake >= 3.20](https://cmake.org/download/)
- [libsdl >= 2.30](https://www.libsdl.org/)
- [ALE >= 0.11.0](https://ale.farama.org/)
- [OpenCV >= 4.11.0](https://opencv.org/releases/)

Atari ROM `space_invaders.bin` was obtained from
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

## Credits

Credits and thanks for resources referenced and used in this repository,
including some code and/or project structure, go to the following:

- [Introducing Q-Learning](https://huggingface.co/learn/deep-rl-course/unit2/q-learning)
- [Mastering Atari Games with Q-Learning](https://medium.com/aimonks/mastering-atari-games-with-q-learning-c222800c7cb3)
- [Approximate Q-Learning With Atari Game: A first Approach To Reinforcement Learning](https://medium.com/@olatejuemmanuel/approximate-q-learning-with-atari-game-a-first-approach-to-reinforcement-learning-da78162ae205)
- [Space Invaders challenge: a Reinforcement Learning competition](https://wandb.ai/raghmura/qualcomm/reports/Space-Invaders-challenge-a-Reinforcement-Learning-competition--Vmlldzo5MzEzMg)
- [Object Detection in 2D Video Games Using the cv2 Match Function in Python](https://medium.com/@viem2377/object-detection-in-2d-video-games-using-the-cv2-match-function-in-python-ee8908fb8c53)
- [Template Matching in OpenCV](https://docs.opencv.org/4.x/d4/dc6/tutorial_py_template_matching.html)
