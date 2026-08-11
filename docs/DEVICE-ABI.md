# Device ABI & Toolchain (original RG35XX / GarlicOS)

Carried over from the EverlastingSummer port project, where it was confirmed by inspecting
the device's own RetroArch binary with `readelf`.

## Hardware
| | |
|---|---|
| SoC | Actions **ATM7039S**, quad **ARM Cortex-A9** ~1.3GHz |
| RAM | **256 MB** DDR3 (shared) |
| GPU | **PowerVR SGX544** (OpenGL **ES 2.0** only — no desktop GL) |
| Screen | 3.5" IPS **640×480** (4:3) |
| Wireless | none (all transfer via SD card on a PC) |
| CFW | **GarlicOS** (RetroArch frontend; standalone `.sh` ports also run) |

## ABI (confirmed from stock binary)
| Property | Value |
|---|---|
| libc | **uClibc** (`/lib/ld-uClibc.so.0`) |
| Toolchain | **`arm-miyoo-linux-uclibcgnueabi`** (MiyooCFW) |
| Base ISA | **ARMv5TE** (ARM926EJ-S target) |
| Float ABI | **soft-float (gnueabi)** — VFPv3 instructions used, floats passed in core regs (NOT `gnueabihf`) |

**Build rule:** use the MiyooCFW toolchain **defaults** (`-O2` only). Do NOT force
`-march=armv7`/`-mfloat-abi=hard` — it breaks ABI compatibility with on-device libs.

## Libraries already on the device (from RetroArch's NEEDED list)
`libSDL-1.2.so.0`, `libSDL_ttf-2.0.so.0`, `libSDL_image-1.2.so.0`, `libSDL_gfx.so.13`,
`libasound.so.2` (ALSA), `libz.so.1`, `libstdc++.so.6`, `libgcc_s.so.1`, `libc.so.0`.

→ **SDL 1.2 is on-device.** An SDL 1.2 build of newkind links against it directly; only
missing audio libs (SDL_mixer / ogg / vorbis) would need bundling.

## Toolchain access
Prebuilt Docker image: **`miyoocfw/toolchain-shared-uclibc:docker`** (arm-miyoo-linux-uclibcgnueabi
pre-configured, mounts project at `/src`). Runnable from Windows PowerShell with Docker Desktop —
no WSL required.
