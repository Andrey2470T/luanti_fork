// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "client/input/inputhandler.h"
#include "debug.h"
#include "config.h"
#include "client/media/shader.h"
#include "client/pipeline/core.h"
// include the shadow mapper classes too
#include "client/shadows/dynamicshadowsrender.h"
#include <Video/VideoDriver.h>

#if !IS_CLIENT_BUILD
#error Do not include in server builds
#endif

struct VideoDriverInfo {
	std::string name;
	std::string friendly_name;
};

class ITextureSource;
class Camera;
class Client;
class LocalPlayer;
class Hud;
class Minimap;
class AtlasPool;
class RenderingCore;
class PostProcessingPipeline;

// Instead of a mechanism to disable fog we just set it to be really far away
#define FOG_RANGE_ALL (100000 * BS)

/* Helpers */

struct FpsControl {
	FpsControl() : last_time(0), busy_time(0), sleep_time(0) {}

	void reset();

	void limit(SDLDevice *device, f32 *dtime);

	u32 getBusyMs() const { return busy_time / 1000; }

	// all values in microseconds (us)
	u64 last_time, busy_time, sleep_time;
};

template <class T>
class UniformBuffer
{
	video::HWBuffer hw;
	T data;
public:
	UniformBuffer(u32 bindingPoint, const T &data_)
		: hw(video::HWBT_UNIFORM, bindingPoint), data(std::move(data_))
	{}

	video::HWBuffer &getHW()
	{
		return hw;
	}

	T &getData()
	{
		return data;
	}

	void upload()
	{
		hw.upload(&data, sizeof(data));
	}
	void upload(u32 size, u32 offset=0)
	{
		hw.upload(&data, size, offset);
	}
};

struct Matrices
{
	core::matrix4 worldViewProj;
	core::matrix4 worldView;
	core::matrix4 world;
	core::matrix4 texture;

	bool operator==(const Matrices &other)
	{
		return (worldViewProj == other.worldViewProj &&
			worldView == other.worldView && world == other.world &&
			texture == other.texture);
	}
};

struct FogParams
{
	video::SColorf color;
	f32 distance;
	f32 parameter;

	bool operator==(const FogParams &other)
	{
		return (color == other.color && distance == other.distance && parameter == other.parameter);
	}
};

/* Rendering engine class */

class RenderingEngine
{
public:
	static const video::SColor MENU_SKY_COLOR;
	static const u32 MATRICES_UBO_BINDING_POINT;
	static const u32 FOGPARAMS_UBO_BINDING_POINT;

	RenderingEngine(MyEventReceiver *eventReceiver);
	~RenderingEngine();

	void setResizable(bool resize);

	video::VideoDriver *getVideoDriver() { return driver; }

	static const VideoDriverInfo &getVideoDriverInfo(video::E_DRIVER_TYPE type);
	static float getDisplayDensity();

	bool setupTopLevelWindow();
	bool setWindowIcon();
	void cleanupMeshCache();

	void removeMesh(const scene::IMesh* mesh);

	/**
	 * This takes 3d_mode into account - side-by-side will return a
	 * halved horizontal size.
	 *
	 * @return "window" size
	 */
	static v2u32 getWindowSize()
	{
		sanity_check(s_singleton);
		return s_singleton->_getWindowSize();
	}

	io::IFileSystem *get_filesystem()
	{
		return m_device->getFileSystem();
	}

	static video::VideoDriver *get_video_driver()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device->getVideoDriver();
	}

	scene::ISceneManager *get_scene_manager()
	{
		return m_device->getSceneManager();
	}

	static SDLDevice *get_raw_device()
	{
		sanity_check(s_singleton && s_singleton->m_device);
		return s_singleton->m_device;
	}

	gui::IGUIEnvironment *get_gui_env()
	{
		return m_device->getGUIEnvironment();
	}

	// If "indef_pos" is given, the value of "percent" is ignored and an indefinite
	// progress bar is drawn.
	void draw_load_screen(const std::wstring &text,
			gui::IGUIEnvironment *guienv, ITextureSource *tsrc,
			float dtime = 0, int percent = 0, float *indef_pos = nullptr);

	void draw_scene(video::SColor skycolor, bool show_hud,
			bool draw_wield_tool, bool draw_crosshair);

	void initializeAtlases();
	void initialize(Client *client, Hud *hud);
	void finalize();

	bool run()
	{
		return m_device->run();
	}

	// FIXME: this is still global when it shouldn't be
	static ShadowRenderer *get_shadow_renderer()
	{
		if (s_singleton && s_singleton->core)
			return s_singleton->core->get_shadow_renderer();
		return nullptr;
	}
	static std::vector<video::E_DRIVER_TYPE> getSupportedVideoDrivers();

	static void autosaveScreensizeAndCo(
			const core::dimension2d<u32> initial_screen_size,
			const bool initial_window_maximized);

	static PointerType getLastPointerType()
	{
		sanity_check(s_singleton && s_singleton->m_receiver);
		return s_singleton->m_receiver->getLastPointerType();
	}

	AtlasPool *getAtlasPool()
	{
		return mainPool.get();
	}

	RenderPipeline *getPipeline()
	{
		return core->getPipeline();
	}
	PostProcessingPipeline *getPostProcessingPipeline();

	void createUBOs();

	static void updateMatrices();
	void getFogParams(video::SColor &color, f32 &start, f32 &end) const;
	void setFogParams(const video::SColor &color={0, 255, 255, 255}, f32 start=50.0f, f32 end=100.0f);

	static void setUBOs(video::MaterialRenderer *renderer);

private:
	static void settingChangedCallback(const std::string &name, void *data);
	v2u32 _getWindowSize() const;

	std::unique_ptr<AtlasPool> mainPool;

	std::unique_ptr<UniformBuffer<Matrices>> matricesUBO;
	std::unique_ptr<UniformBuffer<FogParams>> fogUBO;

	std::unique_ptr<RenderingCore> core;
	SDLDevice *m_device = nullptr;
	video::VideoDriver *driver;
	MyEventReceiver *m_receiver = nullptr;
	static RenderingEngine *s_singleton;
};
