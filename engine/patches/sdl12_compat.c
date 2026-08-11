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

/* SDL_AllocFormat shim: 1.2 has no format allocator. Keep one ARGB8888 format alive
 * via a 1x1 holder surface and hand out its ->format. */
static SDL_Surface *s_fmt_holder = NULL;
SDL_PixelFormat *SDLc_AllocFormat(Uint32 fmt){
    (void)fmt;
    if(!s_fmt_holder)
        s_fmt_holder = SDL_CreateRGBSurface(SDL_SWSURFACE,1,1,32,
                        SHIM_RMASK,SHIM_GMASK,SHIM_BMASK,SHIM_AMASK);
    return s_fmt_holder ? s_fmt_holder->format : NULL;
}
void SDLc_FreeFormat(SDL_PixelFormat *f){ (void)f; /* holder released at SDL_Quit */ }

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
                                SHIM_RMASK,SHIM_GMASK,SHIM_BMASK,SHIM_AMASK);
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
    SDL_Surface *d=cur(); (void)r;
    if(!t) return -1;
    int sw = src ? src->w : t->w;
    int sh = src ? src->h : t->h;
    int dw = dst ? dst->w : d->w;
    int dh = dst ? dst->h : d->h;

    if(sw==dw && sh==dh){                       /* fast path: 1:1 blit */
        SDL_Rect s2, d2;
        if(src) s2=*src;
        if(dst) d2=*dst;
        return SDL_BlitSurface(t, src?&s2:NULL, d, dst?&d2:NULL);
    }
    /* scaled path (e.g. present: SCREEN_WxSCREEN_H texture -> 640x480 screen).
     * zoomSurface scales the whole surface; the fork's scaled copies use src=NULL. */
    SDL_Surface *z = zoomSurface(t, (double)dw/sw, (double)dh/sh, SMOOTHING_OFF);
    if(!z) return -1;
    SDL_Rect d2; d2.x = dst?dst->x:0; d2.y = dst?dst->y:0; d2.w=0; d2.h=0;
    int rc = SDL_BlitSurface(z, NULL, d, &d2);
    SDL_FreeSurface(z);
    return rc;
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
