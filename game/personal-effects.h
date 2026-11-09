// Personal remaster additions, 2026. Native display-only procedural art.
// Original Maelstrom art, masks, phase counts and game rules remain unchanged.
// Included by load.cpp after the original image/filesystem declarations.
#ifndef MAELSTROM_PERSONAL_EFFECTS_H
#define MAELSTROM_PERSONAL_EFFECTS_H

static UITexture *LoadPersonalProcedural(FrameBuf *screen, int id, bool large, SDL_Surface *borrowed)
{
    const bool vortex = large && id >= 1100 && id < 1110;
    const bool fire = large && ((id >= 600 && id < 612) || (id >= 1400 && id < 1418) || (id >= 1500 && id < 1521));
    const bool thrust = !large && ((id >= 400 && id < 448) || (id >= 500 && id < 548));
    const bool debris = large && ((id >= 1800 && id < 1848) || (id >= 1900 && id < 1940));
    if (!vortex && !fire && !thrust && !debris) { return NULL; }
    char path[96];
    SDL_snprintf(path, sizeof(path), "Sprites/%s#%d.png", large ? "icl8" : "ics8", id);
    SDL_IOStream *input = borrowed ? NULL : OpenRead(path);
    SDL_Surface *original = borrowed ? borrowed : (input ? SDL_LoadSurface_IO(input, true) : NULL);
    if (!original) { return NULL; }
    const Uint32 crc = SDL_crc32(0, original->pixels, original->pitch * original->h);
    SDL_Surface *rgba = SDL_ConvertSurface(original, SDL_PIXELFORMAT_RGBA32);
    if (!rgba) { if (!borrowed) SDL_DestroySurface(original); return NULL; }
    int left=rgba->w, top=rgba->h, right=-1, bottom=-1;
    float mass=0, mx=0, my=0, meanLight=0;
    for (int y=0;y<rgba->h;++y) for(int x=0;x<rgba->w;++x) {
        const Uint8 *p=(const Uint8*)rgba->pixels+y*rgba->pitch+x*4;
        if (!p[3]) continue;
        left=SDL_min(left,x); right=SDL_max(right,x); top=SDL_min(top,y); bottom=SDL_max(bottom,y);
        const float a=p[3]/255.0f;
        mass+=a; mx+=a*(x+0.5f); my+=a*(y+0.5f);
        meanLight+=a*SDL_max(p[0],SDL_max(p[1],p[2]))/255.0f;
    }
    const int resolution=large ? 256 : 128;
    SDL_Surface *canvas=SDL_CreateSurface(resolution,resolution,SDL_PIXELFORMAT_RGBA32);
    if (!canvas) { SDL_DestroySurface(rgba); if(!borrowed) SDL_DestroySurface(original); return NULL; }
    SDL_ClearSurface(canvas,0,0,0,0);
    if (mass>0 && right>=left) {
        mx/=mass; my/=mass; meanLight/=mass;
        // Debris orientation comes from its original phase's principal axis.
        float xx=0, yy=0, xy=0;
        if (debris || thrust) {
            for(int y=top;y<=bottom;++y) for(int x=left;x<=right;++x) {
                const Uint8 *p=(const Uint8*)rgba->pixels+y*rgba->pitch+x*4;
                const float a=p[3]/255.0f, dx=x+0.5f-mx, dy=y+0.5f-my;
                xx+=a*dx*dx; yy+=a*dy*dy; xy+=a*dx*dy;
            }
        }
        const float orientation=0.5f*SDL_atan2f(2*xy,xx-yy);
        const float cs=SDL_cosf(orientation), sn=SDL_sinf(orientation);
        const float phase=(float)(id%100);
        const float width=(float)(right-left+1), height=(float)(bottom-top+1);
        const float aa=2.0f*rgba->w/resolution/SDL_min(width,height);
        // Skip only pixels that the original rectangle guard rejected. Keep
        // the same pixel-centre arithmetic and guard, so output is identical.
        const int xBegin=left*resolution/rgba->w;
        const int xEnd=((right+1)*resolution+rgba->w-1)/rgba->w;
        const int yBegin=top*resolution/rgba->h;
        const int yEnd=((bottom+1)*resolution+rgba->h-1)/rgba->h;
        float logicalX[256], normalizedX[256];
        for(int x=xBegin;x<xEnd;++x) {
            logicalX[x]=(x+0.5f)*rgba->w/resolution;
            normalizedX[x]=2*(logicalX[x]-left)/width-1;
        }
        for(int y=yBegin;y<yEnd;++y) {
          const float ly=(y+0.5f)*rgba->h/resolution;
          const float dy=2*(ly-top)/height-1;
          Uint8 *row=(Uint8*)canvas->pixels+y*canvas->pitch;
          for(int x=xBegin;x<xEnd;++x) {
            const float lx=logicalX[x];
            if(lx<left || lx>=right+1 || ly<top || ly>=bottom+1) continue;
            const float dx=normalizedX[x];
            float a=0, red=0, green=0, blue=0;
            if (fire) {
                const float r=SDL_sqrtf(dx*dx+dy*dy);
                const float angle=SDL_atan2f(dy,dx);
                const float rays=0.77f+0.14f*SDL_cosf(angle*11+phase*0.31f)+0.07f*SDL_cosf(angle*19-phase*0.19f);
                const float edge=SDL_clamp((rays-r)/aa,0.0f,1.0f);
                const float core=SDL_clamp(1-r,0.0f,1.0f);
                a=edge*(0.3f+0.7f*core)*(0.3f+0.7f*meanLight);
                red=255; green=65+190*core; blue=20+235*core*core*core;
            } else if (vortex) {
                const float r=SDL_sqrtf(dx*dx+dy*dy);
                const float angle=SDL_atan2f(dy,dx);
                const float arm=0.5f+0.5f*SDL_cosf(3*angle+9*r-phase*0.62831853f);
                const float rim=SDL_clamp((1-r)/aa,0.0f,1.0f);
                a=rim*(0.10f+0.85f*arm*arm*arm)*SDL_clamp(r*5,0.0f,1.0f);
                red=65+80*arm; green=100+135*arm; blue=255;
            } else {
                // Convex shard/flame with antialiased analytic edges, rather than
                // nearest-neighbour magnification of the original pixel mask.
                const float u=cs*dx+sn*dy, v=-sn*dx+cs*dy;
                const float taper=SDL_max(0.0f,1-SDL_fabsf(u));
                const float halfWidth=(debris ? 0.52f : 0.62f)*taper;
                a=SDL_clamp((halfWidth-SDL_fabsf(v))/aa,0.0f,1.0f)*SDL_clamp((1-SDL_fabsf(u))/aa,0.0f,1.0f);
                const float highlight=SDL_clamp(1-SDL_fabsf(v)/(halfWidth+0.001f),0.0f,1.0f);
                if(debris) { red=65+160*highlight; green=105+140*highlight; blue=145+110*highlight; }
                else { red=60+195*highlight; green=140+115*highlight; blue=255; a*=0.55f+0.45f*meanLight; }
            }
            Uint8 *p=row+x*4;
            p[0]=(Uint8)SDL_clamp(red,0.0f,255.0f); p[1]=(Uint8)SDL_clamp(green,0.0f,255.0f);
            p[2]=(Uint8)SDL_clamp(blue,0.0f,255.0f); p[3]=(Uint8)(255*SDL_clamp(a,0.0f,1.0f));
          }
        }
    }
    const bool intact=crc==SDL_crc32(0,original->pixels,original->pitch*original->h);
    SDL_DestroySurface(rgba);
    if(!borrowed) SDL_DestroySurface(original);
    if(!intact) { SDL_DestroySurface(canvas); return NULL; }
    SDL_Texture *texture=screen->LoadImage(canvas);
    SDL_DestroySurface(canvas);
    if(!texture) return NULL;
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_LINEAR);
    SDL_Log("HD procedural %d: original collision pixels unchanged",id);
    return new UITexture(texture,8.0f);
}
#endif
