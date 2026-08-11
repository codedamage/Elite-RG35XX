/* sdl12_compat.h — SDL2 -> SDL 1.2 shim for the newkind Elite port (RG35XX).
 * STARTER SCAFFOLD (WIP): direct 1:1 mappings are done here; stateful wrappers are
 * declared here and implemented in sdl12_compat.c. See docs/BACKPORT-SDL12.md.
 *
 * Usage: force-include this ahead of the fork's sources (gcc -include sdl12_compat.h),
 * drop the bundled SDL2_gfx*/rotozoom* sources, and link -lSDL -lSDL_gfx.
 */
#ifndef SDL12_COMPAT_H
#define SDL12_COMPAT_H

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "SDL.h"                 /* SDL 1.2 */
#include "SDL_gfxPrimitives.h"   /* SDL 1.2 SDL_gfx — same prim names as SDL2_gfx */
#include "SDL_rotozoom.h"        /* SDL 1.2 rotozoom — same names/signatures */

/* --- Model renderer & texture as surfaces (see design doc) --- */
typedef SDL_Surface SDL_Renderer;
typedef SDL_Surface SDL_Texture;
typedef struct SDL_Window SDL_Window;   /* opaque; only size is used */
typedef int SDL_Keycode;
typedef int SDL_AudioDeviceID;

/* --- SDL2 enums/flags the fork uses -> harmless constants under 1.2 --- */
#define SDL_WINDOWPOS_UNDEFINED     0
#define SDL_WINDOW_SHOWN            0
#define SDL_WINDOW_RESIZABLE        0
#define SDL_WINDOW_INPUT_FOCUS      0
#define SDL_WINDOW_FULLSCREEN       0
#define SDL_TEXTUREACCESS_STATIC    0
#define SDL_TEXTUREACCESS_STREAMING 1
#define SDL_TEXTUREACCESS_TARGET    2
#define SDL_PIXELFORMAT_ARGB8888    0    /* sentinel; shim always builds ARGB8888 */
#define SDL_DestroyWindow(w)        ((void)0)
#define SDL_DestroyRenderer(r)      ((void)0)

/* ARGB8888 channel masks (little-endian pixel 0xAARRGGBB) — used by the shim .c */
#define SHIM_AMASK 0xFF000000u
#define SHIM_RMASK 0x00FF0000u
#define SHIM_GMASK 0x0000FF00u
#define SHIM_BMASK 0x000000FFu

/* SDL_AllocFormat/SDL_FreeFormat: no direct 1.2 equivalent -> shim (impl in .c) */
SDL_PixelFormat *SDLc_AllocFormat(Uint32 fmt);
void             SDLc_FreeFormat(SDL_PixelFormat *f);
#define SDL_AllocFormat  SDLc_AllocFormat
#define SDL_FreeFormat   SDLc_FreeFormat

/* --- Blend modes: enum kept for source compatibility (mostly informational) --- */
typedef enum { SDL_BLENDMODE_NONE=0, SDL_BLENDMODE_BLEND=1,
               SDL_BLENDMODE_ADD=2, SDL_BLENDMODE_MOD=4 } SDL_BlendMode;

/* --- Audio: SDL2 device API -> SDL 1.2 --- */
#define SDL_OpenAudioDevice(dev,iscap,want,have,allowed) \
        (SDL_OpenAudio((want),(have)) == 0 ? 1 : 0)   /* returns fake devid 1 on success */
#define SDL_PauseAudioDevice(dev,pause) SDL_PauseAudio(pause)
#define SDL_CloseAudioDevice(dev)       SDL_CloseAudio()

/* --- Text input: no-op in 1.2 --- */
#define SDL_StartTextInput()   ((void)0)
#define SDL_StopTextInput()    ((void)0)

/* --- Message box -> stderr/log (no messagebox in 1.2) --- */
#define SDL_ShowSimpleMessageBox(flags,title,msg,win) \
        (fprintf(stderr, "[%s] %s\n", (title), (msg)), 0)

/* --- SDL2 math helpers -> libc --- */
#ifndef SDL_sqrt
#define SDL_sqrt(x)   sqrt(x)
#define SDL_cos(x)    cos(x)
#define SDL_sin(x)    sin(x)
#define SDL_atan(x)   atan(x)
#define SDL_atan2(y,x) atan2((y),(x))
#define SDL_fabs(x)   fabs(x)
#define SDL_memset(p,v,n) memset((p),(v),(n))
#endif

/* --- Keyboard state: SDL2 name -> 1.2 --- */
#define SDL_GetKeyboardState(n) SDL_GetKeyState(n)

/* ======================================================================
 * Stateful wrappers — implemented in sdl12_compat.c
 * (current render target, draw color, logical scaling).
 * Signatures mirror the SDL2 prototypes the fork calls.
 * ====================================================================== */
SDL_Window  *SDLc_CreateWindow(const char *title,int x,int y,int w,int h,Uint32 flags);
SDL_Renderer*SDLc_CreateRenderer(SDL_Window *win,int index,Uint32 flags);
int   SDLc_RenderSetLogicalSize(SDL_Renderer *r,int w,int h);
SDL_Texture *SDLc_CreateTexture(SDL_Renderer *r,Uint32 fmt,int access,int w,int h);
SDL_Texture *SDLc_CreateTextureFromSurface(SDL_Renderer *r,SDL_Surface *s);
int   SDLc_QueryTexture(SDL_Texture *t,Uint32 *fmt,int *access,int *w,int *h);
void  SDLc_DestroyTexture(SDL_Texture *t);
int   SDLc_SetRenderTarget(SDL_Renderer *r,SDL_Texture *t);
int   SDLc_SetRenderDrawColor(SDL_Renderer *r,Uint8 rr,Uint8 g,Uint8 b,Uint8 a);
int   SDLc_SetRenderDrawBlendMode(SDL_Renderer *r,SDL_BlendMode m);
int   SDLc_RenderClear(SDL_Renderer *r);
int   SDLc_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_Rect *dst);
int   SDLc_RenderDrawLine(SDL_Renderer *r,int x1,int y1,int x2,int y2);
int   SDLc_RenderDrawPoint(SDL_Renderer *r,int x,int y);
void  SDLc_RenderPresent(SDL_Renderer *r);

/* Redirect the SDL2 names the fork uses to our wrappers. */
#define SDL_CreateWindow              SDLc_CreateWindow
#define SDL_CreateRenderer            SDLc_CreateRenderer
#define SDL_RenderSetLogicalSize      SDLc_RenderSetLogicalSize
#define SDL_CreateTexture             SDLc_CreateTexture
#define SDL_CreateTextureFromSurface  SDLc_CreateTextureFromSurface
#define SDL_QueryTexture              SDLc_QueryTexture
#define SDL_DestroyTexture            SDLc_DestroyTexture
#define SDL_SetRenderTarget           SDLc_SetRenderTarget
#define SDL_SetRenderDrawColor        SDLc_SetRenderDrawColor
#define SDL_SetRenderDrawBlendMode    SDLc_SetRenderDrawBlendMode
#define SDL_RenderClear               SDLc_RenderClear
#define SDL_RenderCopy                SDLc_RenderCopy
#define SDL_RenderDrawLine            SDLc_RenderDrawLine
#define SDL_RenderDrawPoint           SDLc_RenderDrawPoint
#define SDL_RenderPresent             SDLc_RenderPresent

/* NOTE (WIP): SDL_DestroyWindow, window-event handling, and any SDL2 enum/flags not
 * covered above will surface as compile errors during task 04 — handle each here. */

#endif /* SDL12_COMPAT_H */
