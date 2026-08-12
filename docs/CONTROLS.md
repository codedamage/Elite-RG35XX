# RG35XX Control Scheme (DRAFT)

Elite has ~30 actions; the RG35XX has ~10 buttons + D-pad. Solution: a **base flight
layer** plus two **modifier layers** (hold Select / hold Start). Final button *indices*
come from `InputProbe` (`docs`/`probe.log`); the labels below (A/B/X/Y/L/R) are what we
map once we know which SDL index each physical button is.

## Base layer (no modifier) — flight
| RG35XX | Elite key | Action |
|---|---|---|
| D-pad Up / Down | s / x (or ↑/↓) | Pitch |
| D-pad Left / Right | , / . (or ←/→) | Roll |
| A | a | Fire laser |
| R (shoulder) | Space | Accelerate |
| L (shoulder) | / | Decelerate |
| B | j | In-system jump |
| Y | h | Hyperspace |
| X | e | ECM |
| Start (tap) | p / r | Pause / resume |

## Hold **Select** — views & screens (F1–F11)
| Select + | Elite | Screen |
|---|---|---|
| D-pad Up | F1 | Front view / **Launch** (when docked) |
| D-pad Down | F2 | Rear view |
| D-pad Left | F3 | Left view |
| D-pad Right | F4 | Right view |
| A | F5 | Galactic chart |
| B | F6 | Local chart |
| X | F7 | System data |
| Y | F8 | Market prices |
| L | F9 | Status / commander |
| R | F10 | Inventory |
| Start | F11 | Options |

## Hold **Start** — combat & utility
| Start + | Elite | Action |
|---|---|---|
| A | t | Target missile |
| B | m | Fire missile |
| X | u | Un-arm missile |
| Y | Tab | Energy bomb |
| L | c | Docking computer |
| R | z | Scanner zoom |

## Menus / prompts
- **Yes / No** prompts (`y`/`n`): A = Yes, B = No.
- **Chart cursor / text entry**: D-pad moves cursor; A = Enter; B = Backspace.
- Commander-name entry (rare): deferred — may add an on-screen keyboard later.

## System
- **Quit to Garlic**: hold **Select + Start** together (~1s).

## Notes
- New commander at first launch: intro-1 asks Y/N → **A** (Yes) or **B** (No).
- Intro-2 "Press Fire or Space" → **A** or **R**.
- Implemented in the shim's joystick→keyboard layer (`sdl12_compat.c`) with modifier-hold
  state; nothing in the game itself changes.
