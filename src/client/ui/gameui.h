// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#pragma once

#include "text_sprite.h"
#include "util/numeric.h"
#include <list>

class Client;
class EnrichedString;
class GUIChatConsole;
struct PointedThing;
class Hud;
class RenderSystem;
class ResourceCache;
struct FpsControl;
class ProfilerGraphSet;
class Nametag;
class LocalPlayer;

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
    GameUI(Client *client);

	void init();
    void update(Client *client, const GUIChatConsole *chat_console, f32 dtime);

    const u8 &getFlags() const
    {
        return flags;
    }

    Hud *getHud() const
    {
        return hud.get();
    }
    ProfilerGraphSet *getProfilerGraphs() const
    {
        return graph_set.get();
    }

    void setInfoText(const std::wstring &str) {
        infotext->setText(str);
    }
    void clearInfoText() {
        infotext->setText(L"");
    }

    void showStatusText(const std::wstring &str)
	{
        statustext->setText(str);
        statustext_time = 0.0f;
	}
	void showTranslatedStatusText(const char *str);
    void clearStatusText() {
        statustext->setText(L"");
    }

    void showDebug();

	bool isChatVisible()
	{
        return flags & GUIF_SHOW_CHAT && recent_chat_count != 0 && profiler_current_page == 0;
	}
    void setChatText(const EnrichedString &chat_text, u32 _recent_chat_count);
    void updateChat();

	void updateProfiler();

    void toggleMinimalDebug();
	void toggleChat(Client *client);
	void toggleHud();
    u8 toggleDebug(LocalPlayer *player);
	void toggleProfiler();

	void clearText();

    void render();

    void updateDebugState(Client *client);
    void updateProfilers(f32 dtime);
    void updateProfilerGraphs();
    
    Nametag *addNametag(
            Client *client,
            const std::string &text,
            const img::color8 &textcolor,
            const std::optional<img::color8> &bgcolor,
            const v3f &pos);
    void removeNametag(Nametag *nt);

private:
    void toggleFlag(GameUIFlags flag)
    {
        if (flags & flag)
            flags &= ~flag;
        else
            flags |= flag;
    }
    void enableFlag(GameUIFlags flag, bool enable)
    {
        if (enable)
            flags |= flag;
        else
            flags &= ~flag;
    }
    RenderSystem *rndsys;
    ResourceCache *cache;

    u8 flags = GUIF_SHOW_CHAT | GUIF_SHOW_HUD;

    float drawtime_avg = 0;

    bool enable_noclip;

    u8 debug_state = 0;

    IntervalLimiter profiler_interval;

    UITextSprite *minimal_debugtext; // The upper debug text line
    UITextSprite *basic_debugtext; // The lower debug text line

    UITextSprite *infotext; // At the middle of the screen
    //std::wstring m_infotext;

    UITextSprite *statustext;
    std::wstring last_status_text;
    f32 statustext_time = 0.0f;
    img::color8 statustext_initial_color = img::black;

    UITextSprite *chattext; // Chat text
    u32 recent_chat_count;

    UITextSprite *profilertext; // Profiler text
    u8 profiler_current_page = 0;
    const u8 profiler_max_page = 3;
    
    std::list<std::unique_ptr<Nametag>> nametags;

    std::unique_ptr<ProfilerGraphSet> graph_set;

    std::unique_ptr<SpriteDrawBatch> drawBatch;

    std::unique_ptr<Hud> hud;
};
