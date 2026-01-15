// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#include "gameui.h"
#include <gettext.h>
#include "client/map/clientmap.h"
#include "client/network/packethandler.h"
#include "client/player/interaction.h"
#include "client/render/renderer.h"
#include "gui/IGUIStaticText.h"
#include "gui/mainmenumanager.h"
#include "gui/guiChatConsole.h"
#include "client/ui/profilergraph.h"
#include "gui/touchcontrols.h"
#include "util/enriched_string.h"
#include "util/pointedthing.h"
#include "hud.h" // HUD_FLAG_*
#include "hud_elements.h"
#include "nodedef.h"
#include "profiler.h"
#include "client/core/client.h"
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "glyph_atlas.h"
#include "version.h"
#include "settings.h"
#include "client/render/drawlist.h"
#include "client/player/selection.h"
#include "client/ui/gameformspec.h"
#include "client/ao/nametag.h"

inline static const char *yawToDirectionString(int yaw)
{
	static const char *direction[4] =
		{"North +Z", "West -X", "South -Z", "East +X"};

	yaw = wrapDegrees_0_360(yaw);
	yaw = (yaw + 45) % 360 / 90;

	return direction[yaw];
}

GameUI::GameUI(Client *client)
    : rndsys(client->getRenderSystem()),
        drawBatch(std::make_unique<SpriteDrawBatch>(rndsys, client->getResourceCache())),
        hud(std::make_unique<Hud>(client, drawBatch.get()))
{
    auto guienv = rndsys->getGUIEnvironment();
	if (guienv && guienv->getSkin())
        statustext_initial_color = guienv->getSkin()->getColor(GUIDefaultColor::ButtonText);

    enable_noclip = g_settings->getBool("noclip");
}
void GameUI::init()
{
    auto font_mgr = rndsys->getFontManager();

    // First line of debug text
    minimal_debugtext = drawBatch->addTextSprite(utf8_to_wide(PROJECT_NAME_C), v2f(5, 5));
    // Second line of debug text
    basic_debugtext = drawBatch->addTextSprite(L"", v2f(5, 5 + minimal_debugtext->getTextHeight()));

	// Chat text
    chattext = drawBatch->addTextSprite(L"", std::nullopt, img::white, nullptr, 0, true);
    chattext->getTextObj().enableOverrideColor(false);
	u16 chat_font_size = g_settings->getU16("chat_font_size");
	if (chat_font_size != 0) {
        chattext->getTextObj().setOverrideFont(font_mgr->getFontOrCreate(
            render::FontMode::GRAY, render::FontStyle::NORMAL, std::clamp<u16>(chat_font_size, 5, 72)));
	}

	// Infotext of nodes and objects.
	// If in debug mode, object debug infos shown here, too.
	// Located on the left on the screen, below chat.
    u32 chat_font_height = chattext->getLineHeight();
    infotext = drawBatch->addTextSprite(L"", v2f(100, chat_font_height * (g_settings->getU16("recent_chat_messages") + 3)));

	// Status text (displays info when showing and hiding GUI stuff, etc.)
    last_status_text = L"<Status>";
    statustext = drawBatch->addTextSprite(last_status_text);
    statustext->setVisible(false);

	// Profiler text (size is updated when text is updated)
    profilertext = drawBatch->addTextSprite(L"<Profiler>");
    profilertext->getTextObj().setOverrideFont(font_mgr->getFontOrCreate(render::FontMode::GRAY, render::FontStyle::NORMAL,
        font_mgr->getDefaultFontSize(render::FontMode::GRAY) * 0.9f));
    profilertext->setVisible(false);

    graph_set = std::make_unique<ProfilerGraphSet>(rndsys, cache);
    graph_set->addGraph("Draw scene [us]", 0, img::color8(img::PF_RGBA8, 255, 100, 100, 255));
    graph_set->addGraph("FPS", 1, img::color8(img::PF_RGBA8, 90, 225, 90, 255));
    graph_set->addGraph("Sleep [us]", 2, img::color8(img::PF_RGBA8, 100, 100, 255, 255));
    graph_set->addGraph("Time non-rendering [us]", 3, img::color8(img::PF_RGBA8, 255, 150, 50, 255));
    graph_set->addGraph("client_received_packets", 4, img::color8(img::PF_RGBA8, 220, 220, 100, 255));
    graph_set->addGraph("num_processed_meshes", 5, img::color8(img::PF_RGBA8, 200, 200, 200, 255));
    graph_set->addGraph("packets_lost", 6, img::color8(img::PF_RGBA8, 200, 200, 200, 255));
    graph_set->addGraph("rudp RTT [ms]", 7, img::color8(img::PF_RGBA8, 200, 200, 200, 255));
    graph_set->addGraph("rudp jitter [ms]", 8, img::color8(img::PF_RGBA8, 200, 200, 200, 255));

    showDebug();
}

void GameUI::update(Client *client, const GUIChatConsole *chat_console, f32 dtime)
{
    v2u wndSize = rndsys->getWindowSize();

	LocalPlayer *player = client->getEnv().getLocalPlayer();
    PlayerInteraction *interaction = player->getInteraction();
    v2f cam_orient = player->getCamera()->getOrientation();

    auto drawstats = rndsys->getRenderer()->getDrawStats();
    auto draw_control = rndsys->getDrawList()->getDrawControl();

	// Minimal debug text must only contain info that can't give a gameplay advantage
    if (flags & GUIF_SHOW_MINIMAL_DEBUG) {
        const u16 fps = 1.0 / drawstats.dtime_jitter.avg;
        drawtime_avg *= 0.95f;
        drawtime_avg += 0.05f * (drawstats.drawtime / 1000);

		std::ostringstream os(std::ios_base::binary);
		os << std::fixed
			<< PROJECT_NAME_C " " << g_version_hash
			<< " | FPS: " << fps
			<< std::setprecision(fps >= 100 ? 1 : 0)
            << " | drawtime: " << drawtime_avg << "ms"
			<< std::setprecision(1)
			<< " | dtime jitter: "
            << (drawstats.dtime_jitter.max_fraction * 100.0f) << "%"
			<< std::setprecision(1)
			<< " | view range: "
            << (draw_control.range_all ? "All" : itos(draw_control.wanted_range))
			<< std::setprecision(2)
            << " | RTT: " << (client->getPacketHandler()->getRTT() * 1000.0f) << "ms";

        std::wstring text = utf8_to_wide(os.str());
        minimal_debugtext->setText(text);
        minimal_debugtext->setBoundRect(
            rectf(v2f(5, 5), minimal_debugtext->getTextWidth(),
            minimal_debugtext->getTextHeight()));
    }

	// Finally set the guitext visible depending on the flag
    minimal_debugtext->setVisible(flags & GUIF_SHOW_MINIMAL_DEBUG);

	// Basic debug text also shows info that might give a gameplay advantage
    if (flags & GUIF_SHOW_BASIC_DEBUG) {
		v3f player_position = player->getPosition();

		std::ostringstream os(std::ios_base::binary);
		os << std::setprecision(1) << std::fixed
			<< "pos: (" << (player_position.X / BS)
			<< ", " << (player_position.Y / BS)
			<< ", " << (player_position.Z / BS)
            << ") | yaw: " << (wrapDegrees_0_360(cam_orient.Y)) << "° "
            << yawToDirectionString(cam_orient.Y)
            << " | pitch: " << (-wrapDegrees_180(cam_orient.X)) << "°"
			<< " | seed: " << ((u64)client->getMapSeed());

        if (interaction->pointed_old.type == POINTEDTHING_NODE) {
			ClientMap &map = client->getEnv().getClientMap();
			const NodeDefManager *nodedef = client->getNodeDefManager();
            MapNode n = map.getNode(interaction->pointed_old.node_undersurface);

			if (n.getContent() != CONTENT_IGNORE) {
				if (nodedef->get(n).name == "unknown") {
					os << ", pointed: <unknown node>";
				} else {
					os << ", pointed: " << nodedef->get(n).name;
				}
				os << ", param2: " << (u64) n.getParam2();
			}
		}

        std::wstring text = utf8_to_wide(os.str());

        basic_debugtext->setText(text);
        basic_debugtext->setBoundRect(
            rectf(v2f(5, 5 + basic_debugtext->getTextHeight()),
            basic_debugtext->getTextWidth(), basic_debugtext->getTextHeight()));
    }

    basic_debugtext->setVisible(flags & GUIF_SHOW_BASIC_DEBUG);

    // Info text
    infotext->setVisible(flags & GUIF_SHOW_HUD && g_menumgr->menuCount() == 0);

    // Status text
    static const f32 statustext_time_max = 1.5f;

    if (!statustext->getText().empty()) {
        statustext_time += dtime;

        if (statustext_time >= statustext_time_max) {
			clearStatusText();
            statustext_time = 0.0f;
		}

	}

    UITextSprite *guitext_status = nullptr;
	bool overriden = g_touchcontrols && g_touchcontrols->isStatusTextOverriden();
	if (overriden) {
		guitext_status = g_touchcontrols->getStatusText();
        statustext->setVisible(false);
	} else {
        guitext_status = statustext;
		if (g_touchcontrols)
			g_touchcontrols->getStatusText()->setVisible(false);
	}

    guitext_status->setVisible(!guitext_status->getText().empty());

    if (!statustext->getText().empty()) {
        s32 status_width  = guitext_status->getTextWidth();
        s32 status_height = guitext_status->getTextHeight();
        s32 status_y = wndSize.Y  - (overriden ? 15 : 150);
        s32 status_x = (wndSize.X - status_width) / 2;

        v2f guitext_status_shift(status_x, status_y - status_height);
        guitext_status->getShape().move(guitext_status_shift);
        //guitext_status->setRelativePosition(recti(status_x ,
        //	status_y - status_height, status_x + status_width, status_y));

		// Fade out
        auto fade_color = statustext_initial_color;
        f32 d = statustext_time / statustext_time_max;
        fade_color.A(static_cast<u32>(
            fade_color.A() * (1.0f - d * d)));
        guitext_status->getTextObj().setOverrideColor(fade_color);
        guitext_status->setBoundRect(
            rectf(guitext_status_shift, guitext_status_shift+toV2T<f32>(guitext_status->getTextSize())));
    }

	// Hide chat when disabled by server or when console is visible
    chattext->setVisible(isChatVisible() && !chat_console->isVisible() && (player->hud_flags & HUD_FLAG_CHAT_VISIBLE));

    hud->setHudVisible(flags & GUIF_SHOW_HUD);
}

void GameUI::showTranslatedStatusText(const char *str)
{
	showStatusText(wstrgettext(str));
}

void GameUI::showDebug()
{
    if (g_settings->getBool("show_debug")) {
        debug_state = true;
        toggleMinimalDebug();
    }
}

void GameUI::setChatText(const EnrichedString &chat_text, u32 _recent_chat_count)
{
    chattext->setText(chat_text);
    recent_chat_count = _recent_chat_count;
}

void GameUI::updateChat()
{
	// Update gui element size and position
    f32 chat_y = 5;

    if (flags & GUIF_SHOW_MINIMAL_DEBUG)
        chat_y += minimal_debugtext->getTextHeight();
    if (flags & GUIF_SHOW_BASIC_DEBUG)
        chat_y += basic_debugtext->getTextHeight();


    const v2u wndSize = rndsys->getWindowSize();
    rectf chatSize(
        v2f(10, chat_y),
        v2f(wndSize.X - 20, std::min<f32>(wndSize.Y, chattext->getTextHeight() + chat_y))
    );

    chattext->setBoundRect(chatSize);
}

void GameUI::updateProfiler()
{
    if (profiler_current_page != 0) {
		std::ostringstream os(std::ios_base::binary);
        os << "   Profiler page " << (s32)profiler_current_page <<
				", elapsed: " << g_profiler->getElapsedMs() << " ms)" << std::endl;

        g_profiler->print(os, profiler_current_page, profiler_max_page);

        profilertext->getTextObj().setOverrideColor(img::color8(img::PF_RGBA8, 0, 0, 0, 120));
        profilertext->setText(utf8_to_wide(os.str()));

        v2f size = toV2T<f32>(profilertext->getTextSize());
        v2f upper_left(6, minimal_debugtext->getTextHeight() * 2.5f);
        v2f lower_right = upper_left;
        lower_right.X += size.X + 10;
        lower_right.Y += size.Y;

        profilertext->setBoundRect(rectf(upper_left, lower_right));
	}

    profilertext->setVisible(profiler_current_page != 0);
}

void GameUI::toggleMinimalDebug()
{
    toggleFlag(GUIF_SHOW_MINIMAL_DEBUG);
}

void GameUI::toggleChat(Client *client)
{
	if (client->getEnv().getLocalPlayer()->hud_flags & HUD_FLAG_CHAT_VISIBLE) {
        toggleFlag(GUIF_SHOW_CHAT);
        if (flags & GUIF_SHOW_CHAT)
			showTranslatedStatusText("Chat shown");
		else
			showTranslatedStatusText("Chat hidden");
	} else {
		showTranslatedStatusText("Chat currently disabled by game or mod");
	}

}

void GameUI::toggleHud()
{
    toggleFlag(GUIF_SHOW_HUD);
    if (flags & GUIF_SHOW_HUD)
		showTranslatedStatusText("HUD shown");
	else
		showTranslatedStatusText("HUD hidden");
}

u8 GameUI::toggleDebug(LocalPlayer *player)
{
    bool has_debug = player->checkPrivilege("debug");
    bool has_basic_debug = has_debug || (player->hud_flags & HUD_FLAG_BASIC_DEBUG);

    // Initial: No debug info
    // 1x toggle: Debug text
    // 2x toggle: Debug text with profiler graph
    // 3x toggle: Debug text and wireframe (needs "debug" priv)
    // 4x toggle: Debug text and bbox (needs "debug" priv)
    //
    // The debug text can be in 2 modes: minimal and basic.
    // * Minimal: Only technical client info that not GameInputSystemplay-relevant
    // * Basic: Info that might give GameInputSystemplay advantage, e.g. pos, angle
    // Basic mode is used when player has the debug HUD flag set,
    // otherwise the Minimal mode is used.
    debug_state = (debug_state + 1) % 5;
    if (debug_state >= 3 && !has_debug)
        debug_state = 0;

    enableFlag(GUIF_SHOW_MINIMAL_DEBUG, debug_state > 0);
    enableFlag(GUIF_SHOW_BASIC_DEBUG, debug_state > 0 && has_basic_debug);
    enableFlag(GUIF_SHOW_PROFILER_GRAPH, debug_state == 2);

    return debug_state;
}

void GameUI::toggleProfiler()
{
    profiler_current_page = (profiler_current_page + 1) % (profiler_max_page + 1);

	// FIXME: This updates the profiler with incomplete values
	updateProfiler();

    if (profiler_current_page != 0) {
		std::wstring msg = fwgettext("Profiler shown (page %d of %d)",
                profiler_current_page, profiler_max_page);
		showStatusText(msg);
	} else {
		showTranslatedStatusText("Profiler hidden");
	}
}

void GameUI::clearText()
{
    minimal_debugtext->setText(L"");
    basic_debugtext->setText(L"");
    infotext->setText(L"");
    statustext->setText(L"");
    chattext->setText(L"");
    profilertext->setText(L"");
}

void GameUI::render()
{
    hud->updateCrosshair();
    hud->updateBuiltinElements();

    drawBatch->rebuild();
    drawBatch->draw();
    hud->renderMinimaps();
    
    //for (auto &nt : nametags)
    //    nt->draw();

    graph_set->draw();
}

void GameUI::updateDebugState(Client *client)
{
    LocalPlayer *player = client->getEnv().getLocalPlayer();

    // debug UI and wireframe
    bool has_debug = player->checkPrivilege("debug");
    bool has_basic_debug = has_debug || (player->hud_flags & HUD_FLAG_BASIC_DEBUG);

    if (flags & GUIF_SHOW_BASIC_DEBUG) {
        if (!has_basic_debug)
            toggleFlag(GUIF_SHOW_BASIC_DEBUG);
    } else if (flags & GUIF_SHOW_MINIMAL_DEBUG) {
        if (has_basic_debug)
            flags |= GUIF_SHOW_BASIC_DEBUG;
    }

    auto drawlist = client->getRenderSystem()->getDrawList();
    if (!has_basic_debug)
        drawlist->getBlockBounds()->disable();
    if (!has_debug) {
        drawlist->getDrawControl().show_wireframe = false;
        player->getCamera()->disable_update = false;
        client->getRenderSystem()->getGameFormSpec()->disableDebugView();
    }

    // noclip
    drawlist->getDrawControl().allow_noclip = enable_noclip && player->checkPrivilege("noclip");
}

void GameUI::updateProfilers(f32 dtime)
{
    float profiler_print_interval =
            g_settings->getFloat("profiler_print_interval");
    bool print_to_log = true;

    // Update game UI anyway but don't log
    if (profiler_print_interval <= 0) {
        print_to_log = false;
        profiler_print_interval = 3;
    }

    if (profiler_interval.step(dtime, profiler_print_interval)) {
        if (print_to_log) {
            infostream << "Profiler:" << std::endl;
            g_profiler->print(infostream);
        }

        updateProfiler();
        g_profiler->clear();
    }

    auto drawstats = rndsys->getRenderer()->getDrawStats();
    // Update graphs
    g_profiler->graphAdd("Time non-rendering [us]",
        drawstats.fps.busy_time - drawstats.drawtime);
    g_profiler->graphAdd("Sleep [us]", drawstats.fps.sleep_time);

    g_profiler->graphSet("FPS", 1.0f / dtime);

    g_profiler->avg("Irr: drawcalls", drawstats.drawcalls);
    if (drawstats.drawcalls > 0)
        g_profiler->avg("Irr: primitives per drawcall",
            drawstats.drawn_primitives / float(drawstats.drawcalls));
    //g_profiler->avg("Irr: HW buffers uploaded", stats2.HWBuffersUploaded);
    //g_profiler->avg("Irr: HW buffers active", stats2.HWBuffersActive);
}

/* Log times and stuff for visualization */
void GameUI::updateProfilerGraphs()
{
    Profiler::GraphValues values;
    g_profiler->graphPop(values);
    graph_set->put(values);

    graph_set->setVisible(flags & GUIF_SHOW_PROFILER_GRAPH);
}

Nametag *GameUI::addNametag(
            Client *client,
            const std::string &text,
            const img::color8 &textcolor,
            const std::optional<img::color8> &bgcolor,
            const v3f &pos)
{
    nametags.emplace_back(std::make_unique<Nametag>(client, text, textcolor, bgcolor, pos));
	return nametags.back().get();
}
void GameUI::removeNametag(Nametag *nt)
{
    auto found = std::find_if(nametags.begin(), nametags.end(),
	    [nt] (const std::unique_ptr<Nametag> &cur_nt)
	    {
		    return nt == cur_nt.get();
		});
		
	if (found != nametags.end())
	    nametags.erase(found);
}
