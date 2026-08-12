/* RG35XX input probe: opens the joystick, shows + logs every button/axis/hat with
 * its SDL index, so we can build the Elite control map from real hardware indices.
 * Draws on-screen (SDL_gfx) and writes probe.log next to itself. Auto-closes ~40s. */
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <stdio.h>
#include <string.h>

int main(void){
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
    SDL_Surface *scr = SDL_SetVideoMode(640,480,16,SDL_SWSURFACE|SDL_FULLSCREEN);
    if(!scr) scr = SDL_SetVideoMode(640,480,32,SDL_SWSURFACE|SDL_FULLSCREEN);

    FILE *log = fopen("probe.log","w");
    int njoy = SDL_NumJoysticks();
    SDL_Joystick *js = (njoy>0) ? SDL_JoystickOpen(0) : NULL;
    if(log){
        fprintf(log,"joystick_count=%d opened=%p name=%s buttons=%d axes=%d hats=%d\n",
                njoy,(void*)js, (njoy>0&&SDL_JoystickName(0))?SDL_JoystickName(0):"none",
                js?SDL_JoystickNumButtons(js):0, js?SDL_JoystickNumAxes(js):0, js?SDL_JoystickNumHats(js):0);
        fflush(log);
    }
    SDL_JoystickEventState(SDL_ENABLE);

    char lines[20][80]; int nl=0;
    Uint32 start=SDL_GetTicks();
    int running=1;
    while(running){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            char buf[80]; buf[0]=0;
            switch(e.type){
                case SDL_JOYBUTTONDOWN: snprintf(buf,80,"BUTTON %d  DOWN", e.jbutton.button); break;
                case SDL_JOYBUTTONUP:   snprintf(buf,80,"BUTTON %d  up",   e.jbutton.button); break;
                case SDL_JOYHATMOTION:  snprintf(buf,80,"HAT %d = %d", e.jhat.hat, e.jhat.value); break;
                case SDL_JOYAXISMOTION:
                    if(e.jaxis.value>16000||e.jaxis.value<-16000)
                        snprintf(buf,80,"AXIS %d = %d", e.jaxis.axis, e.jaxis.value);
                    break;
                case SDL_KEYDOWN:       snprintf(buf,80,"KEY sym=%d DOWN", e.key.keysym.sym); break;
                case SDL_QUIT:          running=0; break;
            }
            if(buf[0]){
                if(log){ fprintf(log,"%s\n",buf); fflush(log); }
                if(nl<20) strcpy(lines[nl++],buf);
                else { memmove(lines,lines+1,19*sizeof(lines[0])); strcpy(lines[19],buf); }
            }
        }
        SDL_FillRect(scr,NULL,0);
        stringRGBA(scr,10,10,"RG35XX INPUT PROBE",255,255,255,255);
        stringRGBA(scr,10,24,"Press EVERY button, shoulder, dpad dir once. Note which physical button = which #.",200,200,200,255);
        char hdr[80]; snprintf(hdr,80,"joysticks detected: %d   (0 = pad is NOT an SDL joystick)", njoy);
        stringRGBA(scr,10,42,hdr, njoy>0?0:255, njoy>0?255:0,0,255);
        stringRGBA(scr,10,56,"auto-closes ~40s; results saved to probe.log",150,150,150,255);
        int y=80; for(int i=0;i<nl;i++){ stringRGBA(scr,10,y,lines[i],255,255,0,255); y+=15; }
        SDL_Flip(scr);
        SDL_Delay(16);
        if(SDL_GetTicks()-start > 40000) running=0;
    }
    if(log) fclose(log);
    if(js) SDL_JoystickClose(js);
    SDL_Quit();
    return 0;
}
