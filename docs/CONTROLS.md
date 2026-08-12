# RG35XX Control Scheme (final)

Confirmed "RG35XX Gamepad" indices: D-pad = HAT 0 (U/R/D/L = 1/2/4/8);
**A=0, B=1, X=2, Y=3, L1=5, R1=6, Select=7, Start=8, Menu=9**; **L2/R2 = analog axes 2/5**.
Implemented in `engine/patches/sdl12_compat.c` (joystick → synthetic keyboard, no game changes).
Momentary actions are held a few frames so the game's poll can't swallow them.

## Base layer — flight
| RG35XX | Elite | Action |
|---|---|---|
| D-pad U/D | ↑/↓ | Pitch |
| D-pad L/R | ←/→ | Roll |
| A | a | Fire |
| B / R1 / R2 | Space | Accelerate |
| L1 / L2 | / | Decelerate |
| X | n | *Intro: new commander* (idle in flight) |
| Y | y | *Intro: load commander* |
| Menu | p | Pause / resume |

## Hold **Select** — views & screens
| Select + | Elite | |
|---|---|---|
| Up | F1 | Front view / **Launch (docked)** |
| Down / Left / Right | F2 / F3 / F4 | Rear / Left / Right view |
| A / B | F5 / F6 | Galactic / Local chart |
| X / Y | F7 / F8 | System data / Market |
| R1 / L1 | F9 / F10 | Status / Inventory |

## Hold **Start** — combat & utility
| Start + | Elite | Action |
|---|---|---|
| A | m | Fire missile |
| B | t | Target missile |
| X | u | Un-arm missile |
| Y | e | ECM |
| R1 | h | Hyperspace |
| L1 | j | In-system jump |
| Up | Tab | Energy bomb |
| Down | c | Docking computer |
| Left | z | Scanner zoom |
| Right | F11 | Options |

## System
- **Quit**: GarlicOS menu (SIGTERM handled → clean exit).

## First run
1. Intro-1 "Load commander? Y/N" → **X** (new) / **Y** (load).
2. Intro-2 "Press Fire or Space" → **B**.
3. Docked → **Select + Up** to launch, then fly.
