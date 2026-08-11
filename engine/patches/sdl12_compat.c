/* sdl12_compat.c — stateful half of the SDL2->1.2 shim. STARTER SCAFFOLD (WIP).
 * Implements the render-target / draw-color / present state machine.
 * See docs/BACKPORT-SDL12.md. Expect to iterate this during task 04.
 */
#include "sdl12_compat.h"

/* --- shim global state --- */
static SDL_Surface *g_screen  = NULL;   /* the real video surface (SDL_SetVideoMode) */
static SDL_Surface *g_target  = NULL;   /* current render target (NULL => screen)    */
static int   g_logical_w = 0, g_logical_h = 0;
static Uint8 g_r=0, g_g=0, g_b=0, g_a=255;

static Uint32 rgba(void){ return ((Uint32)g_r<<24)|((Uint32)g_g<<16)|((Uint32)g_b<<8)|g_a; }
static SDL_Surface *cur(void){ return g_target ? g_target : g_screen; }

SDL_Window *SDLc_CreateWindow(const char *t,int x,int y,int w,int h,Uint32 f){
    (void)t;(void)x;(void)y;(void)f;
    g_logical_w=w; g_logical_h=h;
    return (SDL_Window*)1;               /* opaque handle; size tracked above */
}

SDL_Renderer *SDLc_CreateRenderer(SDL_Window *win,int idx,Uint32 f){
    (void)win;(void)idx;(void)f;
    /* GarlicOS wants fullscreen 640x480. Pick 640x480 to match the panel (task 05). */
    g_screen = SDL_SetVideoMode(640,480,32,SDL_SWSURFACE|SDL_FULLSCREEN);
    return g_screen;
}

int SDLc_RenderSetLogicalSize(SDL_Renderer *r,int w,int h){
    (void)r; g_logical_w=w; g_logical_h=h; return 0;  /* TODO: scale on present if != 640x480 */
}

SDL_Texture *SDLc_CreateTexture(SDL_Renderer *r,Uint32 fmt,int access,int w,int h){
    (void)r;(void)fmt;(void)access;
    return SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,32,
                                0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);
}

SDL_Texture *SDLc_CreateTextureFromSurface(SDL_Renderer *r,SDL_Surface *s){
    (void)r;
    return SDL_DisplayFormatAlpha(s);    /* owns a converted copy */
}

int SDLc_QueryTexture(SDL_Texture *t,Uint32 *fmt,int *acc,int *w,int *h){
    if(fmt)*fmt=0; if(acc)*acc=0; if(w)*w=t?t->w:0; if(h)*h=t?t->h:0; return 0;
}

void SDLc_DestroyTexture(SDL_Texture *t){ if(t && t!=g_screen) SDL_FreeSurface(t); }

int SDLc_SetRenderTarget(SDL_Renderer *r,SDL_Texture *t){ (void)r; g_target=t; return 0; }

int SDLc_SetRenderDrawColor(SDL_Renderer *r,Uint8 rr,Uint8 g,Uint8 b,Uint8 a){
    (void)r; g_r=rr; g_g=g; g_b=b; g_a=a; return 0;
}

int SDLc_SetRenderDrawBlendMode(SDL_Renderer *r,SDL_BlendMode m){
    (void)r;(void)m; return 0;           /* alpha handled via RGBA in gfx prims / SDL_SetAlpha */
}

int SDLc_RenderClear(SDL_Renderer *r){
    SDL_Surface *s=cur(); (void)r;
    return SDL_FillRect(s,NULL,SDL_MapRGBA(s->format,g_r,g_g,g_b,g_a));
}

int SDLc_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_Rect *dst){
    SDL_Surface *d=cur(); SDL_Rect s2,d2; (void)r;
    if(!t) return -1;
    if(src){s2=*src;} if(dst){d2=*dst;}
    /* TODO: if dst size != src size, zoomSurface(t, sx, sy, ...) then blit (task 04/05). */
    return SDL_BlitSurface(t, src?&s2:NULL, d, dst?&d2:NULL);
}

int SDLc_RenderDrawLine(SDL_Renderer *r,int x1,int y1,int x2,int y2){
    (void)r; return lineColor(cur(),x1,y1,x2,y2,rgba());
}
int SDLc_RenderDrawPoint(SDL_Renderer *r,int x,int y){
    (void)r; return pixelColor(cur(),x,y,rgba());
}

void SDLc_RenderPresent(SDL_Renderer *r){
    (void)r;
    /* TODO(task05): if we rendered at a logical size != 640x480, zoomSurface -> g_screen here. */
    SDL_Flip(g_screen);
}
