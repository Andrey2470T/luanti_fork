// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 sapier

#include "guiEngine.h"

#include "Image/ImageLoader.h"
#include "client/mesh/defaultVertexTypes.h"
#include "client/render/renderer.h"
#include "client/sound/soundopenal.h"
#include "client/ui/glyph_atlas.h"
#include "client/render/rendersystem.h"
#include "client/media/resource.h"
#include "client/render/tilelayer.h"
#include "clientdynamicinfo.h"
#include "config.h"
#include "content/content.h"
#include "content/mods.h"
#include "filesys.h"
#include "gui/IGUIEnvironment.h"
#include "gui/mainmenumanager.h"
#include "guiMainMenu.h"
#include "httpfetch.h"
#include "log.h"
#include "porting.h"
#include "scripting_mainmenu.h"
#include "settings.h"
#include "version.h"
#include "IGUIStaticText.h"
#include "util/tracy_wrapper.h"
#include "client/ui/extra_images.h"

#if USE_SOUND
    #include "client/sound/soundopenal.h"
#endif


/******************************************************************************/
void TextDestGuiEngine::gotText(const StringMap &fields)
{
	m_engine->getScriptIface()->handleMainMenuButtons(fields);
}

/******************************************************************************/
void TextDestGuiEngine::gotText(const std::wstring &text)
{
	m_engine->getScriptIface()->handleMainMenuEvent(wide_to_utf8(text));
}

/******************************************************************************/
/** MenuMusicFetcher                                                          */
/******************************************************************************/
void MenuMusicFetcher::addThePaths(const std::string &name,
		std::vector<std::string> &paths)
{
	// Allow full paths
	if (name.find(DIR_DELIM_CHAR) != std::string::npos) {
		addAllAlternatives(name, paths);
	} else {
		addAllAlternatives(porting::path_share + DIR_DELIM + "sounds" + DIR_DELIM + name, paths);
		addAllAlternatives(porting::path_user + DIR_DELIM + "sounds" + DIR_DELIM + name, paths);
	}
}

/******************************************************************************/
/** GUIEngine                                                                 */
/******************************************************************************/
GUIEngine::GUIEngine(JoystickController *joystick,
        gui::IGUIElement *parent,
        RenderSystem *rndsys, ResourceCache *cache,
        IMenuManager *menumgr,
        MainMenuData *data,
        bool &kill) :
    m_rndsys(rndsys),
	m_parent(parent),
	m_menumanager(menumgr),
	m_data(data),
    m_rescache(cache),
    m_kill(kill),
    m_background(std::make_unique<ImageSprite>(m_rndsys, m_rescache)),
    m_overlay(std::make_unique<ImageSprite>(m_rndsys, m_rescache)),
    m_header(std::make_unique<ImageSprite>(m_rndsys, m_rescache)),
    m_footer(std::make_unique<ImageSprite>(m_rndsys, m_rescache))
{
	// initialize texture pointers
	for (image_definition &texture : m_textures) {
        texture.texture = nullptr;
	}
	// is deleted by guiformspec!
	auto buttonhandler = std::make_unique<TextDestGuiEngine>(this);
	m_buttonhandler = buttonhandler.get();

	// create soundmanager
#if USE_SOUND
	if (g_sound_manager_singleton.get()) {
		m_sound_manager = createOpenALSoundManager(g_sound_manager_singleton.get(),
				std::make_unique<MenuMusicFetcher>());
	}
#endif
	if (!m_sound_manager)
		m_sound_manager = std::make_unique<DummySoundManager>();

	// create topleft header
	m_toplefttext = L"";

    auto default_font = m_rndsys->getFontManager()->getDefaultFont();
    recti rect(0, 0, default_font->getTextWidth(m_toplefttext.c_str()),
        default_font->getTextHeight(m_toplefttext.c_str()));
	rect += v2i(4, 0);

    auto guienv = m_rndsys->getGUIEnvironment();
    m_irr_toplefttext = guienv->addStaticText(m_toplefttext.c_str(), rect, false, true, 0, -1);

	// create formspecsource
	auto formspecgui = std::make_unique<FormspecFormSource>("");
	m_formspecgui = formspecgui.get();

	/* Create menu */
    m_menu = gui::make_gui_shared<GUIFormSpecMenu>(
			joystick,
			m_parent,
			-1,
			m_menumanager,
			nullptr /* &client */,
            guienv,
            m_rescache,
            m_rndsys,
			m_sound_manager.get(),
			formspecgui.release(),
			buttonhandler.release(),
			"",
			false);

	m_menu->allowClose(false);
	m_menu->lockSize(true,v2u(800,600));

	// Initialize scripting

	infostream << "GUIEngine: Initializing Lua" << std::endl;

	m_script = std::make_unique<MainMenuScripting>(this);

	g_settings->registerChangedCallback("fullscreen", fullscreenChangedCallback, this);

	try {
		m_script->setMainMenuData(&m_data->script_data);
		m_data->script_data.errormessage.clear();

		if (!loadMainMenuScript()) {
			errorstream << "No future without main menu!" << std::endl;
			abort();
		}

		run();
	} catch (LuaError &e) {
		errorstream << "Main menu error: " << e.what() << std::endl;
		m_data->script_data.errormessage = e.what();
	}

	m_menu->quitMenu();
	m_menu.reset();
}


/******************************************************************************/
std::string findLocaleFileWithExtension(const std::string &path)
{
    if (fs::exists(path + ".mo"))
		return path + ".mo";
    if (fs::exists(path + ".po"))
		return path + ".po";
    if (fs::exists(path + ".tr"))
		return path + ".tr";
	return "";
}


/******************************************************************************/
std::string findLocaleFileInMods(const std::string &path, const std::string &filename_no_ext)
{
	std::vector<ModSpec> mods = flattenMods(getModsInPath(path, "root", true));

	for (const auto &mod : mods) {
		std::string ret = findLocaleFileWithExtension(
				mod.path + DIR_DELIM "locale" DIR_DELIM + filename_no_ext);
		if (!ret.empty())
			return ret;
	}

	return "";
}

/******************************************************************************/
Translations *GUIEngine::getContentTranslations(const std::string &path,
		const std::string &domain, const std::string &lang_code)
{
	if (domain.empty() || lang_code.empty())
		return nullptr;

	std::string filename_no_ext = domain + "." + lang_code;
	std::string key = path + DIR_DELIM "locale" DIR_DELIM + filename_no_ext;

	if (key == m_last_translations_key)
		return &m_last_translations;

	std::string trans_path = key;

	switch (getContentType(path)) {
	case ContentType::GAME:
		trans_path = findLocaleFileInMods(path + DIR_DELIM "mods" DIR_DELIM,
				filename_no_ext);
		break;
	case ContentType::MODPACK:
		trans_path = findLocaleFileInMods(path, filename_no_ext);
		break;
	default:
		trans_path = findLocaleFileWithExtension(trans_path);
		break;
	}

	if (trans_path.empty())
		return nullptr;

	m_last_translations_key = key;
	m_last_translations = {};

	std::string data;
    if (mt_fs::ReadFile(trans_path, data)) {
        m_last_translations.loadTranslation(mt_fs::GetFilenameFromPath(trans_path.c_str()), data);
	}

	return &m_last_translations;
}

/******************************************************************************/
bool GUIEngine::loadMainMenuScript()
{
	// Set main menu path (for core.get_mainmenu_path())
	m_scriptdir = g_settings->get("main_menu_path");
	if (m_scriptdir.empty()) {
		m_scriptdir = porting::path_share + DIR_DELIM + "builtin" + DIR_DELIM + "mainmenu";
	}

	// Load builtin (which will load the main menu script)
	std::string script = porting::path_share + DIR_DELIM "builtin" + DIR_DELIM "init.lua";
	try {
		m_script->loadScript(script);
		m_script->checkSetByBuiltin();
		// Menu script loaded
		return true;
	} catch (const ModError &e) {
		errorstream << "GUIEngine: execution of menu script failed: "
			<< e.what() << std::endl;
	}

	return false;
}

void setVertex(ByteArray &ba, v2f pos, img::color8 c, v2f uv, u32 n)
{
    ba.setFloat(pos.X, n);
    ba.setFloat(pos.Y, n+1);

    ba.setUInt8(c.R(), n+2);
    ba.setUInt8(c.G(), n+3);
    ba.setUInt8(c.B(), n+4);
    ba.setUInt8(c.A(), n+5);

    ba.setFloat(uv.X, n+6);
    ba.setFloat(uv.Y, n+7);
}

/******************************************************************************/
void GUIEngine::run()
{
    auto font_mgr = m_rndsys->getFontManager();
    auto rnd = m_rndsys->getRenderer();
    u32 text_height = font_mgr->getDefaultFont()->getFontHeight();

	// Reset fog color
	{
        FogType fog_type = FogType::Linear;
		f32 fog_start = 0;
		f32 fog_end = 0;
		f32 fog_density = 0;

        rnd->setFogParams(fog_type, Renderer::menu_sky_color, fog_start, fog_end, fog_density);
	}

	const v2u initial_screen_size(
			g_settings->getU16("screen_w"),
			g_settings->getU16("screen_h")
		);
	const bool initial_window_maximized = !g_settings->getBool("fullscreen") &&
			g_settings->getBool("window_maximized");
    auto last_window_info = ClientDynamicInfo::getCurrent(m_rndsys);

	f32 dtime = 0.0f;

    auto draw_stats = rnd->getDrawStats();
    draw_stats.fps.reset();

	auto framemarker = FrameMarker("GUIEngine::run()-frame").started();

    auto wnd = m_rndsys->getWindow();

    while (m_rndsys->run() && !m_startgame && !m_kill) {
		framemarker.end();
        draw_stats.fps.limit(wnd, &dtime);
		framemarker.start();

        if (wnd->isVisible()) {
			// check if we need to update the "upper left corner"-text
            if (text_height != font_mgr->getDefaultFont()->getFontHeight()) {
				updateTopLeftTextSize();
                text_height = font_mgr->getDefaultFont()->getFontHeight();
			}
            auto window_info = ClientDynamicInfo::getCurrent(m_rndsys);
			if (!window_info.equal(last_window_info)) {
				m_script->handleMainMenuEvent("WindowInfoChange");
				last_window_info = window_info;
			}

            m_rndsys->beginDraw(render::CBF_COLOR | render::CBF_DEPTH, Renderer::menu_sky_color);

            if (m_clouds_enabled) {
                //g_menumgr->drawClouds(dtime);
                //drawOverlay();
            } else {
                //drawBackground();
            }

            //g_menumgr->drawClouds(dtime);

            drawFooter();

            m_rndsys->getGUIEnvironment()->drawAll();

			// The header *must* be drawn after the menu because it uses
			// GUIFormspecMenu::getAbsoluteRect().
			// The header *can* be drawn after the menu because it never intersects
			// the menu.
            drawHeader();

            m_rndsys->endDraw();
		}

		m_script->step();

        sound_volume_control(m_sound_manager.get(), wnd->isActive());
		m_sound_manager->step(dtime);

#ifdef __ANDROID__
		m_menu->getAndroidUIInput();
#endif
	}
	framemarker.end();

	m_script->beforeClose();

    m_rndsys->autosaveScreensizeAndCo(initial_screen_size, initial_window_maximized);
}

/******************************************************************************/
GUIEngine::~GUIEngine()
{
	g_settings->deregisterAllChangedCallbacks(this);

	// deinitialize script first. gc destructors might depend on other stuff
	infostream << "GUIEngine: Deinitializing scripting" << std::endl;
	m_script.reset();

	m_sound_manager.reset();

	m_irr_toplefttext->remove();
}

/******************************************************************************/
void GUIEngine::setFormspecPrepend(const std::string &fs)
{
	if (m_menu) {
		m_menu->setFormspecPrepend(fs);
	}
}


/******************************************************************************/
void GUIEngine::drawBackground()
{
    v2u screensize = m_rndsys->getWindowSize();

	img::Image* texture = m_textures[TEX_LAYER_BACKGROUND].texture;

	/* If no texture, draw background of solid color */
	if(!texture){
        img::color8 color(img::PF_RGBA8,80,58,37,255);
		recti rect(0, 0, screensize.X, screensize.Y);
        m_background->update(nullptr, toRectf(rect), color);
        m_background->draw();
		return;
	}

	v2u sourcesize = texture->getSize();

	if (m_textures[TEX_LAYER_BACKGROUND].tile)
	{
		v2u tilesize(
				MYMAX(sourcesize.X,m_textures[TEX_LAYER_BACKGROUND].minsize),
				MYMAX(sourcesize.Y,m_textures[TEX_LAYER_BACKGROUND].minsize));
		for (unsigned int x = 0; x < screensize.X; x += tilesize.X )
		{
			for (unsigned int y = 0; y < screensize.Y; y += tilesize.Y )
			{
                m_background->update(texture, rectf(x, y, x+tilesize.X, y+tilesize.Y), img::white);
                m_background->draw();
			}
		}
		return;
	}

	// Chop background image to the smaller screen dimension
	v2u bg_size = screensize;
	v2f scale(
			(f32) bg_size.X / sourcesize.X,
			(f32) bg_size.Y / sourcesize.Y);
	if (scale.X < scale.Y)
		bg_size.X = (int) (scale.Y * sourcesize.X);
	else
		bg_size.Y = (int) (scale.X * sourcesize.Y);
	v2i offset = v2i(
		(s32) screensize.X - (s32) bg_size.X,
		(s32) screensize.Y - (s32) bg_size.Y
	) / 2;
	/* Draw background texture */
    m_background->update(texture, rectf(offset.X, offset.Y, bg_size.X + offset.X, bg_size.Y + offset.Y), img::white);
    m_background->draw();
}

/******************************************************************************/
void GUIEngine::drawOverlay()
{
    v2u screensize = m_rndsys->getWindowSize();

	img::Image* texture = m_textures[TEX_LAYER_OVERLAY].texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	/* Draw background texture */
    m_background->update(texture, rectf(0, 0, screensize.X, screensize.Y), img::white);
    m_background->draw();
}

/******************************************************************************/
void GUIEngine::drawHeader()
{
    v2u screensize = m_rndsys->getWindowSize();

    //img::Image* texture = m_textures[TEX_LAYER_HEADER].texture;
    auto header_img = m_rescache->get<img::Image>(ResourceType::IMAGE, "menu_header.png");

	// If no texture, draw nothing
    if (!header_img)
		return;

	/*
	 * Calculate the maximum rectangle
	 */
	recti formspec_rect = m_menu->getAbsoluteRect();
	// 4 px of padding on each side
	recti max_rect(4, 4, screensize.X - 8, formspec_rect.ULC.Y - 8);

	// If no space (less than 16x16 px), draw nothing
    //if (max_rect.getWidth() < 16 || max_rect.getHeight() < 16)
    //	return;

	/*
	 * Calculate the preferred rectangle
	 */
	f32 mult = (((f32)screensize.X / 2.0)) /
            ((f32)header_img->getSize().X);

    v2i splashsize(((f32)header_img->getSize().X) * mult,
            ((f32)header_img->getSize().Y) * mult);

	s32 free_space = (((s32)screensize.Y)-320)/2;

	recti desired_rect(0, 0, splashsize.X, splashsize.Y);
	desired_rect += v2i((screensize.X/2)-(splashsize.X/2),
			((free_space/2)-splashsize.Y/2)+10);

	/*
	 * Make the preferred rectangle fit into the maximum rectangle
	 */
	// 1. Scale
	f32 scale = std::min((f32)max_rect.getWidth() / (f32)desired_rect.getWidth(),
			(f32)max_rect.getHeight() / (f32)desired_rect.getHeight());
	if (scale < 1.0f) {
		v2i old_center = desired_rect.getCenter();
		desired_rect.LRC.X = desired_rect.ULC.X + desired_rect.getWidth() * scale;
		desired_rect.LRC.Y = desired_rect.ULC.Y + desired_rect.getHeight() * scale;
		desired_rect += old_center - desired_rect.getCenter();
	}

	// 2. Move
	desired_rect.constrainTo(max_rect);

    m_background->update(header_img, toRectf(desired_rect), img::white);
    m_background->draw();
}

/******************************************************************************/
void GUIEngine::drawFooter()
{
    v2u screensize = m_rndsys->getWindowSize();

	img::Image* texture = m_textures[TEX_LAYER_FOOTER].texture;

	/* If no texture, draw nothing */
	if(!texture)
		return;

	f32 mult = (((f32)screensize.X)) /
			((f32)texture->getSize().X);

	v2i footersize(((f32)texture->getSize().X) * mult,
			((f32)texture->getSize().Y) * mult);

	// Don't draw the footer if there isn't enough room
	s32 free_space = (((s32)screensize.Y)-320)/2;

	if (free_space > footersize.Y) {
		recti rect(0,0,footersize.X,footersize.Y);
		rect += v2i(screensize.X/2,screensize.Y-footersize.Y);
		rect -= v2i(footersize.X/2, 0);

        m_background->update(texture, toRectf(rect), img::white);
        m_background->draw();
	}
}

/******************************************************************************/
bool GUIEngine::setTexture(texture_layer layer, const std::string &texturepath,
		bool tile_image, unsigned int minsize)
{
    if (texturepath.empty() || !fs::exists(texturepath)) {
		return false;
	}

    m_textures[layer].texture = m_rescache->getOrLoad<img::Image>(ResourceType::IMAGE, texturepath);
	m_textures[layer].tile    = tile_image;
	m_textures[layer].minsize = minsize;

	if (!m_textures[layer].texture) {
		return false;
	}

	return true;
}

/******************************************************************************/
bool GUIEngine::downloadFile(const std::string &url, const std::string &target)
{
#if USE_CURL
	auto target_file = open_ofstream(target.c_str(), true);
	if (!target_file.good())
		return false;

	HTTPFetchRequest fetch_request;
	HTTPFetchResult fetch_result;
	fetch_request.url = url;
	fetch_request.caller = HTTPFETCH_SYNC;
	fetch_request.timeout = std::max(MIN_HTTPFETCH_TIMEOUT,
		(long)g_settings->getS32("curl_file_download_timeout"));
	bool completed = httpfetch_sync_interruptible(fetch_request, fetch_result);

	if (!completed || !fetch_result.succeeded) {
		target_file.close();
        mt_fs::DeleteSingleFileOrEmptyDirectory(target);
		return false;
	}
	// TODO: directly stream the response data into the file instead of first
	// storing the complete response in memory
	target_file << fetch_result.data;

	return true;
#else
	return false;
#endif
}

/******************************************************************************/
void GUIEngine::setTopleftText(const std::string &text)
{
	m_toplefttext = translate_string(utf8_to_wide(text));

	updateTopLeftTextSize();
}

/******************************************************************************/
void GUIEngine::updateTopLeftTextSize()
{
    auto default_font = m_rndsys->getFontManager()->getDefaultFont();
    recti rect(0, 0, default_font->getTextWidth(m_toplefttext.c_str()),
        default_font->getTextHeight(m_toplefttext.c_str()));
	rect += v2i(4, 0);

	m_irr_toplefttext->remove();
    m_irr_toplefttext = m_rndsys->getGUIEnvironment()->addStaticText(
            m_toplefttext.c_str(), rect, false, false, 0, -1);
}

/******************************************************************************/
void GUIEngine::fullscreenChangedCallback(const std::string &name, void *data)
{
	static_cast<GUIEngine*>(data)->getScriptIface()->handleMainMenuEvent("FullscreenChange");
}
