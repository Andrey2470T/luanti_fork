// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#pragma once

#include "text_sprite.h"

class Client;
class EnrichedString;
class GUIChatConsole;
struct MapDrawControl;
struct PointedThing;
struct RunStats;
struct CameraOrientation;
class Hud;
class RenderSystem;
class ResourceCache;

/*
 * This object intend to contain the core UI elements
 * It includes:
 *   - status texts
 *   - debug texts
 *   - chat texts
 *   - hud flags
 */

// Flags that can, or may, change during main game loop
enum GameUIFlags
{
    GUIF_SHOW_CHAT = 0x1,
    GUIF_SHOW_HUD = 0x2,
    GUIF_SHOW_MINIMAL_DEBUG = 0x4,
    GUIF_SHOW_BASIC_DEBUG = 0x8,
    GUIF_SHOW_PROFILER_GRAPH = 0x10
};

class GameUI
{
	// Temporary between coding time to move things here
	friend class Game;

	// Permit unittests to access members directly
	friend class TestGameUI;

public:
    GameUI(RenderSystem *_rndsys);

	// Flags that can, or may, change during main game loop
    enum Flags
	{
        FLAGS_SHOW_CHAT = 0x1,
        FLAGS_SHOW_HUD = 0x2,
        FLAGS_SHOW_MINIMAL_DEBUG = 0x4,
        FLAGS_SHOW_BASIC_DEBUG = 0x8,
        FLAHS_SHOW_PROFILER_GRAPH = 0x10
	};

	void init();
	void update(const RunStats &stats, Client *client, MapDrawControl *draw_control,
			const CameraOrientation &cam, const PointedThing &pointed_old,
			const GUIChatConsole *chat_console, float dtime);

    const u8 &getFlags() const
    {
        return flags;
    }

    inline void setInfoText(const std::wstring &str) {
        infotext->setText(str);
    }
    inline void clearInfoText() {
        infotext->setText(L"");
    }

	inline void showStatusText(const std::wstring &str)
	{
        statustext->setText(str);
        statustext_time = 0.0f;
	}
	void showTranslatedStatusText(const char *str);
    inline void clearStatusText() {
        statustext->setText(L"");
    }

	bool isChatVisible()
	{
        return flags & GUIF_SHOW_CHAT && recent_chat_count != 0 && profiler_current_page == 0;
	}
    void setChatText(const EnrichedString &chat_text, u32 _recent_chat_count);
	void updateChatSize();

	void updateProfiler();

	void toggleChat(Client *client);
	void toggleHud();
	void toggleProfiler();

	void clearText();

private:
    void toggleFlag(GameUIFlags flag)
    {
        if (flags & flag)
            flags &= ~flag;
        else
            flags |= flag;
    }
    RenderSystem *rndsys;
    ResourceCache *cache;

    std::unique_ptr<Hud> hud;

    u8 flags = GUIF_SHOW_CHAT | GUIF_SHOW_HUD;

    float drawtime_avg = 0;

    std::unique_ptr<UISpriteBank> debugtext;  // First and second lines of debug text
    std::wstring first_debug_line;
    std::wstring second_debug_line;

    std::unique_ptr<UITextSprite> infotext; // At the middle of the screen
    //std::wstring m_infotext;

    std::unique_ptr<UITextSprite> statustext;
    std::wstring status_line;
    f32 statustext_time = 0.0f;
    img::color8 statustext_initial_color = img::black;

    std::unique_ptr<UITextSprite> chattext; // Chat text
    u32 recent_chat_count = 0;
    rectf current_chat_size{0, 0, 0, 0};

    std::unique_ptr<UITextSprite> profilertext; // Profiler text
    u8 profiler_current_page = 0;
    const u8 profiler_max_page = 3;
};
