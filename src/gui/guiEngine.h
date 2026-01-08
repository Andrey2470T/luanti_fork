// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 sapier

#pragma once

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "guiFormSpecMenu.h"
#include "client/render/clouds.h"
#include "client/sound/sound.h"
#include "util/enriched_string.h"
#include "translation.h"
#include "guiSharedPointer.h"

/******************************************************************************/
/* Structs and macros                                                         */
/******************************************************************************/
/** texture layer ids */
enum texture_layer {
	TEX_LAYER_BACKGROUND = 0,
	TEX_LAYER_OVERLAY,
	TEX_LAYER_HEADER,
	TEX_LAYER_FOOTER,
	TEX_LAYER_MAX
};

struct image_definition {
	img::Image *texture = nullptr;
	bool             tile;
	unsigned int     minsize;
};

/******************************************************************************/
/* forward declarations                                                       */
/******************************************************************************/
class GUIEngine;
class RenderSystem;
class MainMenuScripting;
class ResourceCache;
struct MainMenuData;
class ImageSprite;

/******************************************************************************/
/* declarations                                                               */
/******************************************************************************/

/** GUIEngine specific implementation of TextDest used within guiFormSpecMenu */
class TextDestGuiEngine : public TextDest
{
public:
	/**
	 * default constructor
	 * @param engine the engine data is transmitted for further processing
	 */
	TextDestGuiEngine(GUIEngine* engine) : m_engine(engine) {};

	/**
	 * receive fields transmitted by guiFormSpecMenu
	 * @param fields map containing formspec field elements currently active
	 */
	void gotText(const StringMap &fields);

	/**
	 * receive text/events transmitted by guiFormSpecMenu
	 * @param text textual representation of event
	 */
	void gotText(const std::wstring &text);

private:
	/** target to transmit data to */
	GUIEngine *m_engine = nullptr;
};

/** GUIEngine specific implementation of SoundFallbackPathProvider */
class MenuMusicFetcher final : public SoundFallbackPathProvider
{
protected:
	void addThePaths(const std::string &name,
			std::vector<std::string> &paths) override;
};

/** implementation of main menu based uppon formspecs */
class GUIEngine {
	/** grant ModApiMainMenu access to private members */
	friend class ModApiMainMenu;
	friend class ModApiMainMenuSound;
	friend class MainMenuSoundHandle;

public:
	/**
	 * default constructor
	 * @param dev device to draw at
	 * @param parent parent gui element
	 * @param menumgr manager to add menus to
	 * @param smgr scene manager to add scene elements to
	 * @param data struct to transfer data to main game handling
	 */
	GUIEngine(JoystickController *joystick,
			gui::IGUIElement *parent,
            RenderSystem *rndsys,
            ResourceCache *cache,
			IMenuManager *menumgr,
			MainMenuData *data,
			bool &kill);

	/** default destructor */
	virtual ~GUIEngine();

	/**
	 * return MainMenuScripting interface
	 */
	MainMenuScripting *getScriptIface()
	{
		return m_script.get();
	}

	/**
	 * return dir of current menuscript
	 */
	std::string getScriptDir()
	{
		return m_scriptdir;
	}

	/**
	 * Get translations for content
	 *
	 * Only loads a single textdomain from the path, as specified by `domain`,
	 * for performance reasons.
	 *
	 * WARNING: Do not store the returned pointer for long as the contents may
	 * change with the next call to `getContentTranslations`.
	 * */
	Translations *getContentTranslations(const std::string &path,
			const std::string &domain, const std::string &lang_code);

private:
	std::string m_last_translations_key;
	/** Only the most recently used translation set is kept loaded */
	Translations m_last_translations;

	/** find and run the main menu script */
	bool loadMainMenuScript();

	/** run main menu loop */
	void run();

	/** update size of topleftext element */
	void updateTopLeftTextSize();

    RenderSystem                         *m_rndsys = nullptr;
	/** parent gui element */
	gui::IGUIElement                     *m_parent = nullptr;
	/** manager to add menus to */
	IMenuManager                         *m_menumanager = nullptr;
	/** pointer to data beeing transfered back to main game handling */
	MainMenuData                         *m_data = nullptr;
    /** resource cache */
    ResourceCache                        *m_rescache = nullptr;
	/** sound manager */
	std::unique_ptr<ISoundManager>        m_sound_manager;

	/** representation of form source to be used in mainmenu formspec */
	FormspecFormSource                   *m_formspecgui = nullptr;
	/** formspec input receiver */
	TextDestGuiEngine                    *m_buttonhandler = nullptr;
	/** the formspec menu */
    gui::GUISharedPointer<GUIFormSpecMenu>      m_menu;

	/** reference to kill variable managed by SIGINT handler */
	bool                                 &m_kill;

	/** variable used to abort menu and return back to main game handling */
	bool                                  m_startgame = false;

	/** scripting interface */
	std::unique_ptr<MainMenuScripting>    m_script;

	/** script basefolder */
    std::string                           m_scriptdir;

    std::unique_ptr<SpriteDrawBatch>      baseDrawBatch;
    UIRects                              *m_background;
    UIRects                              *m_overlay;
    std::unique_ptr<SpriteDrawBatch>      headerDrawBatch;
    UIRects                              *m_header;
    UIRects                              *m_footer;

    f32 m_animation_time = 0.0f;

	void setFormspecPrepend(const std::string &fs);

	/**
	 * draw background layer
	 * @param driver to use for drawing
	 */
    void drawBackground();
	/**
	 * draw overlay layer
	 * @param driver to use for drawing
	 */
    void drawOverlay();
	/**
	 * draw header layer
	 * @param driver to use for drawing
	 */
    void drawHeader();
	/**
	 * draw footer layer
	 * @param driver to use for drawing
	 */
    void drawFooter();

	/**
	 * load a texture for a specified layer
	 * @param layer draw layer to specify texture
	 * @param texturepath full path of texture to load
	 */
	bool setTexture(texture_layer layer, const std::string &texturepath,
			bool tile_image, unsigned int minsize);

	/**
	 * download a file using curl
	 * @param url url to download
	 * @param target file to store to
	 */
	static bool downloadFile(const std::string &url, const std::string &target);

	/** array containing pointers to current specified texture layers */
	image_definition m_textures[TEX_LAYER_MAX];

	/**
	 * specify text to appear as top left string
	 * @param text to set
	 */
	void setTopleftText(const std::string &text);

	/** pointer to gui element shown at topleft corner */
    gui::IGUIStaticText *m_irr_toplefttext = nullptr;
	/** and text that is in it */
	EnrichedString m_toplefttext;

	/** is drawing of clouds enabled atm */
	bool m_clouds_enabled = true;

	static void fullscreenChangedCallback(const std::string &name, void *data);
};
