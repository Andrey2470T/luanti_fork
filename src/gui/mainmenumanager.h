// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

/*
	All kinds of stuff that needs to be exposed from main.cpp
*/
#include "client/render/rendersystem.h"
#include "gui/IGUIEnvironment.h"
#include "modalMenu.h"
#include <cassert>
#include <list>
#include "client/render/clouds.h"
#include "client/render/camera.h"

namespace gui {
	class IGUIStaticText;
}

class IGameCallback
{
public:
	virtual void exitToOS() = 0;
	virtual void keyConfig() = 0;
	virtual void disconnect() = 0;
	virtual void changePassword() = 0;
	virtual void changeVolume() = 0;
	virtual void showOpenURLDialog(const std::string &url) = 0;
	virtual void signalKeyConfigChange() = 0;
	virtual void touchscreenLayout() = 0;
};

// Handler for the modal menus

class MainMenuManager : public IMenuManager
{
public:
    MainMenuManager(RenderSystem *rndsys, ResourceCache *rescache)
        : m_guienv(rndsys->getGUIEnvironment()),
          m_menuclouds(std::make_unique<Clouds>(rndsys, rescache, rand())),
          m_menucamera(std::make_unique<Camera>())
    {
        m_menuclouds->setHeight(100.0f);
        f32 fog_range = 0.0f;
        m_menuclouds->update(0.0f, m_menucamera.get(), nullptr, fog_range, img::color8(img::PF_RGBA8, 240, 240, 255, 255));
        m_menucamera->setDirection(v3f(0, 60, 100));
        m_menucamera->setFarValue(10000);
    }

	virtual void createdMenu(gui::IGUIElement *menu)
	{
		for (gui::IGUIElement *e : m_stack) {
			if (e == menu)
				return;
		}

		if(!m_stack.empty())
			m_stack.back()->setVisible(false);

		m_stack.push_back(menu);
        m_guienv->setFocus(m_stack.back());
	}

	virtual void deletingMenu(gui::IGUIElement *menu)
	{
		// Remove all entries if there are duplicates
		m_stack.remove(menu);

		if(!m_stack.empty()) {
			m_stack.back()->setVisible(true);
            m_guienv->setFocus(m_stack.back());
		}
	}

	// Returns true to prevent further processing
	virtual bool preprocessEvent(const core::Event& event)
	{
		if (m_stack.empty())
			return false;
		GUIModalMenu *mm = dynamic_cast<GUIModalMenu*>(m_stack.back());
		return mm && mm->preprocessEvent(event);
	}

	size_t menuCount() const
	{
		return m_stack.size();
	}

	void deleteFront()
	{
		m_stack.front()->setVisible(false);
		deletingMenu(m_stack.front());
	}

	bool pausesGame()
	{
		for (gui::IGUIElement *i : m_stack) {
			GUIModalMenu *mm = dynamic_cast<GUIModalMenu*>(i);
			if (mm && mm->pausesGame())
				return true;
		}
		return false;
	}

private:
    gui::IGUIEnvironment *m_guienv;
	std::list<gui::IGUIElement*> m_stack;

    std::unique_ptr<Clouds> m_menuclouds;
    std::unique_ptr<Camera> m_menucamera;
};

extern std::unique_ptr<MainMenuManager> g_menumgr;

static inline bool isMenuActive()
{
    return g_menumgr->menuCount() != 0;
}

class MainGameCallback : public IGameCallback
{
public:
	MainGameCallback() = default;
	virtual ~MainGameCallback() = default;

	void exitToOS() override
	{
		shutdown_requested = true;
	}

	void disconnect() override
	{
		disconnect_requested = true;
	}

	void changePassword() override
	{
		changepassword_requested = true;
	}

	void changeVolume() override
	{
		changevolume_requested = true;
	}

	void keyConfig() override
	{
		keyconfig_requested = true;
	}

	void signalKeyConfigChange() override
	{
		keyconfig_changed = true;
	}

	void touchscreenLayout() override
	{
		touchscreenlayout_requested = true;
	}

	void showOpenURLDialog(const std::string &url) override
	{
		show_open_url_dialog = url;
	}

	bool disconnect_requested = false;
	bool changepassword_requested = false;
	bool changevolume_requested = false;
	bool keyconfig_requested = false;
	bool touchscreenlayout_requested = false;
	bool shutdown_requested = false;
	bool keyconfig_changed = false;
	std::string show_open_url_dialog = "";
};

extern MainGameCallback *g_gamecallback;
