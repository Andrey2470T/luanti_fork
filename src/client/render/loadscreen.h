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
class UISprite;
class UITextSprite;
class MeshCreator2D;

class LoadScreen
{
	ResourceCache *cache;
    Renderer2D *renderer;

	std::unique_ptr<UITextSprite> guitext;
    std::unique_ptr<UISprite> progress_bg_rect;
    std::unique_ptr<UISprite> progress_rect;

    bool draw_clouds;
    s32 last_percent = 0;
public:
    LoadScreen(ResourceCache *_cache, Renderer2D *_renderer, MeshCreator2D *_creator, IGUIEnvironment *_guienv);

    ~LoadScreen();

    void updateText(v2u screensize, const std::wstring &text, f32 dtime, bool menu_clouds,
                    s32 percent, f32 scale_f, f32 *shutdown_progress, MeshCreator2D *creator);
    void draw() const;
private:
	static void settingChangedCallback(const std::string &name, void *data);
};
