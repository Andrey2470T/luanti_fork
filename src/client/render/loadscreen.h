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
class Renderer2D;
struct ImageFiltered;
class MeshBuffer;
class MeshCreator2D;

class LoadScreen
{
	ResourceCache *cache;
    Renderer2D *renderer;

	std::unique_ptr<GUIStaticText> guitext;
    std::unique_ptr<ImageFiltered> progress_bg_img;
    std::unique_ptr<ImageFiltered> progress_img;

    std::unique_ptr<MeshBuffer> progress_bg_rect;
    std::unique_ptr<MeshBuffer> progress_rect;

	// Settings
    static bool menu_clouds;
    static f32 gui_scaling;
public:
    LoadScreen(ResourceCache *_cache, Renderer2D *_renderer, IGUIEnvironment *_guienv);

    ~LoadScreen();

    void updateText(v2u screensize, const std::wstring &text, f32 dtime,
                    s32 percent, f32 display_density, f32 *shutdown_progress, MeshCreator2D *creator);
    void draw() const;
private:
	static void settingChangedCallback(const std::string &name, void *data);
};
