/* sdl12_compat.c — stateful half of the SDL2->1.2 shim. STARTER SCAFFOLD (WIP).
 * Implements the render-target / draw-color / present state machine.
 * See docs/BACKPORT-SDL12.md. Expect to iterate this during task 04.
 */
#include "sdl12_compat.h"
#include <stdlib.h>
#include <signal.h>
#undef SDL_PollEvent   /* call the REAL SDL 1.2 SDL_PollEvent inside our wrapper */

#ifdef SIM_BUILD
static long g_sim_frame = 0;   /* headless simulator: frame counter for capture + scripted input */
#endif

/* ============================================================================
 * glibc-style ctype accessors that the device's (older) uClibc doesn't export.
 * The toolchain's <ctype.h> routes isalpha/isspace/toupper/... through these,
 * producing undefined symbols (__ctype_b_loc etc.) that fail to load on-device.
 * Define them IN the binary so nothing is imported from the device libc.
 * (Not needed for the x86 SIM build, whose glibc already provides them.)
 * ==========================================================================*/
#ifndef SIM_BUILD
/* glibc class bits (_ISbit): low bits high-byte, high bits low-byte. */
#define CB_UP 0x0100u /* upper */
#define CB_LO 0x0200u /* lower */
#define CB_AL 0x0400u /* alpha */
#define CB_DI 0x0800u /* digit */
#define CB_XD 0x1000u /* xdigit*/
#define CB_SP 0x2000u /* space */
#define CB_PR 0x4000u /* print */
#define CB_GR 0x8000u /* graph */
#define CB_BL 0x0001u /* blank */
#define CB_CN 0x0002u /* cntrl */
#define CB_PU 0x0004u /* punct */
#define CB_AN 0x0008u /* alnum */

static unsigned short   s_ctb[384];
static __ctype_touplow_t s_ctup[384];
static __ctype_touplow_t s_ctlo[384];
static const unsigned short   *s_ctb_p  = s_ctb  + 128;
static const __ctype_touplow_t *s_ctup_p = s_ctup + 128;
static const __ctype_touplow_t *s_ctlo_p = s_ctlo + 128;
static int s_ctype_ready = 0;

static void ctype_init(void){
    int i;
    if(s_ctype_ready) return;
    for(i=-128;i<256;i++){
        int idx=i+128; unsigned c=(unsigned)(i & 0xFF); unsigned short m=0;
        if(i>=0 && i<256){
            if(c>='A'&&c<='Z'){ m|=CB_UP|CB_AL|CB_AN|CB_PR|CB_GR; if(c<='F') m|=CB_XD; }
            else if(c>='a'&&c<='z'){ m|=CB_LO|CB_AL|CB_AN|CB_PR|CB_GR; if(c<='f') m|=CB_XD; }
            else if(c>='0'&&c<='9'){ m|=CB_DI|CB_XD|CB_AN|CB_PR|CB_GR; }
            else if(c==' '){ m|=CB_SP|CB_BL|CB_PR; }
            else if(c=='\t'){ m|=CB_SP|CB_BL|CB_CN; }
            else if(c=='\n'||c=='\v'||c=='\f'||c=='\r'){ m|=CB_SP|CB_CN; }
            else if(c<32||c==127){ m|=CB_CN; }
            else if(c<127){ m|=CB_PU|CB_PR|CB_GR; }
        }
        s_ctb[idx]=m;
        s_ctup[idx]=(i>=0&&c>='a'&&c<='z')?(int)(c-32):i;
        s_ctlo[idx]=(i>=0&&c>='A'&&c<='Z')?(int)(c+32):i;
    }
    s_ctype_ready=1;
}
const unsigned short    **__ctype_b_loc(void){       ctype_init(); return &s_ctb_p;  }
const __ctype_touplow_t **__ctype_toupper_loc(void){ ctype_init(); return &s_ctup_p; }
const __ctype_touplow_t **__ctype_tolower_loc(void){ ctype_init(); return &s_ctlo_p; }
#endif /* !SIM_BUILD */

/* --- shim global state --- */
static SDL_Surface *g_screen  = NULL;   /* the real video surface (SDL_SetVideoMode) */
static SDL_Surface *g_back    = NULL;   /* offscreen backbuffer = the "renderer" AND the target texture */
static SDL_Surface *g_target  = NULL;   /* current render target (NULL => present to screen)             */
static int   g_logical_w = 0, g_logical_h = 0;
static Uint8 g_r=0, g_g=0, g_b=0, g_a=255;
static long  g_pframe = 0;   /* present-frame tick (for input tap-hold timing) */

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
    /* Bilinear looks best but is far too slow on the Cortex-A9 per frame, so it's
     * sim-only; the device uses nearest-neighbour (much faster). */
#ifdef SIM_BUILD
    int smooth = SMOOTHING_ON;
#else
    int smooth = SMOOTHING_OFF;
#endif
    SDL_Surface *z = zoomSurface(t, (double)dw/sw, (double)dh/sh, smooth);
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
    g_pframe++;                 /* frame tick for tap-hold timing (both sim + device) */
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

/* Tap-hold: a momentary key (intro Y/N, F-keys) must stay DOWN across at least one
 * game poll, or the down+up get swallowed in a single handle_sdl_events sweep and
 * kbd_*_pressed never sees it. So hold taps a few present-frames before releasing. */
#define TAP_HOLD 3
static struct { int sym; long rel; int on; } s_taps[16];
static void schedule_up(int sym){
    int i; for(i=0;i<16;i++) if(!s_taps[i].on){ s_taps[i].on=1; s_taps[i].sym=sym; s_taps[i].rel=g_pframe+TAP_HOLD; return; }
}
static void drain_due_taps(void){
    int i; for(i=0;i<16;i++) if(s_taps[i].on && g_pframe>=s_taps[i].rel){ q_push_key(0,s_taps[i].sym); s_taps[i].on=0; }
}

/* ---- Confirmed RG35XX "RG35XX Gamepad" indices (from InputProbe) ----
 * D-pad = HAT 0 (U/R/D/L = 1/2/4/8);  A=0 B=1 X=2 Y=3;  R1=8;  Select=5;
 * L2/R2 = analog axes 2 / 5.  (L1/Start/Menu still logged for refinement.) */
#define BTN_A      0
#define BTN_B      1
#define BTN_X      2
#define BTN_Y      3
#define BTN_L1     5
#define BTN_R1     6
#define BTN_SELECT 7
#define BTN_START  8
#define BTN_MENU   9
#define AX_L2      2
#define AX_R2      5

static int s_sel=0, s_start=0; /* Select / Start held => modifier layers */
static int s_l2=0,s_r2=0;      /* analog-trigger latched key states       */
static int s_btn_key[32];      /* held key emitted per button (0=none)    */

static void q_tap(int sym){ q_push_key(1,sym); schedule_up(sym); }  /* held a few frames */

static int base_hold_sym(int b){    /* held flight actions */
    switch(b){
        case BTN_A:  return SDLK_a;      /* fire       */
        case BTN_B:  return SDLK_SPACE;  /* accelerate */
        case BTN_R1: return SDLK_SPACE;  /* accelerate */
        case BTN_L1: return SDLK_SLASH;  /* decelerate */
        default: return 0;
    }
}
static int base_tap_sym(int b){     /* one-shots */
    switch(b){
        case BTN_X:    return SDLK_n;   /* intro1: new commander (harmless in flight) */
        case BTN_Y:    return SDLK_y;   /* intro1: load commander                     */
        case BTN_MENU: return SDLK_p;   /* pause / resume                             */
        default: return 0;
    }
}
static int sel_sym(int b){          /* Select + button => screens (F5-F10) */
    switch(b){
        case BTN_A:  return SDLK_F5;  /* galactic chart */
        case BTN_B:  return SDLK_F6;  /* local chart    */
        case BTN_X:  return SDLK_F7;  /* system data    */
        case BTN_Y:  return SDLK_F8;  /* market         */
        case BTN_R1: return SDLK_F9;  /* status         */
        case BTN_L1: return SDLK_F10; /* inventory      */
        default: return 0;
    }
}
static int start_sym(int b){       /* Start + button => combat / utility */
    switch(b){
        case BTN_A:  return SDLK_m;   /* fire missile   */
        case BTN_B:  return SDLK_t;   /* target missile */
        case BTN_X:  return SDLK_u;   /* un-arm missile */
        case BTN_Y:  return SDLK_e;   /* ECM            */
        case BTN_R1: return SDLK_h;   /* hyperspace     */
        case BTN_L1: return SDLK_j;   /* in-system jump */
        default: return 0;
    }
}
/* d-pad: always held arrows (pitch/roll); Select adds F1-F4, Start adds util keys */
static void hat_dir(int *state,int now,int arrow,int sel_k,int start_k){
    if(now && !*state){ *state=1; q_push_key(1,arrow);
        if(s_sel && sel_k)        q_tap(sel_k);
        else if(s_start && start_k) q_tap(start_k);
    }
    else if(!now && *state){ *state=0; q_push_key(0,arrow); }
}

static void on_signal(int s){ (void)s; exit(0); }

static void ensure_joy(void){
    if(s_joy_init) return;
    s_joy_init=1;
    signal(SIGTERM,on_signal); signal(SIGINT,on_signal);  /* clean exit from Garlic */
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
    drain_due_taps();                       /* release any tap-held keys that are due */
    if(q_pop(ev)) return 1;                 /* drain synthetic first */

    SDL_Event e;
    while(SDL_PollEvent(&e)){
        switch(e.type){
            case SDL_JOYHATMOTION:{
                int v=e.jhat.value;
                /* args: arrow(base), Select-layer, Start-layer */
                hat_dir(&s_up, v&SDL_HAT_UP,    SDLK_UP,    SDLK_F1,  SDLK_TAB); /* front/launch ; energy bomb */
                hat_dir(&s_dn, v&SDL_HAT_DOWN,  SDLK_DOWN,  SDLK_F2,  SDLK_c);   /* rear view    ; docking comp */
                hat_dir(&s_lf, v&SDL_HAT_LEFT,  SDLK_LEFT,  SDLK_F3,  SDLK_z);   /* left view    ; scanner zoom */
                hat_dir(&s_rt, v&SDL_HAT_RIGHT, SDLK_RIGHT, SDLK_F4,  SDLK_F11); /* right view   ; options      */
                if(q_pop(ev)) return 1;
                continue;
            }
            case SDL_JOYAXISMOTION:{
                int a=e.jaxis.axis, val=e.jaxis.value;
                if(a==AX_L2)      edge(&s_l2, val>16000, SDLK_SLASH);  /* L2 -> decelerate */
                else if(a==AX_R2) edge(&s_r2, val>16000, SDLK_SPACE);  /* R2 -> accelerate */
                if(q_pop(ev)) return 1;
                continue;
            }
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:{
                int down=(e.type==SDL_JOYBUTTONDOWN), b=e.jbutton.button;
                fprintf(stderr,"JOYBTN #%d %s\n", b, down?"down":"up");
                if(b==BTN_SELECT){ s_sel=down;   if(q_pop(ev)) return 1; continue; }
                if(b==BTN_START ){ s_start=down; if(q_pop(ev)) return 1; continue; }
                if(down){
                    if(s_sel){ int m=sel_sym(b);   if(m) q_tap(m); }       /* Select layer */
                    else if(s_start){ int m=start_sym(b); if(m) q_tap(m); }/* Start layer  */
                    else {
                        int h=base_hold_sym(b);
                        if(h){ q_push_key(1,h); if(b>=0&&b<32) s_btn_key[b]=h; }
                        else { int t=base_tap_sym(b); if(t) q_tap(t); }
                    }
                } else if(b>=0 && b<32 && s_btn_key[b]){
                    q_push_key(0,s_btn_key[b]); s_btn_key[b]=0;             /* release held */
                }
                if(q_pop(ev)) return 1;
                continue;
            }
            default:
                *ev=e; return 1;             /* pass keyboard/quit/etc. through */
        }
    }
    return q_pop(ev);                        /* any late synthetic events */
}
