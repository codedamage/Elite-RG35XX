# Elite → original RG35XX

Port of the classic **Elite** to the **original Anbernic RG35XX** (GarlicOS), via the
open-source C reimplementation **Elite: The New Kind (newkind)** — *not* by emulating the
Archimedes/BBC original (that would need an emulator; see MASTER plan §Approach).

> **Device:** Actions ATM7039S · ARM Cortex-A9 · **256MB RAM** · PowerVR SGX544 · 640×480 ·
> GarlicOS (**uClibc, armv5te, soft-float** — MiyooCFW toolchain). No WiFi.

## Start here
1. **[Elite-Port-MASTER.md](Elite-Port-MASTER.md)** — the full plan, approach, and sequencing.
2. **[tasks/](tasks/)** — the work split into ordered, self-contained task files.
3. **[docs/DEVICE-ABI.md](docs/DEVICE-ABI.md)** — confirmed device ABI + toolchain facts.

## Approach in one line
Take **`lgblgblgb/newkind`** (SDL2), **backport it to SDL 1.2** to link the device's on-board
**SDL 1.2 + SDL_gfx**, cross-compile with the **MiyooCFW uClibc Docker toolchain**, add a
**gamepad→keyboard** input layer, and package as a **GarlicOS port**. Reuses the pipeline proven
in the EverlastingSummer port project. (Engine decision verified — see
[tasks/01](tasks/01-source-and-engine-selection.md).)

## Legal
Elite © Bell & Braben. *The New Kind* is a fan reimplementation with a murky history
(distribution was paused in 2003, later re-allowed around Elite's 30th). Treat this as a
**personal, non-distributed** build. Don't redistribute binaries. Do not commit game data
that isn't clearly free.

## Status
🟡 Planning. See MASTER + tasks.
