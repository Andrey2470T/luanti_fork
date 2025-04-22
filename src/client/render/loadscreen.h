#include "BasicIncludes.h"
#include <memory>

namespace render
{
	class Texture2D;
	class DrawContext;
};

namespace main
{
	class MainWindow;
};

class ResourceCache;
class GUIStaticText;
class IGUIEnvironment;

class LoadScreen
{
	ResourceCache *cache;

	std::unique_ptr<GUIStaticText> guitext;
	render::Texture2D *progress_bg_img;
	render::Texture2D *progress_img;

	// Settings
    static bool menu_clouds;
    static f32 gui_scaling;
public:
    LoadScreen(ResourceCache *_cache, IGUIEnvironment *_guienv);

    void updateText(v2u screensize, const std::wstring &text, f32 dtime,
        s32 percent, f32 display_density, f32 *shutdown_progress);
    void draw() const;
private:
	static void settingChangedCallback(const std::string &name, void *data);
};
