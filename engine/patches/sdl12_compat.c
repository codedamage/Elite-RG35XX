/* sdl12_compat.c — stateful half of the SDL2->1.2 shim. STARTER SCAFFOLD (WIP).
 * Implements the render-target / draw-color / present state machine.
 * See docs/BACKPORT-SDL12.md. Expect to iterate this during task 04.
 */
#include "sdl12_compat.h"
#include <stdlib.h>
#undef SDL_PollEvent   /* call the REAL SDL 1.2 SDL_PollEvent inside our wrapper */

#ifdef SIM_BUILD
static long g_sim_frame = 0;   /* headless simulator: frame counter for capture + scripted input */
#endif

/* --- shim global state --- */
static SDL_Surface *g_screen  = NULL;   /* the real video surface (SDL_SetVideoMode) */
static SDL_Surface *g_back    = NULL;   /* offscreen backbuffer = the "renderer" AND the target texture */
static SDL_Surface *g_target  = NULL;   /* current render target (NULL => present to screen)             */
static int   g_logical_w = 0, g_logical_h = 0;
static Uint8 g_r=0, g_g=0, g_b=0, g_a=255;

static Uint32 rgba(void){ return ((Uint32)g_r<<24)|((Uint32)g_g<<16)|((Uint32)g_b<<8)|g_a; }
static SDL_Surface *cur(void){ return g_target ? g_target : g_screen; }

/* public accessor (used by the SDL_RenderSetClipRect macro) */
SDL_Surface *SDLc_cur(void){ return cur(); }

/* SDL_GetPrefPath -> the game folder (launcher cd's here); saves stay local.
 * Return a freeable copy: SDL2 callers may SDL_free() the result. */
const char *SDLc_GetPrefPath(const char *org,const char *app){
    (void)org;(void)app; return strdup("./");
}

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
    /* GarlicOS is fullscreen 640x480. Its fbdev is usually 16bpp (RGB565), so try
     * 32 -> 16 -> auto so we don't fail on panels that reject 32bpp. 32-bit ARGB
     * textures still blit fine onto a 16-bit screen (SDL converts). */
    Uint32 flags = SDL_SWSURFACE | SDL_FULLSCREEN;
    g_screen = SDL_SetVideoMode(640,480,32,flags);
    if(!g_screen) g_screen = SDL_SetVideoMode(640,480,16,flags);
    if(!g_screen) g_screen = SDL_SetVideoMode(640,480,0, flags);
    if(!g_screen) fprintf(stderr,"SHIM: SDL_SetVideoMode(640x480) failed: %s\n", SDL_GetError());
    /* The "renderer" IS an offscreen backbuffer sized to the game's logical screen
     * (set by CreateWindow). ALL drawing primitives take this as sdl_ren, so they
     * draw here; the target texture aliases it; present scales it onto g_screen. */
    int bw = g_logical_w>0 ? g_logical_w : 640;
    int bh = g_logical_h>0 ? g_logical_h : 480;
    g_back = SDL_CreateRGBSurface(SDL_SWSURFACE,bw,bh,32,
                                  SHIM_RMASK,SHIM_GMASK,SHIM_BMASK,SHIM_AMASK);
    fprintf(stderr,"SHIM: backbuffer %dx%d, screen %dx%d\n",bw,bh,
            g_screen?g_screen->w:0,g_screen?g_screen->h:0);
    return g_back;
}

int SDLc_RenderSetLogicalSize(SDL_Renderer *r,int w,int h){
    (void)r; g_logical_w=w; g_logical_h=h; return 0;  /* TODO: scale on present if != 640x480 */
}

SDL_Texture *SDLc_CreateTexture(SDL_Renderer *r,Uint32 fmt,int access,int w,int h){
    (void)r;(void)fmt;
    /* The game's main render-target texture aliases the backbuffer, so drawing to
     * "the target" and drawing via sdl_ren hit the same surface. Other textures
     * (sprites) are independent surfaces. */
    if(access==SDL_TEXTUREACCESS_TARGET && g_back) return g_back;
    return SDL_CreateRGBSurface(SDL_SWSURFACE,w,h,32,
                                SHIM_RMASK,SHIM_GMASK,SHIM_BMASK,SHIM_AMASK);
}

SDL_Texture *SDLc_CreateTextureFromSurface(SDL_Renderer *r,SDL_Surface *s){
    (void)r;
    return SDL_DisplayFormatAlpha(s);    /* owns a converted copy */
}

void SDLc_DestroyTexture(SDL_Texture *t){ if(t && t!=g_screen && t!=g_back) SDL_FreeSurface(t); }

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
#ifdef SIM_BUILD
    if(!dst && !src){   /* the full-screen present: dump the game's RAW render target */
        static long tc=0;
        if(tc==2||tc==90||tc==250){
            char p[80]; snprintf(p,sizeof p,"/src/sim/tex_%04ld.bmp",tc);
            SDL_SaveBMP(t,p);
            fprintf(stderr,"SIM: raw tex %s = %dx%d ; screen = %dx%d\n",p,t->w,t->h,d->w,d->h);
        }
        tc++;
    }
#endif
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
    static int logged=0;
    if(!logged && !dst){ logged=1;   /* the full-screen present copy */
        fprintf(stderr,"SHIM: present scale tex=%dx%d -> screen=%dx%d\n",
                sw,sh,dw,dh);
    }
    /* SMOOTHING_ON (bilinear): when downscaling 800x600 -> 640x480, nearest-neighbour
     * randomly drops 1px lines (view borders, HUD rules); bilinear keeps them (dimmed)
     * and looks consistent. */
    SDL_Surface *z = zoomSurface(t, (double)dw/sw, (double)dh/sh, SMOOTHING_ON);
    if(!z) return -1;
    SDL_SetAlpha(z, 0, 255);                 /* opaque copy: don't alpha-blend onto screen */
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
    SDL_Flip(g_screen);
#ifdef SIM_BUILD
    if(g_screen){
        long f = g_sim_frame;
        if(f==2||f==60||f==120||f==160||f==220||f==280||f==340||f==400){
            char p[80]; snprintf(p,sizeof p,"/src/sim/frame_%04ld.bmp",f);
            SDL_SaveBMP(g_screen,p);
            fprintf(stderr,"SIM: saved %s (screen %dx%d)\n",p,g_screen->w,g_screen->h);
        }
        if(f>440){ fprintf(stderr,"SIM: done.\n"); exit(0); }
    }
    g_sim_frame++;
#endif
}

/* =====================================================================
 * Input: RG35XX joystick -> synthetic keyboard events.
 * The game's handle_sdl_events() maps SDLK_* -> KEY_* via its own table,
 * so we just feed it SDL_KEYDOWN/UP with the right keysym.sym.
 * D-pad (hat + axes) is high-confidence; face/shoulder buttons are a
 * PROVISIONAL map and every button index is logged to refine later.
 * ===================================================================== */
static SDL_Joystick *s_joy = NULL;
static int  s_joy_init = 0;
static int  s_up=0,s_dn=0,s_lf=0,s_rt=0;   /* current logical d-pad state */
static int  s_btn[32];                      /* button hold states (for quit combo) */

/* tiny queue of pending synthetic events */
#define SQN 32
static SDL_Event s_q[SQN]; static int s_qh=0,s_qt=0;
static void q_push_key(int down,int sym){
    SDL_Event e; memset(&e,0,sizeof e);
    e.type = down?SDL_KEYDOWN:SDL_KEYUP;
    e.key.state = down?SDL_PRESSED:SDL_RELEASED;
    e.key.keysym.sym = (SDLKey)sym;
    s_q[s_qt]=e; s_qt=(s_qt+1)%SQN;
}
static int q_pop(SDL_Event *ev){ if(s_qh==s_qt) return 0; *ev=s_q[s_qh]; s_qh=(s_qh+1)%SQN; return 1; }
static void edge(int *state,int now,int sym){
    if(now && !*state){ *state=1; q_push_key(1,sym); }
    else if(!now && *state){ *state=0; q_push_key(0,sym); }
}

/* PROVISIONAL button->key map (verify indices from the JOYBTN log lines). */
static int btn_sym(int b){
    switch(b){
        case 0: return SDLK_a;      /* A  -> fire            */
        case 1: return SDLK_SPACE;  /* B  -> accelerate      */
        case 2: return SDLK_SLASH;  /* X  -> decelerate      */
        case 3: return SDLK_j;      /* Y  -> in-system jump  */
        case 4: return SDLK_SLASH;  /* L1 -> decelerate      */
        case 5: return SDLK_SPACE;  /* R1 -> accelerate      */
        case 6: return SDLK_e;      /* L2 -> ECM             */
        case 7: return SDLK_h;      /* R2 -> hyperspace      */
        case 8: return SDLK_TAB;    /* Select -> energy bomb */
        case 9: return SDLK_RETURN; /* Start  -> launch/OK   */
        default: return 0;
    }
}

static void ensure_joy(void){
    if(s_joy_init) return;
    s_joy_init=1;
    if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)==0){
        if(SDL_NumJoysticks()>0) s_joy=SDL_JoystickOpen(0);
        SDL_JoystickEventState(SDL_ENABLE);
        fprintf(stderr,"SHIM: joystick init count=%d opened=%p\n",
                SDL_NumJoysticks(),(void*)s_joy);
    } else {
        fprintf(stderr,"SHIM: SDL_INIT_JOYSTICK failed: %s\n", SDL_GetError());
    }
}

int SDLc_PollEvent(SDL_Event *ev){
#ifdef SIM_BUILD
    /* Headless sim: feed a scripted key sequence to advance intro & try flight. */
    {
        static int si=0;
        /* Hold each key for MANY frames (like a human press) so the game's uneven
         * poll cadence can't swallow a down+up in one sweep. */
        static const struct { long f; int sym; int down; } script[] = {
            { 40,SDLK_n,1},    {130,SDLK_n,0},      /* intro1: N = new commander -> break */
            {170,SDLK_SPACE,1},{250,SDLK_SPACE,0},  /* intro2: SPACE -> start game        */
            {300,SDLK_F1,1},   {430,SDLK_F1,0},     /* docked: hold F1 long -> launch      */
            {-1,0,0}
        };
        if(script[si].f>=0 && g_sim_frame>=script[si].f){
            ev->type = script[si].down?SDL_KEYDOWN:SDL_KEYUP;
            ev->key.state = script[si].down?SDL_PRESSED:SDL_RELEASED;
            ev->key.keysym.sym=(SDLKey)script[si].sym;
            fprintf(stderr,"SIM-INJECT sym=%d down=%d @frame=%ld\n",
                    script[si].sym,script[si].down,g_sim_frame);
            si++;
            return 1;
        }
        return 0;   /* no real input device in sim */
    }
#endif
    ensure_joy();
    if(q_pop(ev)) return 1;                 /* drain synthetic first */

    SDL_Event e;
    while(SDL_PollEvent(&e)){
        switch(e.type){
            case SDL_JOYHATMOTION:{
                int v=e.jhat.value;
                edge(&s_up, v&SDL_HAT_UP,    SDLK_UP);
                edge(&s_dn, v&SDL_HAT_DOWN,  SDLK_DOWN);
                edge(&s_lf, v&SDL_HAT_LEFT,  SDLK_LEFT);
                edge(&s_rt, v&SDL_HAT_RIGHT, SDLK_RIGHT);
                if(q_pop(ev)) return 1;
                continue;
            }
            case SDL_JOYAXISMOTION:{
                int a=e.jaxis.axis, val=e.jaxis.value;
                if(a==0){ edge(&s_lf,val<-12000,SDLK_LEFT); edge(&s_rt,val>12000,SDLK_RIGHT); }
                else if(a==1){ edge(&s_up,val<-12000,SDLK_UP); edge(&s_dn,val>12000,SDLK_DOWN); }
                if(q_pop(ev)) return 1;
                continue;
            }
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:{
                int down=(e.type==SDL_JOYBUTTONDOWN), b=e.jbutton.button;
                fprintf(stderr,"JOYBTN #%d %s\n", b, down?"down":"up");
                if(b>=0 && b<32) s_btn[b]=down;
                if(s_btn[8] && s_btn[9]){ ev->type=SDL_QUIT; return 1; } /* Select+Start = quit */
                int sym=btn_sym(b);
                if(sym){
                    ev->type = down?SDL_KEYDOWN:SDL_KEYUP;
                    ev->key.state = down?SDL_PRESSED:SDL_RELEASED;
                    ev->key.keysym.sym = (SDLKey)sym;
                    return 1;
                }
                continue;                    /* unmapped button: swallow, keep polling */
            }
            default:
                *ev=e; return 1;             /* pass keyboard/quit/etc. through */
        }
    }
    return q_pop(ev);                        /* any late synthetic events */
}
