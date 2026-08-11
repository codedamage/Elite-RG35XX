# Elite → RG35XX — Master Plan

> **Goal:** play the classic **Elite** on the **original RG35XX (GarlicOS)** by cross-compiling
> the open-source C reimplementation **Elite: The New Kind (newkind)** and packaging it as a
> native GarlicOS port with proper handheld controls.

**Repo:** `F:\PetProjects\Elite RG35XX` · **Device facts:** [docs/DEVICE-ABI.md](docs/DEVICE-ABI.md)

---

## 1. Approach & why

Three ways to get Elite onto this device:

| Approach | Verdict |
|---|---|
| Emulate the Archimedes/BBC original (RPCEmu/ArcEm/b-em) | ❌ Heavy, no packaged handheld build, RISC-OS/game setup fiddly |
| Rewrite from BBC disassembly | ❌ Huge, unnecessary |
| **Cross-compile `newkind` (C reimplementation of Elite)** | ✅ **Chosen** — software-rendered, SDL, tiny, matches our proven pipeline |

`newkind` (Christian Pinder's *Elite: The New Kind*, mirrored/maintained on GitHub) is a
faithful C reimplementation. It is **software-rendered** (no GPU/GL needed — perfect, since the
device has no desktop GL), small, and SDL-based. The device already ships **SDL 1.2**, so an
SDL 1.2 build links straight against it.

**Pipeline (reused from the EverlastingSummer project):**
MiyooCFW uClibc Docker toolchain → cross-compile → GarlicOS `.sh` port → sneakernet to SD → test.

---

## 2. Key facts (confirmed / to verify)

**Confirmed**
- Device ABI: uClibc / armv5te / soft-float; SDL 1.2 present on device (see DEVICE-ABI.md).
- Toolchain available as a prebuilt Docker image (`miyoocfw/toolchain-shared-uclibc:docker`).
- `newkind` is software-rendered C + SDL; forks exist (fesh0r/mdw = SDL 1.2-era; lgblgblgb = SDL2).
- Legal: fan reimplementation, gray status → personal, non-distributed build only.

**To verify (scheduled in tasks 01/05)**
- Exact SDL version of the chosen fork (target **SDL 1.2** to match device; SDL2 = fallback w/ bundling).
- Internal render resolution (needs scaling to 640×480).
- Audio backend (does it need SDL_mixer, or can sound be stubbed?).
- Whether game data files are free to bundle.

---

## 3. Engine decision  ✅ (verified — see [tasks/01](tasks/01-source-and-engine-selection.md))

Both forks were cloned and inspected. Neither is plain SDL 1.2:
- **fesh0r/newkind** = **Allegro 4** (original 2001 code) — worst dependency for this device.
- **lgblgblgb/newkind** = **SDL2**, but clean: bundles `SDL2_gfx`/`rotozoom` as source, only needs
  **SDL2 core + libm**, raw `SDL_OpenAudio`, renders-to-texture (scales to 640×480).

**Device ships SDL 1.2 + SDL_gfx 1.2 (not SDL2, not Allegro).**

**Decision:** take **lgblgblgb** as the base codebase and **backport its SDL2 layer to SDL 1.2**,
linking the device's on-board **SDL 1.2 + SDL_gfx** → *no library bundling*. The SDL2 usage is thin
and maps ~1:1 to SDL 1.2 (`SDL_OpenAudio` unchanged; `SDL_gfx`/`rotozoom` same author; window+texture
→ `SDL_SetVideoMode` surface + software scale). **Fallback:** bundle a cross-built SDL2 (uncertain
fbdev/KMSDRM backend) if the backport stalls.

> Net effect: the hard part shifts from "cross-build SDL2" to a contained **SDL2→1.2 graphics-layer
> backport** — and the dependency task (03) becomes nearly empty.

---

## 4. Phases → tasks

Each links to a self-contained file in [tasks/](tasks/). Ordered; later tasks depend on earlier.

| # | Task | Gate / done-when |
|---|---|---|
| 00 | [Environment & toolchain](tasks/00-environment-and-toolchain.md) | `docker compose run --rm shell` opens; `$CC` works |
| 01 | [Source & engine selection](tasks/01-source-and-engine-selection.md) | Fork + SDL version + deps + license decided |
| 02 | [Hello-world pipeline test](tasks/02-hello-world-pipeline-test.md) | `hello.log` appears on device |
| 03 | [Dependencies](tasks/03-dependencies.md) | Any missing libs built for the device |
| 04 | [Build the engine](tasks/04-build-engine.md) | `EliteTNK` ARM/uClibc binary produced |
| 05 | [Display & scaling](tasks/05-display-and-scaling.md) | Renders correctly at 640×480 |
| 06 | [Input mapping](tasks/06-input-mapping.md) | All Elite actions reachable from RG35XX buttons |
| 07 | [Audio](tasks/07-audio.md) | Sound works, or cleanly stubbed |
| 08 | [Packaging & deploy](tasks/08-packaging-and-deploy.md) | Launches from Garlic → Ports menu |
| 09 | [Testing](tasks/09-testing.md) | Test matrix passes (build/ABI/smoke/perf/input/memory/save) |
| 10 | [Polish (optional)](tasks/10-polish-optional.md) | Saves persist, sleep/wake OK, docs updated |

**Critical path:** 00 → 01 → 02 → 04 → 05 → 06 → 08 → 09. (03 only if audio/SDL2 needed; 07/10 parallel-able.)

---

## 5. Testing strategy (summary — full plan in task 09)

Test continuously, not just at the end:
- **Build/ABI test** after every compile: `readelf` confirms armv5te + uClibc interpreter.
- **Hello-world smoke** (task 02) proves the toolchain→package→run loop before the real build.
- **On-device smoke** after the engine builds: does it boot to the title/launch screen?
- **Input test:** every documented Elite key reachable via the mapped gamepad scheme.
- **Performance test:** playable FPS in combat/witchspace (worst case), logged.
- **Memory test:** RSS via `/proc/<pid>/status` stays well under 256MB.
- **Save/load test:** commander save survives quit + sleep/wake.

---

## 6. Risks

1. **SDL version mismatch** — if only SDL2 forks are usable, we must cross-build+bundle SDL2 (task 03).
2. **Resolution/scaling** — newkind's fixed internal res may not map cleanly to 640×480 (task 05).
3. **Controls** — Elite is key-heavy; mapping to a D-pad + 6-ish buttons needs a modifier/menu scheme (task 06).
4. **uClibc quirks** — old C code may hit missing/renamed libc symbols; patch per build error (task 04).
5. **Legal** — keep it personal/non-distributed; verify data-file freedom before committing anything (task 01).

---

## 7. Definition of done

Elite: The New Kind launches from the GarlicOS Ports menu on the original RG35XX, renders at
640×480, is fully playable with a documented RG35XX control scheme, holds a stable framerate,
stays within RAM, and saves/loads a commander across sleep/wake — with the whole build
reproducible via `docker compose` from a clean checkout.
