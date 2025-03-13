
# 🚀 Missile Command (Ncurses)

SCC110 Coursework: A remake of the Atari game Missile Command, built using C with the Ncurses library. Defend your cities from incoming missiles by launching counter-missiles!

![ezgif com-resize](https://github.com/user-attachments/assets/0b050898-ad8a-4078-90e2-05bb709f6dfe)


### What is [Ncurses](https://en.wikipedia.org/wiki/Ncurses)?

> ncurses (new curses) is a programming library for creating textual user interfaces (TUIs) that work across a wide variety of terminals; it is written in a way that attempts to optimize the commands that are sent to the terminal, so as reduce the latency experienced when updating the displayed content[^1].

[^1]: https://en.wikipedia.org/wiki/Ncurses

### Features

WIP
- [x] Animated Missile trails and explosions
- [x] Endless Level System
- [x] Scoring based on missiles stopped and remaining cities/ammo
- [x] Escalating Difficulty
- [x] WASD/SPACE Keyboard Controls
- [x] Point/Click Mouse Controls
- [x] Title/Game Over Screens 
- [ ] Finished Game Over conditions
- [ ] High Score Tracking
- [ ] Sound Effects

![missile_gameplay_1-gif-ezgif com-resize](https://github.com/user-attachments/assets/95e17c5a-b1e8-4ef9-8b4d-d172eb0f7b5a)

![missile_gameplay_2-gif-ezgif com-resize](https://github.com/user-attachments/assets/60db1a0f-2df4-4474-ad72-7d3551efd05d)

### How to Play

* WASD to move cursor and SPACE to fire missiles
* Alternatively, Point and click with mouse
* Enemy missiles explode on contact with explosion
* Missiles are limited and represented by icons at center-bottom
* Defend cities to win, points are earned for each remaining city
* Game Over when all cities are lost

### Installation

Prerequisites

  Make sure you have gcc and ncurses installed.

On Linux systems:

`sudo apt install libncurses-dev`
`git clone https://github.com/nlowe2019/missile-command.git`
`cd MissileCommand`
`gcc -o missile_command missile_command.c sprites.c -lncurses -lm`
`./missile_command` 
