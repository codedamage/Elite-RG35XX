# SDL2 → SDL 1.2 Backport Design (task 04)

Base: `lgblgblgb/newkind` (SDL2). Target: link the device's on-board **SDL 1.2 + SDL_gfx**.
This doc maps the **actual** SDL2 API surface used by the fork (measured by grepping the source)
to SDL 1.2 equivalents, and defines the shim strategy.

## Strategy: a compatibility shim, not a rewrite

The SDL2 usage is thin and mostly mechanical. We add:
- **`sdl12_compat.h`** — macros / inline wrappers for the direct 1:1 mappings.
- **`sdl12_compat.c`** — the few **stateful** wrappers (current render target, current draw color,
  logical-size scaling).
- **Build change:** drop the bundled `SDL2_gfxPrimitives.c` / `SDL2_rotozoom.c`; instead include the
  device's **SDL 1.2 `SDL_gfx`** headers and link `-lSDL_gfx`. The primitive/rotozoom function names
  and signatures are identical (same author) — they already take `SDL_Surface*`.

Key trick: **`SDL_Renderer` and `SDL_Texture` are both modelled as `SDL_Surface`** in the shim.
Because SDL 1.2 `SDL_gfxPrimitives` take an `SDL_Surface*` as their first arg, all the
`lineColor(renderer, …)` / `filledPolygonColor(renderer, …)` call sites compile unchanged once
`renderer` is a surface.

## API mapping (grounded in measured usage)

### Renderer / window / present
| SDL2 (used) | Count | SDL 1.2 mapping |
|---|---|---|
| `SDL_CreateWindow` | 1 | store W/H; real mode set in CreateRenderer |
| `SDL_CreateRenderer` | 1 | `SDL_SetVideoMode(W,H,16/32,SDL_SWSURFACE)` → screen surface (stateful) |
| `SDL_RenderPresent` | 2 | `SDL_Flip(screen)` |
| `SDL_RenderClear` | 3 | `SDL_FillRect(target, NULL, drawcolor)` |
| `SDL_RenderSetLogicalSize` | 2 | render into a logical-size surface, `zoomSurface` → screen on present (stateful) |

### Textures / render targets
| SDL2 | Count | SDL 1.2 mapping |
|---|---|---|
| `SDL_CreateTexture` | 5 | `SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,…)` |
| `SDL_CreateTextureFromSurface` | 4 | return the surface (add per-surface alpha as needed) |
| `SDL_SetRenderTarget` | 3 | set `g_target = surface` (NULL ⇒ screen) — stateful |
| `SDL_RenderCopy` | 9 | `SDL_BlitSurface(src,srcR,g_target,dstR)`; if sizes differ, `zoomSurface` first |
| `SDL_QueryTexture` | 2 | return `surface->w/h` |
| `SDL_DestroyTexture` | 5 | `SDL_FreeSurface` |

### Draw state / primitives
| SDL2 | Count | SDL 1.2 mapping |
|---|---|---|
| `SDL_SetRenderDrawColor` | 15 | store current RGBA (stateful) |
| `SDL_SetRenderDrawBlendMode` | 13 | store blend flag; alpha carried in gfx color / `SDL_SetAlpha` |
| `SDL_RenderDrawLine` | 6 | `lineColor(g_target,x1,y1,x2,y2,rgba)` (SDL_gfx) |
| `SDL_RenderDrawPoint` | 2 | `pixelColor(g_target,x,y,rgba)` |
| `*Color` gfx prims (`lineColor`,`filledPolygonColor`,`aapolygonColor`,`circleColor`,…) | ~ | **unchanged** — link SDL 1.2 `SDL_gfx` |
| `zoomSurface`,`rotozoomSurface*` | ~ | **unchanged** — SDL 1.2 `SDL_rotozoom` |

### Surfaces (already 1.2-compatible)
`SDL_CreateRGBSurface` (13), `SDL_BlitSurface` (4), `SDL_LoadBMP` (4), `SDL_FreeSurface` (10),
`SDL_LockSurface`/`SDL_UnlockSurface`, `SDL_MUSTLOCK`, `SDL_Rect`, `SDL_SWSURFACE` — **no change**.

### Audio (SDL2 device API → 1.2)
| SDL2 | Count | SDL 1.2 |
|---|---|---|
| `SDL_OpenAudio` | 1 | same |
| `SDL_OpenAudioDevice` | — | `SDL_OpenAudio` |
| `SDL_PauseAudioDevice` | 2 | `SDL_PauseAudio` |
| `SDL_CloseAudioDevice` | 2 | `SDL_CloseAudio` |
Callback + `SDL_AudioSpec` are compatible. (Or compile-time stub — task 07.)

### Input / misc
| SDL2 | SDL 1.2 |
|---|---|
| `event.key.keysym.sym`, `SDLK_*` | mostly identical; verify a few renamed keysyms |
| key-state array | `SDL_GetKeyState()` (1.2) vs `SDL_GetKeyboardState()` |
| `SDL_StartTextInput` (1) | no-op |
| `SDL_ShowSimpleMessageBox` (1), `SDL_MESSAGEBOX*` | replace with `fprintf(stderr,…)`/log |
| `SDL_sqrt/cos/atan/fabs/memset` | libc `<math.h>`/`<string.h>` (macro) |
| `SDL_BYTEORDER`, `SDL_LIL_ENDIAN` | present in 1.2 |

## Risks / watch-items
1. **Render-to-texture + scaling** (`SetRenderTarget`+`RenderCopy`) is the fiddliest — get the
   target-surface state machine right, and only `zoomSurface` when src≠dst size (perf).
2. **Blend modes / alpha:** SDL_gfx primitives honor the alpha byte in RGBA; texture alpha via
   `SDL_SetAlpha(SDL_SRCALPHA)`. Verify translucent HUD/effects look right.
3. **Pixel format / endianness:** pick a 16- or 32-bit screen; keep RGBA masks consistent with the
   gfx color convention (`0xRRGGBBAA`).
4. **Keysym deltas:** a handful of SDL2 keycodes renamed — check `keyboard.c` against SDL 1.2.
5. **Perf:** software scaling every frame is the main cost — prefer setting the video mode to the
   native render size (640×480) and avoid per-frame `zoomSurface` where possible (ties into task 05).

## Definition of done (task 04)
`EliteTNK` compiles against SDL 1.2 + SDL_gfx (no SDL2 anywhere in `readelf -d`), passes `check-abi.sh`,
and the render/audio/input paths are wired through the shim. First on-device boot is validated in task 05.
