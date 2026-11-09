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

#include "MaelstromUI.h"
#include "Maelstrom_Globals.h"
#include "main.h"
#include "load.h"
#include "controls.h"
#include "init.h"
#include "about.h"
#include "game.h"
#include "continue.h"
#include "gameover.h"
#include "player.h"
#include "lobby.h"
#include "mods.h"
#include "MacDialog.h"
#include "../screenlib/UIContainer.h"
#include "../screenlib/UIElementButton.h"
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementEditbox.h"
#include "../screenlib/UIElementRadio.h"
#include "../screenlib/UIElementThumbstick.h"
#include "../screenlib/UIDialogButton.h"
#include "../utils/hashtable.h"
#include "../utils/files.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "personal-font/stb_truetype.h"

struct DialogInfo {
    DialogInfo(const char *text,
               const char *button1Text, const char *button1Action,
               const char *button2Text, const char *button2Action,
               const char *button3Text, const char *button3Action)
        : text(text ? SDL_strdup(text) : NULL),
          button1Text(button1Text ? SDL_strdup(button1Text) : NULL),
          button1Action(button1Action ? SDL_strdup(button1Action) : NULL),
          button2Text(button2Text ? SDL_strdup(button2Text) : NULL),
          button2Action(button2Action ? SDL_strdup(button2Action) : NULL),
          button3Text(button3Text ? SDL_strdup(button3Text) : NULL),
          button3Action(button3Action ? SDL_strdup(button3Action) : NULL)
    { }

    ~DialogInfo() {
        if (this->text) {
            SDL_free(this->text);
        }
        if (this->button1Text) {
            SDL_free(this->button1Text);
        }
        if (this->button1Action) {
            SDL_free(this->button1Action);
        }
        if (this->button2Text) {
            SDL_free(this->button2Text);
        }
        if (this->button2Action) {
            SDL_free(this->button2Action);
        }
        if (this->button3Text) {
            SDL_free(this->button3Text);
        }
        if (this->button3Action) {
            SDL_free(this->button3Action);
        }
    }

    char *text;
    char *button1Text;
    char *button1Action;
    char *button2Text;
    char *button2Action;
    char *button3Text;
    char *button3Action;
};

static void MessageDialogInit(void *param, UIDialog *dialog)
{
    DialogInfo *info = (DialogInfo *)param;

    UIElement *message = dialog->GetElement<UIElement>("message");
    if (message) {
        message->SetText(info->text);
    }

    UIElement *button1 = dialog->GetElement<UIElement>("button1");
    if (button1) {
        if (info->button1Text) {
            button1->SetText(info->button1Text);
            button1->Show();
        } else {
            button1->Hide();
        }
        if (info->button1Action) {
            button1->SetAction(info->button1Action);
        } else {
            button1->SetAction(NULL);
        }
    }

    UIElement *button2 = dialog->GetElement<UIElement>("button2");
    if (button2) {
        if (info->button2Text) {
            button2->SetText(info->button2Text);
            button2->Show();
        } else {
            button2->Hide();
        }
        if (info->button2Action) {
            button2->SetAction(info->button2Action);
        } else {
            button2->SetAction(NULL);
        }
    }

    UIElement *button3 = dialog->GetElement<UIElement>("button3");
    if (button3) {
        if (info->button3Text) {
            button3->SetText(info->button3Text);
            button3->Show();
        } else {
            button3->Hide();
        }
        if (info->button3Action) {
            button3->SetAction(info->button3Action);
        } else {
            button3->SetAction(NULL);
        }
    }
}

static void MessageDialogDone(void *param, UIDialog *dialog, int status)
{
    DialogInfo *info = (DialogInfo *)param;

    delete info;
}

void ShowMessage(const char *text,
                 const char *button1Text, const char *button1Action,
                 const char *button2Text, const char *button2Action,
                 const char *button3Text, const char *button3Action)
{
    if (!ui) {
        // Not initialized yet
        return;
    }

    UIDialog *dialog = ui->GetPanel<UIDialog>(DIALOG_MESSAGE);
    if (dialog) {
        // Free any existing message
        ui->HidePanel(dialog);

        DialogInfo *info = new DialogInfo(text, button1Text, button1Action,
                                          button2Text, button2Action,
                                          button3Text, button3Action);
        dialog->SetDialogInitHandler(MessageDialogInit, info);
        dialog->SetDialogDoneHandler(MessageDialogDone, info);
        ui->ShowPanel(dialog);
    }
}

void HideMessage()
{
    UIDialog *dialog = ui->GetPanel<UIDialog>(DIALOG_MESSAGE);
    if (dialog) {
        ui->HidePanel(dialog);
    }
}

static void
hash_nuke_string_font(const void *key, const void *value, void *data)
{
	SDL_free((char*)key);
	fontserv->FreeFont((MFont *)value);
}

static void
hash_nuke_string_text(const void *key, const void *value, void *data)
{
	SDL_free((char*)key);
	fontserv->FreeText((SDL_Texture *)value);
}

MaelstromUI::MaelstromUI(FrameBuf *screen, Prefs *prefs) : UIManager(screen, prefs)
{
	/* Create our font hashtables */
	m_fonts = hash_create(screen, hash_hash_string, hash_keymatch_string, hash_nuke_string_font);
	m_strings = hash_create(screen, hash_hash_string, hash_keymatch_string, hash_nuke_string_text);

	/* Set up some conditions useful for UI loading */
#if defined(SDL_PLATFORM_IOS) || defined(SDL_PLATFORM_ANDROID) || defined(SDL_PLATFORM_EMSCRIPTEN)
	bool quit_available = false;
#else
	bool quit_available = true;
#endif
	if (quit_available) {
		SetCondition("QUIT_AVAILABLE");
	}
	if (gNetworkAvailable) {
		SetCondition("NETWORK_AVAILABLE");
	}
	if (IsPhone()) {
		SetCondition("PHONE");
	}

	/* Load up our UI templates */
	ClearLoadPath();
    if (SDL_GetHintBoolean("MAELSTROM_MODERN_ART", false) &&
        !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false) && !IsPhone()) {
        AddLoadPath("PersonalArt/UI");
    }
	for (unsigned int i = gResolutionIndex; i < gResolutions.length(); ++i) {
		char path[1024];

		SDL_snprintf(path, sizeof(path), "UI%s", gResolutions[i].path_suffix);
		AddLoadPath(path);
	}
	LoadTemplates("UITemplates.xml");
}

MaelstromUI::~MaelstromUI()
{
	Shutdown();
	hash_destroy(m_fonts);
	hash_destroy(m_strings);
}

MFont *
MaelstromUI::GetFont(const char *fontName, int fontSize)
{
	char *key;
	size_t keysize;
	MFont *font;

	keysize = strlen(fontName)+1+2+1;
	key = SDL_stack_alloc(char, keysize);
	SDL_snprintf(key, keysize, "%s:%d", fontName, fontSize);
	if (hash_find(m_fonts, key, (const void**)&font)) {
		SDL_stack_free(key);
		return font;
	}

	font = fontserv->NewFont(fontName, fontSize);
	if (font) {
		/* Add it to our cache */
		hash_insert(m_fonts, SDL_strdup(key), font);
	}
	SDL_stack_free(key);

	return font;
}

// Personal edition: smooth 3x glyphs in the original logical text box.
// Font is bundled OFL Abel, not a system font dependency. Classic stays untouched.
static SDL_Texture *CreatePersonalText(const char *text, MFont *classicFont, Uint8 style, Uint32 color)
{
    struct FontBytes { unsigned char *data = NULL; ~FontBytes() { SDL_free(data); } };
    static FontBytes fontBytes;
    static stbtt_fontinfo font;
    static bool attempted = false, ready = false;
    if (!attempted) {
        attempted = true;
        SDL_IOStream *input = OpenRead("PersonalArt/Fonts/Abel-Regular.ttf");
        if (input) {
            const Sint64 size = SDL_GetIOSize(input);
            if (size > 0 && size < 1024 * 1024) {
                fontBytes.data = (unsigned char *)SDL_malloc((size_t)size);
                if (fontBytes.data && SDL_ReadIO(input, fontBytes.data, (size_t)size) == (size_t)size) {
                    ready = stbtt_InitFont(&font, fontBytes.data, 0) != 0;
                }
            }
            SDL_CloseIO(input);
        }
    }
    if (!ready) { return NULL; }
    const int logicalWidth = fontserv->TextWidth(text, classicFont, style);
    const int logicalHeight = classicFont->header->fRectHeight;
    if (logicalWidth <= 0 || logicalWidth > 4096 || logicalHeight <= 0 || logicalHeight > 128) { return NULL; }
    int points[4096];
    size_t pointCount = 0;
    const char *cursor = text;
    size_t remaining = SDL_strlen(text);
    Uint32 cp;
    while ((cp = SDL_StepUTF8(&cursor, &remaining)) != 0) {
        // Original Geneva maps this legacy copyright marker to the copyright sign.
        if (pointCount == 0 && cp == 'c' && SDL_strncmp(text, "c1992", 5) == 0) { cp = 0xA9; }
        if (!stbtt_FindGlyphIndex(&font, cp) && cp != ' ') { return NULL; }
        if (pointCount == 4096) { return NULL; }
        points[pointCount++] = cp;
    }
    int ascent, descent, gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &gap);
    int advanceTotal = 0;
    for (size_t i = 0; i < pointCount; ++i) {
        int advance, bearing;
        stbtt_GetCodepointHMetrics(&font, points[i], &advance, &bearing);
        advanceTotal += advance;
        if (i + 1 < pointCount) { advanceTotal += stbtt_GetCodepointKernAdvance(&font, points[i], points[i+1]); }
    }
    if (!advanceTotal) { return NULL; }
    const int density = 3, width = logicalWidth * density, height = logicalHeight * density;
    const float sy = (float)(height - 3) / (ascent - descent);
    const float sx = (float)(width - 2) / advanceTotal;
    const int baseline = (int)(ascent * sy);
    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (!surface) { return NULL; }
    SDL_ClearSurface(surface, 0, 0, 0, 0);
    float pen = 1.0f;
    for (size_t i = 0; i < pointCount; ++i) {
        int w, h, ox, oy, advance, bearing;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, sx, sy, points[i], &w, &h, &ox, &oy);
        if (bitmap) {
            for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
                const int px = (int)pen + ox + x, py = baseline + oy + y;
                if (px < 0 || px >= width || py < 0 || py >= height) { continue; }
                Uint8 *dest = (Uint8 *)surface->pixels + py * surface->pitch + px * 4;
                dest[0] = (color >> 16) & 255; dest[1] = (color >> 8) & 255; dest[2] = color & 255;
                dest[3] = SDL_max(dest[3], bitmap[y*w+x]);
                if ((style & STYLE_BOLD) && px + 1 < width) {
                    Uint8 *bold = dest + 4;
                    bold[0] = dest[0]; bold[1] = dest[1]; bold[2] = dest[2];
                    bold[3] = SDL_max(bold[3], bitmap[y*w+x]);
                }
            }
            stbtt_FreeBitmap(bitmap, NULL);
        }
        stbtt_GetCodepointHMetrics(&font, points[i], &advance, &bearing);
        pen += advance * sx;
        if (i + 1 < pointCount) { pen += stbtt_GetCodepointKernAdvance(&font, points[i], points[i+1]) * sx; }
    }
    if (style & STYLE_ULINE) {
        for (int x = 0; x < width; ++x) {
            Uint8 *dest = (Uint8 *)surface->pixels + (height-2) * surface->pitch + x * 4;
            dest[0] = (color >> 16) & 255; dest[1] = (color >> 8) & 255; dest[2] = color & 255; dest[3] = 255;
        }
    }
    SDL_Texture *texture = screen->LoadImage(surface);
    SDL_DestroySurface(surface);
    if (texture) {
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
        SDL_SetNumberProperty(SDL_GetTextureProperties(texture), "personal.text.scale", density);
    }
    return texture;
}
UITexture *
MaelstromUI::CreateText(const char *text, const char *fontName, int fontSize, UIFontStyle fontStyle, Uint32 color)
{
	MFont *font;
	Uint8 style = STYLE_NORM;
	char *key;
	size_t keysize;
	SDL_Texture *texture;

	/* First see if we can find it in our cache */
	keysize = strlen(fontName)+1+2+1+1+1+8+1+strlen(text)+1;
	key = SDL_stack_alloc(char, keysize);
	SDL_snprintf(key, keysize, "%s:%d:%c:%8.8x:%s", fontName, fontSize, '0'+fontStyle, color, text);
	if (hash_find(m_strings, key, (const void**)&texture)) {
		SDL_stack_free(key);
		return new UITexture(texture, (float)SDL_GetNumberProperty(SDL_GetTextureProperties(texture), "personal.text.scale", 1));
	}

	font = GetFont(fontName, fontSize);
	if (!font) {
		error("Couldn't find font %s size %d\n", fontName, fontSize);
		return NULL;
	}

	switch (fontStyle) {
		case UIFONT_STYLE_NORMAL:
			style = STYLE_NORM;
			break;
		case UIFONT_STYLE_BOLD:
			style = STYLE_BOLD;
			break;
		case UIFONT_STYLE_UNDERLINE:
			style = STYLE_ULINE;
			break;
		case UIFONT_STYLE_ITALIC:
			style = STYLE_ITALIC;
			break;
	}

	texture = NULL;
    if (SDL_GetHintBoolean("MAELSTROM_MODERN_ART", false) &&
        !SDL_GetHintBoolean("MAELSTROM_CLASSIC_ART", false)) {
        texture = CreatePersonalText(text, font, style, color);
    }
    if (!texture) { texture = fontserv->TextImage(text, font, style, color); }
	if (texture) {
		/* Add it to our cache */
		hash_insert(m_strings, SDL_strdup(key), texture);
	}
	SDL_stack_free(key);

	return new UITexture(texture, (float)SDL_GetNumberProperty(SDL_GetTextureProperties(texture), "personal.text.scale", 1));
}

void 
MaelstromUI::FreeText(UITexture *texture)
{
	/* Leave the SDL texture in the cache */
	delete texture;
}

UITexture *
MaelstromUI::CreateImage(const char *name)
{
	return Load_Image(screen, name);
}

UITexture *
MaelstromUI::CreateImage(SDL_Surface *surface)
{
	SDL_Texture *texture = screen->LoadImage(surface);
	SDL_DestroySurface(surface);
	return new UITexture(texture, 1.0f);
}

void
MaelstromUI::FreeImage(UITexture *texture)
{
	Free_Texture(screen, texture);
}

UITexture *
MaelstromUI::CreateBackground(const char *name)
{
    UITexture *background;

    //FIXME SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    background = CreateImage(name);
    if (background) {
        int gridSize = background->Width() / 3;
        background->SetStretchGrid(gridSize);
    }

    //FIXME SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    return background;
}

void
MaelstromUI::FreeBackground(UITexture *texture)
{
    FreeImage(texture);
}

void
MaelstromUI::PlaySound(const char *name)
{
	sound->PlaySound(SDL_atoi(name), 5);
}

UIPanel *
MaelstromUI::CreatePanel(const char *type, const char *name)
{
	if (SDL_strcasecmp(type, "Panel") == 0) {
		return new UIPanel(this, name);
	} else if (SDL_strcasecmp(type, "Dialog") == 0) {
		return new MacDialog(ui, name);
	}
	return UIManager::CreatePanel(type, name);
}

UIPanelDelegate *
MaelstromUI::CreatePanelDelegate(UIPanel *panel, const char *delegate)
{
	if (SDL_strcasecmp(delegate, "LoadingPanel") == 0) {
		return new LoadingPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "MainPanel") == 0) {
		return new MainPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "AboutPanel") == 0) {
		return new AboutPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "LobbyDialog") == 0) {
		return new LobbyDialogDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "GamePanel") == 0) {
		return new GamePanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "ContinuePanel") == 0) {
		return new ContinuePanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "GameOverPanel") == 0) {
		return new GameOverPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "ControlsDialog") == 0) {
		return new ControlsDialogDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "ModsDialog") == 0) {
		return new ModsDialogDelegate(panel);
	}
	return UIManager::CreatePanelDelegate(panel, delegate);
}

UIElement *
MaelstromUI::CreateElement(UIBaseElement *parent, const char *type, const char *name)
{
	UIElement *element;

	if (SDL_strcasecmp(type, "Area") == 0) {
		element = new UIElement(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Line") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineLine());
	} else if (SDL_strcasecmp(type, "Rectangle") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineRect());
	} else if (SDL_strcasecmp(type, "Label") == 0) {
		element = new UIElement(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Image") == 0) {
		element = new UIElement(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Container") == 0) {
		element = new UIContainer(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Button") == 0) {
		element = new UIElementButton(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "RoundedButton") == 0) {
		element = new UIElementButton(parent, name, new MacRoundedButton());
	} else if (SDL_strcasecmp(type, "Checkbox") == 0) {
		element = new UIElementCheckbox(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Thumbstick") == 0) {
		element = new UIElementThumbstick(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Icon") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineIcon());
	} else if (SDL_strcasecmp(type, "Sprite") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineSprite());
	} else if (SDL_strcasecmp(type, "Title") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineTitle());
	} else if (SDL_strcasecmp(type, "DialogLabel") == 0) {
		element = new UIElement(parent, name, new MacDialogDrawEngine());
	} else if (SDL_strcasecmp(type, "DialogContainer") == 0) {
		element = new UIContainer(parent, name, new MacDialogDrawEngine());
	} else if (SDL_strcasecmp(type, "DialogButton") == 0) {
		element = new UIDialogButton(parent, name, new MacDialogButton());
	} else if (SDL_strcasecmp(type, "DialogCheckbox") == 0) {
		element = new UIElementCheckbox(parent, name, new MacDialogCheckbox());
	} else if (SDL_strcasecmp(type, "DialogRadioGroup") == 0) {
		element = new UIElementRadioGroup(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "DialogRadioButton") == 0) {
		element = new UIElementRadioButton(parent, name, new MacDialogRadioButton());
	} else if (SDL_strcasecmp(type, "DialogEditbox") == 0) {
		element = new UIElementEditbox(parent, name, new MacDialogEditbox());
	} else {
		element = UIManager::CreateElement(parent, name, type);
	}
	return element;
}

//////////////////////////////////////////////////////////////////////////////
bool
UIDrawEngineIcon::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIDrawEngine::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("id", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'id'", node->name());
		return false;
	}

	UITexture *image = GetCIcon(m_screen, atoi(attr->value()));
	if (!image) {
		error("Unable to load icon %d", atoi(attr->value()));
		return false;
	}

	m_element->SetImage(image);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool
UIDrawEngineSprite::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIDrawEngine::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("id", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'id'", node->name());
		return false;
	}

	UITexture *image = GetSprite(m_screen, atoi(attr->value()), true);
	if (!image) {
		error("Unable to load icon %d", atoi(attr->value()));
		return false;
	}
	m_element->SetImage(image);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool
UIDrawEngineTitle::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIDrawEngine::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("id", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'id'", node->name());
		return false;
	}

	UITexture *image = Load_Title(m_screen, SDL_atoi(attr->value()));
	if (!image) {
		error("Unable to load icon %d", SDL_atoi(attr->value()));
		return false;
	}
	m_element->SetImage(image);

	return true;
}
