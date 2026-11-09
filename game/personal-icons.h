/* Personal-edition visual-only icon renderer, 2026.
 * New procedural artwork; not original Ambrosia artwork. Original assets and
 * Andrew Welch/Ambrosia, Ian Gilman, Mark Lewis and Sam Lantinga credits remain
 * in the distribution. Licensed under the same zlib terms as Maelstrom.
 * Include after Maelstrom_Globals.h. No masks, scoring or animation are changed.
 */
#ifndef MAELSTROM_PERSONAL_ICONS_H
#define MAELSTROM_PERSONAL_ICONS_H

static float PersonalIconLine(float x, float y, float ax, float ay, float bx, float by)
{
    const float dx=bx-ax, dy=by-ay;
    float t=((x-ax)*dx+(y-ay)*dy)/(dx*dx+dy*dy);
    t=SDL_clamp(t,0.0f,1.0f);
    const float ex=x-ax-t*dx, ey=y-ay-t*dy;
    return SDL_sqrtf(ex*ex+ey*ey);
}

// Rounded vector strokes. Coordinates are a 4 x 6 digit cell.
static float PersonalIconDigit(float x,float y,int digit)
{
    static const unsigned char segments[10]={63,6,91,79,102,109,125,7,127,111};
    static const float lines[7][4]={
        {0,0,4,0},{4,0,4,3},{4,3,4,6},{0,6,4,6},
        {0,3,0,6},{0,0,0,3},{0,3,4,3}};
    float d=100.0f;
    for(int i=0;i<7;++i) {
        if(segments[digit]&(1<<i)) {
            d=SDL_min(d,PersonalIconLine(x,y,lines[i][0],lines[i][1],lines[i][2],lines[i][3]));
        }
    }
    return d;
}

// hud=true handles only original 8x8 CIcons 128..137.
// hud=false handles only original 32x32 multiplier and point-display sprites.
// Returning NULL always means use the original loader unchanged.
static UITexture *LoadPersonalUpgradeIcon(FrameBuf *screen, int id, bool hud)
{
    if(!SDL_GetHintBoolean("MAELSTROM_MODERN_ART",false) ||
       SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART",false)) { return NULL; }
    const bool multiplier = !hud && (id==2000 || id==2002 || id==2004 || id==2006);
    const bool points = !hud && id>=1000 && id<=1005;
    if((hud && (id<128 || id>137)) || (!hud && !multiplier && !points)) {return NULL;}
    const int dimension=hud?64:128;
    const float scale=hud?8.0f:4.0f;
    const float logical=hud?8.0f:32.0f;
    SDL_Surface *surface=SDL_CreateSurface(dimension,dimension,SDL_PIXELFORMAT_RGBA32);
    if(!surface) {return NULL;}
    int factor=multiplier?2+(id-2000)/2:0;
    if(hud) {switch(id) {case 130:factor=2;break;case 131:factor=3;break;case 132:factor=4;break;case 134:factor=5;break;}}
    // Preserve the classic color vocabulary for every upgrade and score value.
    Uint8 red=70,green=160,blue=255;
    if(factor==2 || id==133 || id==1000) {red=60;green=245;blue=95;}
    if(factor==3 || id==1001) {red=50;green=130;blue=255;}
    if(factor==4 || id==1002 || (hud&&id==137)) {red=255;green=80;blue=40;}
    if(factor==5 || id==1003) {red=255;green=65;blue=225;}
    if(id==1004) {red=255;green=240;blue=65;}
    if(id==1005) {red=80;green=230;blue=255;}
    char number[8]={0};
    if(points) {SDL_snprintf(number,sizeof(number),"%d",id==1000?500:(id-1000)*1000);}
    for(int y=0;y<dimension;++y) {
        Uint8 *row=(Uint8*)surface->pixels+y*surface->pitch;
        for(int x=0;x<dimension;++x) {
            const float px=(x+0.5f)/scale, py=(y+0.5f)/scale;
            float d=100.0f, fill=0.0f, stroke=0.0f;
            if(points) {
                const int length=(int)SDL_strlen(number);
                const float left=(32.0f-(length*6.0f-2.0f))*0.5f;
                for(int i=0;i<length;++i) {
                    d=SDL_min(d,PersonalIconDigit(px-left-i*6.0f,py-13.0f,number[i]-'0'));
                }
                stroke=SDL_clamp((0.48f-d)*scale+0.5f,0.0f,1.0f);
            } else if(factor) {
                const float unit=hud?0.55f:1.25f;
                const float cx=logical*0.5f, cy=cx;
                const float radius=SDL_sqrtf((px-cx)*(px-cx)+(py-cy)*(py-cy));
                if(!hud) {fill=SDL_clamp((7.6f-radius)*scale+0.5f,0.0f,1.0f);}
                const float tx=(px-cx)/unit, ty=(py-cy)/unit;
                d=SDL_min(PersonalIconLine(tx,ty,-4,-1.3f,-2,1.3f),PersonalIconLine(tx,ty,-4,1.3f,-2,-1.3f));
                d=SDL_min(d,PersonalIconDigit(tx+0.2f,ty+3,factor));
                stroke=SDL_clamp((0.42f-d)*unit*scale+0.5f,0.0f,1.0f);
            } else {
                const float dx=px-4,dy=py-4;
                const float radius=SDL_sqrtf(dx*dx+dy*dy);
                if(id==128) {fill=SDL_clamp((3.25f-radius)*scale,0.0f,1.0f);}
                if(id==129) {
                    d=SDL_min(PersonalIconLine(px,py,1,4,4,1),PersonalIconLine(px,py,4,1,7,4));
                    d=SDL_min(d,PersonalIconLine(px,py,4,1,4,7));
                    stroke=SDL_clamp((0.7f-d)*scale,0.0f,1.0f);red=245;green=245;blue=250;
                }
                if(id==133) {
                    for(int i=0;i<4;++i) {float cx=4+(i<2?-1.35f:1.35f),cy=4+(i%2?-1.35f:1.35f);
                        float ax=px-cx,ay=py-cy;d=SDL_min(d,SDL_sqrtf(ax*ax+ay*ay));}
                    fill=SDL_clamp((1.7f-d)*scale,0.0f,1.0f);
                }
                if(id==135 || id==136) {
                    for(int i=0;i<3;++i) {float cx=id==135?2.0f+i*2.0f:4.0f,cy=id==135?(i==1?2.0f:5.2f):1.6f+i*2.4f;
                        float ax=px-cx,ay=py-cy;d=SDL_min(d,SDL_sqrtf(ax*ax+ay*ay));}
                    fill=SDL_clamp((1.1f-d)*scale,0.0f,1.0f);
                }
                if(id==137) {stroke=SDL_clamp((0.6f-SDL_fabsf(radius-2.7f))*scale,0.0f,1.0f);}
            }
            Uint8 *p=row+x*4;
            const float alpha=SDL_max(fill,stroke);
            float light=1.0f;
            if(fill>0 && !points) {light=0.45f+0.4f*(1.0f-py/logical);}
            // Multiplier letters stay bright against a subdued colored disk.
            light=SDL_max(light,stroke);
            p[0]=(Uint8)(red*light);p[1]=(Uint8)(green*light);p[2]=(Uint8)(blue*light);p[3]=(Uint8)(255*alpha);
        }
    }
    SDL_Texture *texture=screen->LoadImage(surface);
    SDL_DestroySurface(surface);
    if(!texture) {return NULL;}
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_LINEAR);
    return new UITexture(texture,scale);
}
#endif
