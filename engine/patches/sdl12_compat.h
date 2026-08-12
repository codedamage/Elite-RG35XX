/* sdl12_compat.h - SDL2 -> SDL 1.2 shim for the newkind Elite port (RG35XX).
 * WIP. Direct mappings live here; stateful wrappers are in sdl12_compat.c.
 * See docs/BACKPORT-SDL12.md.
 *
 * Usage: force-include ahead of the fork sources (gcc -include sdl12_compat.h),
 * drop the bundled SDL2 gfx and rotozoom sources, link -lSDL -lSDL_gfx.
 * (Note: no star-slash in this comment on purpose - it would close it early.)
 */
#ifndef SDL12_COMPAT_H
#define SDL12_COMPAT_H

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "SDL.h"                 /* SDL 1.2 */
#include "SDL_gfxPrimitives.h"   /* SDL 1.2 SDL_gfx - same prim names as SDL2_gfx */
#include "SDL_rotozoom.h"        /* SDL 1.2 rotozoom - same names/signatures */

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
#define SDL_RENDERER_SOFTWARE       0
#define SDL_RENDERER_ACCELERATED    0
#define SDL_RENDERER_PRESENTVSYNC   0
#define SDL_RENDERER_TARGETTEXTURE  0

#ifndef SDLK_STOP
#define SDLK_STOP  SDLK_UNKNOWN
#endif

/* ARGB8888 channel masks (little-endian pixel 0xAARRGGBB) - used by the shim .c */
#define SHIM_AMASK 0xFF000000u
#define SHIM_RMASK 0x00FF0000u
#define SHIM_GMASK 0x0000FF00u
#define SHIM_BMASK 0x000000FFu

/* --- No-op / trivially-mapped SDL2 window & misc calls --- */
#define SDL_DestroyWindow(w)        ((void)0)
#define SDL_DestroyRenderer(r)      ((void)0)
#define SDL_RaiseWindow(w)          ((void)0)
#define SDL_SetWindowIcon(w,s)      ((void)0)
#define SDL_SetWindowTitle(w,t)     ((void)0)
#define SDL_GetScancodeName(s)      ("")
#define SDL_StartTextInput()        ((void)0)
#define SDL_StopTextInput()         ((void)0)
/* clip rect applies to the current render target */
#define SDL_RenderSetClipRect(r,rc) SDL_SetClipRect(SDLc_cur(), (rc))
#define SDL_GetPrefPath(org,app)    SDLc_GetPrefPath((org),(app))

/* SDL_AllocFormat/SDL_FreeFormat: no direct 1.2 equivalent -> shim (impl in .c) */
SDL_PixelFormat *SDLc_AllocFormat(Uint32 fmt);
void             SDLc_FreeFormat(SDL_PixelFormat *f);
#define SDL_AllocFormat  SDLc_AllocFormat
#define SDL_FreeFormat   SDLc_FreeFormat

/* SDL_QueryTexture: assign straight into the caller's w/h (handles Uint16* or int*) */
#define SDL_QueryTexture(t,fmt,acc,wp,hp) \
        ( (void)(fmt),(void)(acc), (*(wp)=(t)->w), (*(hp)=(t)->h), 0 )

/* --- Blend modes: enum kept for source compatibility (informational) --- */
typedef enum { SDL_BLENDMODE_NONE=0, SDL_BLENDMODE_BLEND=1,
               SDL_BLENDMODE_ADD=2, SDL_BLENDMODE_MOD=4 } SDL_BlendMode;

/* --- Audio: SDL2 device API -> SDL 1.2 --- */
#define SDL_OpenAudioDevice(dev,iscap,want,have,allowed) \
        (SDL_OpenAudio((want),(have)) == 0 ? 1 : 0)   /* fake devid 1 on success */
#define SDL_PauseAudioDevice(dev,pause) SDL_PauseAudio(pause)
#define SDL_CloseAudioDevice(dev)       SDL_CloseAudio()

/* --- SDL2 math helpers -> libc (guard each; SDL 1.2 already defines SDL_memset) --- */
#ifndef SDL_sqrt
#define SDL_sqrt(x)    sqrt(x)
#endif
#ifndef SDL_cos
#define SDL_cos(x)     cos(x)
#endif
#ifndef SDL_sin
#define SDL_sin(x)     sin(x)
#endif
#ifndef SDL_atan
#define SDL_atan(x)    atan(x)
#endif
#ifndef SDL_atan2
#define SDL_atan2(y,x) atan2((y),(x))
#endif
#ifndef SDL_fabs
#define SDL_fabs(x)    fabs(x)
#endif

/* --- Keyboard state: SDL2 name -> 1.2 --- */
#define SDL_GetKeyboardState(n) SDL_GetKeyState(n)

/* --- Input: translate the RG35XX joystick into the keyboard events the game
 *     already understands (impl in .c). The game maps SDLK_* -> KEY_* itself. --- */
int SDLc_PollEvent(SDL_Event *ev);
#define SDL_PollEvent SDLc_PollEvent

/* --- Message box -> stderr/log (no messagebox in 1.2) --- */
#define SDL_ShowSimpleMessageBox(flags,title,msg,win) \
        (fprintf(stderr, "[%s] %s\n", (title), (msg)), 0)

/* ======================================================================
 * Stateful wrappers - implemented in sdl12_compat.c
 * ====================================================================== */
SDL_Surface *SDLc_cur(void);                       /* current render target */
const char  *SDLc_GetPrefPath(const char *org,const char *app);
SDL_Window  *SDLc_CreateWindow(const char *title,int x,int y,int w,int h,Uint32 flags);
SDL_Renderer*SDLc_CreateRenderer(SDL_Window *win,int index,Uint32 flags);
int   SDLc_RenderSetLogicalSize(SDL_Renderer *r,int w,int h);
SDL_Texture *SDLc_CreateTexture(SDL_Renderer *r,Uint32 fmt,int access,int w,int h);
SDL_Texture *SDLc_CreateTextureFromSurface(SDL_Renderer *r,SDL_Surface *s);
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
#define SDL_DestroyTexture            SDLc_DestroyTexture
#define SDL_SetRenderTarget           SDLc_SetRenderTarget
#define SDL_SetRenderDrawColor        SDLc_SetRenderDrawColor
#define SDL_SetRenderDrawBlendMode    SDLc_SetRenderDrawBlendMode
#define SDL_RenderClear               SDLc_RenderClear
#define SDL_RenderCopy                SDLc_RenderCopy
#define SDL_RenderDrawLine            SDLc_RenderDrawLine
#define SDL_RenderDrawPoint           SDLc_RenderDrawPoint
#define SDL_RenderPresent             SDLc_RenderPresent

#endif /* SDL12_COMPAT_H */
