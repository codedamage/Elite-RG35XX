# Controls — Elite RG35XX

Gamepad → keyboard translation is implemented in `engine/patches/sdl12_compat.c`
(synthetic keypress injection; no changes to the game logic).

**Confirmed button indices** (from `InputProbe` on-device):  
D-pad = HAT 0 (Up=1 / Right=2 / Down=4 / Left=8)  
**A=0, B=1, X=2, Y=3, L1=5, R1=6, Select=7, Start=8, Menu=9**  
L2/R2 = analog axes 2 / 5

---

## Base Layer — Flight

| RG35XX          | Elite key | Action                                   |
|:----------------|:---------:|:-----------------------------------------|
| D-pad Up / Down | ↑ / ↓     | Pitch                                    |
| D-pad Left / Right | ← / →  | Roll                                     |
| A               | `a`       | Fire laser                               |
| B / R1 / R2     | Space     | Accelerate                               |
| L1 / L2         | `/`       | Decelerate                               |
| X               | `n`       | New commander *(intro only)*             |
| Y               | `y`       | Load commander *(intro only)*            |
| Menu            | `p`       | Pause / Resume                           |

---

## Hold Select — Views & Screens

| Select +        | Elite key | Screen / Action                          |
|:----------------|:---------:|:-----------------------------------------|
| D-pad Up        | F1        | Front view · **Launch** (when docked)    |
| D-pad Down      | F2        | Rear view                                |
| D-pad Left      | F3        | Left view                                |
| D-pad Right     | F4        | Right view · Buy equipment (docked)      |
| A               | F5        | Galactic chart                           |
| B               | F6        | Short-range chart                        |
| X               | F7        | Planet info                              |
| Y               | F8        | Market / Buy-Sell                        |
| R1              | F9        | Commander status                         |
| L1              | F10       | Inventory                                |

---

## Hold Start — Combat & Utility

| Start +         | Elite key | Action                                   |
|:----------------|:---------:|:-----------------------------------------|
| A               | `m`       | Fire missile                             |
| B               | `t`       | Target missile                           |
| X               | `u`       | Un-arm missile                           |
| Y               | `e`       | ECM                                      |
| R1              | `h`       | Hyperspace                               |
| L1              | `j`       | In-system jump                           |
| D-pad Up        | Tab       | Energy bomb                              |
| D-pad Down      | `c`       | Docking computer toggle                  |
| D-pad Left      | `z`       | Scanner zoom                             |
| D-pad Right     | F11       | Options / Save / Load / Quit             |

---

## System

- **Quit**: GarlicOS system menu (SIGTERM → clean exit).

---

## First Run Walkthrough

1. **Intro screen 1** — "Load commander? Y/N" → press **X** (new) or **Y** (load saved).
2. **Intro screen 2** — "Press Fire or Space, Commander." → press **B**.
3. **Docked** → **Select + D-pad Up** to launch.
4. **In space** → D-pad to fly, A to fire.

---

## Notes

- Momentary button actions are held for a few frames to ensure the game's keyboard poll catches them.
- L2/R2 are wired identically to L1/R1 (some revisions report digital shoulder buttons as analog axes).
- Energy bomb (`Tab`) is on **Start + D-pad Up** to avoid accidental triggers mid-combat.
- The function-key layer (**Select + …**) mirrors Elite's original F-key row as closely as possible.
