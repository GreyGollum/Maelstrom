/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "Maelstrom_Globals.h"
#include "load.h"

#include "../utils/files.h"
#include "personal-effects.h"
#include "personal-icons.h"


// Code-native remaster UI icons. Original PNGs remain untouched.
static UITexture *LoadModernUIIcon(FrameBuf *screen, const char *name)
{
    const bool key = SDL_strcmp(name, "keyicon") == 0;
    const bool button = SDL_strcmp(name, "mainbutton") == 0;
    const bool kid = SDL_strcmp(name, "kidmode") == 0 || SDL_strcmp(name, "kidmode-selected") == 0;
    const bool selected = SDL_strcmp(name, "kidmode-selected") == 0;
    const bool mods = SDL_strcmp(name, "mods") == 0;
    if (!key && !button && !kid && !mods) { return NULL; }
    SDL_Surface *surface = SDL_CreateSurface(128, 128, SDL_PIXELFORMAT_RGBA32);
    if (!surface) { return NULL; }
    for (int y = 0; y < 128; ++y) {
        Uint8 *row = (Uint8 *)surface->pixels + y * surface->pitch;
        for (int x = 0; x < 128; ++x) {
            const float px = (x + 0.5f) / 4.0f, py = (y + 0.5f) / 4.0f;
            const float qx = SDL_max(SDL_fabsf(px - 16.0f) - 10.0f, 0.0f);
            const float qy = SDL_max(SDL_fabsf(py - 16.0f) - 10.0f, 0.0f);
            const float edge = 3.0f - SDL_sqrtf(qx*qx + qy*qy);
            Uint8 *p = row + x*4;
            p[0] = 22; p[1] = 37; p[2] = 54;
            p[3] = (Uint8)(255.0f * SDL_clamp(edge, 0.0f, 1.0f));
            if (edge > 0.0f && edge < 1.1f) { p[0]=65; p[1]=132; p[2]=172; }
            bool ink = false;
            if (mods) {
                ink = (px>9 && px<23 && ((py>9 && py<10.5f)||(py>15 && py<16.5f)||(py>21 && py<22.5f))) ||
                    ((px>12 && px<14 && py>7 && py<13)||(px>19 && px<21 && py>13 && py<19)||(px>14 && px<16 && py>19 && py<25));
            } else if (kid) {
                float dx=px-16, dy=py-14;
                float radius=SDL_sqrtf(dx*dx+dy*dy);
                ink = (radius>8 && radius<9) || ((SDL_fabsf(dx-3)<1 || SDL_fabsf(dx+3)<1) && py>11 && py<13) ||
                    (dy>2 && dy<5 && SDL_fabsf(radius-5)<0.9f);
                if(selected && py>26 && py<28 && px>10 && px<22) { ink=true; }
            }
            if (ink) {p[0]=selected?100:190; p[1]=225; p[2]=245; p[3]=255;}
        }
    }
    SDL_Texture *texture=screen->LoadImage(surface);
    SDL_DestroySurface(surface);
    if(!texture) { return NULL; }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    return new UITexture(texture,4.0f);
}
UITexture *Load_Texture(FrameBuf *screen, const char *folder, const char *name)
{
    if (SDL_strcmp(folder, "Images") == 0 && SDL_GetHintBoolean("MAELSTROM_MODERN_ART", false) &&
        !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false)) {
        UITexture *icon = LoadModernUIIcon(screen, name);
        if (icon) { return icon; }
    }
    char file[256];

	// Use the game display area for determining which art set to use
	for (unsigned int i = gResolutionIndex; i < gResolutions.length(); ++i) {
		SDL_snprintf(file, sizeof(file), "%s%s/%s%s.png",
				folder, gResolutions[i].path_suffix, name, gResolutions[i].file_suffix);
		SDL_Surface *surface = SDL_LoadSurface_IO(OpenRead(file), true);
		if (surface) {
			float visualScale = gResolutions[i].scale;
			// Personal edition: opt-in title artwork only. Original collision
			// surfaces, CRCs, sprites and logical layout remain untouched.
			if (SDL_GetHintBoolean("MAELSTROM_MODERN_ART", false) &&
			    !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false) &&
			    SDL_strcmp(folder, "Images") == 0 &&
			    SDL_strcmp(name, "Maelstrom_Titles#130") == 0) {
				SDL_IOStream *input = OpenRead("PersonalArt/Images/Maelstrom_Titles#130.png");
				SDL_Surface *modern = input ? SDL_LoadSurface_IO(input, true) : NULL;
				if (modern) {
					if (modern->w > surface->w && modern->h > surface->h &&
					    modern->w <= 4096 && modern->h <= 4096 &&
					    (Sint64)modern->w * surface->h == (Sint64)modern->h * surface->w) {
						visualScale *= (float)modern->w / surface->w;
						SDL_DestroySurface(surface);
						surface = modern;
					} else {
						SDL_Log("Personal title has incompatible dimensions; using original");
						SDL_DestroySurface(modern);
					}
				}
			}
			SDL_Texture *texture = screen->LoadImage(surface);
			SDL_DestroySurface(surface);
			// Personal edition: soften enlarged artwork without filtering bitmap text.
			// Original asset bytes and gameplay dimensions remain unchanged.
			if (texture && !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false)) {
				SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
			}
			return new UITexture(texture, visualScale);
		}
	}
	return NULL;
}

void Free_Texture(FrameBuf *screen, UITexture *texture)
{
	screen->FreeImage(texture->Texture());
	delete texture;
}

UITexture *Load_Image(FrameBuf *screen, const char *name)
{
	return Load_Texture(screen, "Images", name);
}

UITexture *Load_Title(FrameBuf *screen, int title_id)
{
	char name[256];
	SDL_snprintf(name, sizeof(name), "Maelstrom_Titles#%d", title_id);
	return Load_Texture(screen, "Images", name);
}

UITexture *GetCIcon(FrameBuf *screen, short id)
{
    if (UITexture *modern = LoadPersonalUpgradeIcon(screen, id, true)) { return modern; }
	char name[256];
	SDL_snprintf(name, sizeof(name), "Maelstrom_Icon#%d", id);
	return Load_Texture(screen, "Images", name);
}

// Personal edition: validate a complete visual animation before selecting it.
// init.cpp continues to derive hit rectangles, masks and CRC from original art.
// Restart after changing art files: availability is deliberately cached per run.
static bool HasCompletePersonalSpriteSet(int id)
{
	struct Set { int first, count, state; };
	static Set sets[] = { {200, 48, 0}, {500, 60, 0}, {1300, 2, 0} };
	for (auto &set : sets) {
		if (id < set.first || id >= set.first + set.count) {
			continue;
		}
		if (set.state == 0) {
			set.state = -1;
			int dimension = 0;
			bool complete = true;
			for (int frame = 0; frame < set.count; ++frame) {
				char path[128];
				SDL_snprintf(path, sizeof(path), "PersonalArt/Sprites/icl8#%d.png", set.first + frame);
				SDL_IOStream *input = OpenRead(path);
				SDL_Surface *candidate = input ? SDL_LoadSurface_IO(input, true) : NULL;
				if (!candidate) { complete = false; break; }
				const bool valid = candidate->w == candidate->h &&
					(candidate->w == 64 || candidate->w == 128 || candidate->w == 256) &&
					(!dimension || dimension == candidate->w) &&
					SDL_ISPIXELFORMAT_ALPHA(candidate->format);
				dimension = candidate->w;
				SDL_DestroySurface(candidate);
				if (!valid) { complete = false; break; }
			}
			if (complete) { set.state = 1; }
			else { SDL_Log("Personal sprite set %d incomplete or invalid; using originals", set.first); }
		}
		return set.state == 1;
	}
	return false;
}

// Optional source-art prototype: rotate a single image at the existing phase.
// This is a planar spin, not a replacement for authored tumbling frames.
// Personal procedural shield: visual-only, no source art or physics changes.
static UITexture *LoadPersonalShield(FrameBuf *screen, int id)
{
    SDL_Surface *surface = SDL_CreateSurface(128, 128, SDL_PIXELFORMAT_RGBA32);
    if (!surface) { return NULL; }
    for (int y = 0; y < 128; ++y) {
        Uint8 *row = (Uint8 *)surface->pixels + y * surface->pitch;
        for (int x = 0; x < 128; ++x) {
            const float dx = (x + 0.5f - 64.0f) / 60.0f;
            const float dy = (y + 0.5f - 64.0f) / 60.0f;
            const float r = SDL_sqrtf(dx * dx + dy * dy);
            Uint8 *pixel = row + x * 4;
            const float rim = SDL_max(0.0f, 1.0f - SDL_fabsf(r - 0.94f) / 0.06f);
            const float highlight = SDL_max(0.0f, 1.0f - ((dx + 0.35f)*(dx + 0.35f) + (dy + 0.4f)*(dy + 0.4f)) / 0.12f);
            pixel[0] = id == 1300 ? 240 : 80;
            pixel[1] = id == 1300 ? 110 : 180;
            pixel[2] = id == 1300 ? 160 : 255;
            pixel[3] = r < 1.0f ? (Uint8)(10.0f + 145.0f * rim + 28.0f * highlight) : 0;
        }
    }
    SDL_Texture *texture = screen->LoadImage(surface);
    SDL_DestroySurface(surface);
    if (!texture) { return NULL; }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    return new UITexture(texture, 4.0f);
}
// Cache only within one LoadSprite batch. Scope cleanup also handles failures,
// renderer recreation and mod reloads without retaining stale source surfaces.
static SDL_Surface *personalSourceRGBA = NULL;
static const char *personalSourcePath = NULL;
static int personalBounds[4] = {0,0,0,0};
void ResetPersonalSpriteSourceCache()
{
    SDL_DestroySurface(personalSourceRGBA);
    personalSourceRGBA = NULL;
    personalSourcePath = NULL;
}
static SDL_Surface *GetPersonalSpriteSource(const char *path)
{
    if (personalSourcePath && SDL_strcmp(path, personalSourcePath) == 0) {
        return personalSourceRGBA;
    }
    ResetPersonalSpriteSourceCache();
    personalSourcePath = path;
    SDL_IOStream *input = OpenRead(path);
    SDL_Surface *source = input ? SDL_LoadSurface_IO(input, true) : NULL;
    if (!source) { return NULL; }
    if (source->w <= 2048 && source->h <= 2048 && source->w >= 64 && source->h >= 64 &&
        SDL_ISPIXELFORMAT_ALPHA(source->format)) {
        personalSourceRGBA = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
    }
    SDL_DestroySurface(source);
    if (personalSourceRGBA) {
        SDL_Surface *p=personalSourceRGBA;
        personalBounds[0]=p->w; personalBounds[1]=p->h; personalBounds[2]=-1; personalBounds[3]=-1;
        for(int y=0;y<p->h;++y) for(int x=0;x<p->w;++x) {
            const Uint8 *pixel=(const Uint8*)p->pixels+y*p->pitch+x*4;
            if(pixel[3]>32) {
                personalBounds[0]=SDL_min(personalBounds[0],x); personalBounds[1]=SDL_min(personalBounds[1],y);
                personalBounds[2]=SDL_max(personalBounds[2],x); personalBounds[3]=SDL_max(personalBounds[3],y);
            }
        }
    }
    return personalSourceRGBA;
}
// Procedural color treatment only: retain each original phase and exact alpha.
// This does not replace collision art or invent additional animation frames.
static UITexture *LoadPersonalEffect(FrameBuf *screen, int id, bool large, SDL_Surface *borrowed)
{
    const bool vortex = large && id >= 1100 && id < 1110;
    const bool fire = large && ((id >= 600 && id < 612) || (id >= 1400 && id < 1418) || (id >= 1500 && id < 1521));
    const bool thrust = !large && ((id >= 400 && id < 448) || (id >= 500 && id < 548));
    if (!vortex && !fire && !thrust) { return NULL; }
    char path[96];
    SDL_snprintf(path, sizeof(path), "Sprites/%s#%d.png", large ? "icl8" : "ics8", id);
    SDL_IOStream *input = borrowed ? NULL : OpenRead(path);
    SDL_Surface *original = borrowed ? borrowed : (input ? SDL_LoadSurface_IO(input, true) : NULL);
    if (!original) { return NULL; }
    SDL_Surface *rgba = SDL_ConvertSurface(original, SDL_PIXELFORMAT_RGBA32);
    if (!borrowed) { SDL_DestroySurface(original); }
    if (!rgba) { return NULL; }
    SDL_Surface *canvas = SDL_ScaleSurface(rgba, 128, 128, SDL_SCALEMODE_NEAREST);
    if (!canvas) { SDL_DestroySurface(rgba); return NULL; }
    bool alphaMatches = true;
    for (int y=0; y<128; ++y) {
        Uint8 *row = (Uint8 *)canvas->pixels + y*canvas->pitch;
        const Uint8 *expected = (const Uint8 *)rgba->pixels + (y*rgba->h/128)*rgba->pitch;
        for (int x=0; x<128; ++x) {
            Uint8 *p = row+x*4;
            if (p[3]) {
                const float dx=(x+0.5f-64.0f)/64.0f, dy=(y+0.5f-64.0f)/64.0f;
                const float core=SDL_max(0.0f,1.0f-SDL_sqrtf(dx*dx+dy*dy));
                const float light=SDL_max(p[0],SDL_max(p[1],p[2]))/255.0f;
                if (vortex) {
                    p[0]=(Uint8)(45+100*core+35*light);
                    p[1]=(Uint8)(80+100*core+65*light);
                    p[2]=(Uint8)(155+100*light);
                } else if (thrust) {
                    p[0]=(Uint8)(70+180*light); p[1]=(Uint8)(140+110*light); p[2]=255;
                } else {
                    p[0]=(Uint8)(170+85*light);
                    p[1]=(Uint8)(50+145*light+50*core);
                    p[2]=(Uint8)(25+100*light*core);
                }
            }
            if (p[3] != expected[(x*rgba->w/128)*4+3]) { alphaMatches=false; }
        }
    }
    SDL_DestroySurface(rgba);
    if (!alphaMatches) { SDL_DestroySurface(canvas); return NULL; }
    if (SDL_GetHintBoolean("MAELSTROM_ART_DIAGNOSTICS", false)) { SDL_Log("Personal pixels %d: %08x", id, SDL_crc32(0, canvas->pixels, canvas->pitch * canvas->h)); }
    SDL_Texture *texture=screen->LoadImage(canvas);
    SDL_DestroySurface(canvas);
    if (!texture) { return NULL; }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    SDL_Log("Personal effect %d: alpha footprint verified", id);
    return new UITexture(texture, large ? 4.0f : 8.0f);
}
// HD display-only art. Source alpha supplies smooth silhouettes, while init.cpp
// retains the original low-resolution collision surface, hit rectangle and CRC.
// Fit each visual inside the original phase's occupied bounding rectangle.
static UITexture *LoadPersonalHD(FrameBuf *screen, SDL_Surface *source, SDL_Surface *collision, float angle, int id, bool large)
{
    const int resolution=256;
    const Uint32 collisionCRC=SDL_crc32(0, collision->pixels, collision->pitch*collision->h);
    int left=collision->w, top=collision->h, right=-1, bottom=-1;
    for(int y=0;y<collision->h;++y) for(int x=0;x<collision->w;++x) {
        const Uint8 *p=(const Uint8*)collision->pixels+y*collision->pitch+x*4;
        if(p[3]) { left=SDL_min(left,x); right=SDL_max(right,x); top=SDL_min(top,y); bottom=SDL_max(bottom,y); }
    }
    const int sw=personalBounds[2]-personalBounds[0]+1, sh=personalBounds[3]-personalBounds[1]+1;
    if(right<left || sw<=0 || sh<=0) { return NULL; }
    SDL_Surface *canvas=SDL_CreateSurface(resolution,resolution,SDL_PIXELFORMAT_RGBA32);
    if(!canvas) { return NULL; }
    SDL_ClearSurface(canvas,0,0,0,0);
    const float radians=angle*0.017453292519943295f, c=SDL_cosf(radians), s=SDL_sinf(radians);
    const float rw=SDL_fabsf(c)*sw+SDL_fabsf(s)*sh, rh=SDL_fabsf(s)*sw+SDL_fabsf(c)*sh;
    // Skip transparent rows/columns before sampling, without changing pixel
    // centers, bilinear weights, resolution or the occupied phase bounds.
    const int xBegin=SDL_max(0,(int)SDL_ceilf((float)left*resolution/collision->w-0.5f));
    const int yBegin=SDL_max(0,(int)SDL_ceilf((float)top*resolution/collision->h-0.5f));
    const int xEnd=SDL_min(resolution,(int)SDL_ceilf((float)(right+1)*resolution/collision->w-0.5f));
    const int yEnd=SDL_min(resolution,(int)SDL_ceilf((float)(bottom+1)*resolution/collision->h-0.5f));
    for(int y=yBegin;y<yEnd;++y) for(int x=xBegin;x<xEnd;++x) {
        const float lx=(x+0.5f)*collision->w/resolution, ly=(y+0.5f)*collision->h/resolution;
        if(lx<left || lx>=right+1 || ly<top || ly>=bottom+1) { continue; }
        const float dx=((lx-left)/(right-left+1)-0.5f)*rw;
        const float dy=((ly-top)/(bottom-top+1)-0.5f)*rh;
        const float sx=c*dx+s*dy+personalBounds[0]+sw*0.5f-0.5f;
        const float sy=-s*dx+c*dy+personalBounds[1]+sh*0.5f-0.5f;
        const int ix=(int)SDL_floorf(sx), iy=(int)SDL_floorf(sy);
        const float fx=sx-ix, fy=sy-iy;
        float alpha=0, color[3]={0,0,0};
        for(int j=0;j<2;++j) for(int i=0;i<2;++i) {
            const int px=ix+i, py=iy+j;
            if(px<0 || py<0 || px>=source->w || py>=source->h) { continue; }
            const Uint8 *p=(const Uint8*)source->pixels+py*source->pitch+px*4;
            const float weight=(i?fx:1-fx)*(j?fy:1-fy)*p[3];
            alpha+=weight;
            for(int k=0;k<3;++k) { color[k]+=weight*p[k]; }
        }
        Uint8 *out=(Uint8*)canvas->pixels+y*canvas->pitch+x*4;
        if(alpha>0) { for(int k=0;k<3;++k) { out[k]=(Uint8)SDL_clamp(color[k]/alpha,0.0f,255.0f); } out[3]=(Uint8)SDL_clamp(alpha,0.0f,255.0f); }
    }
    if(collisionCRC!=SDL_crc32(0,collision->pixels,collision->pitch*collision->h)) { SDL_DestroySurface(canvas); return NULL; }
    SDL_Texture *texture=screen->LoadImage(canvas);
    SDL_DestroySurface(canvas);
    if(!texture) { return NULL; }
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_LINEAR);
    SDL_Log("HD sprite %d: original collision pixels unchanged",id);
    return new UITexture(texture, large ? 8.0f : 16.0f);
}

static UITexture *LoadPersonalSourceSprite(FrameBuf *screen, int id, bool large, SDL_Surface *borrowed)
{
	if (large && (id == 1300 || id == 1301)) { return LoadPersonalShield(screen, id); }
    if (!SDL_GetHintBoolean("MAELSTROM_MASKED_ART", false)) {
        UITexture *procedural = LoadPersonalProcedural(screen, id, large, borrowed);
        if (procedural) { return procedural; }
    }
    UITexture *effect = LoadPersonalEffect(screen, id, large, borrowed);
    if (effect) { return effect; }
	const char *path = NULL;
	float angle = 0.0f;
    bool fitSourceBounds = false;
	if (id >= 200 && id < 248) {
		path = "PersonalArt/Images/ship-source.png";
		angle = (id - 200) * 7.5f;
	} else if (!large && id >= 300 && id < 320) {
        path = "PersonalArt/Images/asteroid-source.png";
        angle = (id - 300) * 18.0f;
    } else if (large && id >= 400 && id < 440) {
        path = "PersonalArt/Images/asteroid-source.png";
        angle = (id - 400) * 9.0f;
    } else if (large && id >= 500 && id < 560) {
		path = "PersonalArt/Images/asteroid-source.png";
		angle = (id - 500) * 6.0f;
	}
	    if (large && id >= 800 && id < 830) {
        path = "PersonalArt/Images/pickup.png"; fitSourceBounds = true;
    } else if (large && id >= 900 && id < 910) {
        path = "PersonalArt/Images/bonus.png"; fitSourceBounds = true;
    } else if (large && id >= 1200 && id < 1240) {
        path = "PersonalArt/Images/mine.png"; angle = (id-1200)*9.0f; fitSourceBounds = true;
    } else if (large && id >= 1700 && id < 1740) {
        path = "PersonalArt/Images/enemy-purple.png"; angle = (id-1700)*9.0f; fitSourceBounds = true;
    } else if (large && id >= 2100 && id < 2140) {
        path = "PersonalArt/Images/enemy-green.png"; angle = (id-2100)*9.0f; fitSourceBounds = true;
    }
    if (!path) { return NULL; }
	SDL_Surface *rgba = GetPersonalSpriteSource(path);
	if (!rgba) { return NULL; }
	char originalPath[128];
	SDL_snprintf(originalPath, sizeof(originalPath), "Sprites/%s#%d.png", large ? "icl8" : "ics8", id);
	SDL_IOStream *originalInput = borrowed ? NULL : OpenRead(originalPath);
	SDL_Surface *original = borrowed ? borrowed : (originalInput ? SDL_LoadSurface_IO(originalInput, true) : NULL);
	if (!original) {  return NULL; }
	SDL_Surface *originalRGBA = SDL_ConvertSurface(original, SDL_PIXELFORMAT_RGBA32);
	if (!borrowed) { SDL_DestroySurface(original); }
	if (!originalRGBA) {  return NULL; }
    if(!SDL_GetHintBoolean("MAELSTROM_MASKED_ART",false)) {
        UITexture *hd=LoadPersonalHD(screen,rgba,originalRGBA,angle,id,large);
        SDL_DestroySurface(originalRGBA);
        return hd;
    }
	SDL_Surface *canvas = SDL_ScaleSurface(originalRGBA, 128, 128, SDL_SCALEMODE_NEAREST);
	if (!canvas) { SDL_DestroySurface(originalRGBA);  return NULL; }
	    // New single-image sources are color overlays inside the original phase mask.
    // Fit occupied bounds, avoiding artwork margins shrinking the visible color.
    int ox0=0, oy0=0, ox1=127, oy1=127;
    int sx0=0, sy0=0, sw=rgba->w, sh=rgba->h;
    if(fitSourceBounds) {
        ox0=oy0=128; ox1=oy1=-1;
        for(int y=0;y<128;++y) for(int x=0;x<128;++x) {
            if(*((Uint8*)canvas->pixels+y*canvas->pitch+x*4+3)) {
                ox0=SDL_min(ox0,x); ox1=SDL_max(ox1,x); oy0=SDL_min(oy0,y); oy1=SDL_max(oy1,y);
            }
        }
        sx0=personalBounds[0]; sy0=personalBounds[1]; sw=personalBounds[2]-sx0+1; sh=personalBounds[3]-sy0+1;
        if(ox1<ox0 || sw<=0 || sh<=0) { SDL_DestroySurface(originalRGBA); SDL_DestroySurface(canvas); return NULL; }
    }
    const float radians = angle * 0.017453292519943295f;
	const float cosine = SDL_cosf(radians), sine = SDL_sinf(radians);
	for (int y = 0; y < 128; ++y) {
		Uint8 *row = (Uint8 *)canvas->pixels + y * canvas->pitch;
		for (int x = 0; x < 128; ++x) {
			Uint8 *pixel = row + x * 4;
			if (!pixel[3]) { continue; }
			const float dx = (x - ox0 + 0.5f) / (ox1-ox0+1) - 0.5f;
			const float dy = (y - oy0 + 0.5f) / (oy1-oy0+1) - 0.5f;
			const int sx = (int)SDL_floorf((cosine * dx + sine * dy + 0.5f) * sw + sx0);
			const int sy = (int)SDL_floorf((-sine * dx + cosine * dy + 0.5f) * sh + sy0);
			if (sx < 0 || sy < 0 || sx >= rgba->w || sy >= rgba->h) { continue; }
			const Uint8 *sample = (const Uint8 *)rgba->pixels + sy * rgba->pitch + sx * 4;
			for (int channel = 0; channel < 3; ++channel) {
				pixel[channel] = (Uint8)((sample[channel] * sample[3] + pixel[channel] * (255 - sample[3])) / 255);
			}
		}
	}
    bool alphaMatches = true;
    for (int y = 0; y < 128; ++y) {
        const Uint8 *actual = (const Uint8 *)canvas->pixels + y * canvas->pitch;
        const Uint8 *expected = (const Uint8 *)originalRGBA->pixels + (y * originalRGBA->h / 128) * originalRGBA->pitch;
        for (int x = 0; x < 128; ++x) {
            if (actual[x * 4 + 3] != expected[(x * originalRGBA->w / 128) * 4 + 3]) { alphaMatches = false; }
        }
    }
    SDL_DestroySurface(originalRGBA);

    if (!alphaMatches) {
        SDL_Log("Personal sprite %d alpha validation failed; using classic", id);
        SDL_DestroySurface(canvas);
        return NULL;
    }
    SDL_Log("Personal sprite %d: alpha footprint verified", id);
    if (SDL_GetHintBoolean("MAELSTROM_ART_DIAGNOSTICS", false)) { SDL_Log("Personal pixels %d: %08x", id, SDL_crc32(0, canvas->pixels, canvas->pitch * canvas->h)); }
    SDL_Texture *texture = screen->LoadImage(canvas);
	SDL_DestroySurface(canvas);
	if (!texture) { return NULL; }
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
	UITexture *result = new UITexture(texture, large ? 4.0f : 8.0f);
	return result;
}

UITexture *GetSprite(FrameBuf *screen, short id, bool large, SDL_Surface *originalSurface)
{
    if (large) {
        if (UITexture *modern = LoadPersonalUpgradeIcon(screen, id, false)) { return modern; }
    }
	char name[256];
	SDL_snprintf(name, sizeof(name), "%s#%d", large ? "icl8" : "ics8", id);
	if (SDL_GetHintBoolean("MAELSTROM_SOURCE_SPRITES", false) &&
	    !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false)) {
		UITexture *prototype = LoadPersonalSourceSprite(screen, id, large, originalSurface);
		if (prototype) { return prototype; }
	}
	if (large && SDL_GetHintBoolean("MAELSTROM_MODERN_ART", false) &&
	    !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false) && HasCompletePersonalSpriteSet(id)) {
		char path[256];
		SDL_snprintf(path, sizeof(path), "PersonalArt/Sprites/%s.png", name);
		SDL_IOStream *input = OpenRead(path);
		SDL_Surface *surface = input ? SDL_LoadSurface_IO(input, true) : NULL;
		if (surface) {
			const float scale = surface->w / 32.0f;
			SDL_Texture *texture = screen->LoadImage(surface);
			SDL_DestroySurface(surface);
			if (texture) {
				SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
				return new UITexture(texture, scale);
			}
		}
	}
	return Load_Texture(screen, "Sprites", name);
}
