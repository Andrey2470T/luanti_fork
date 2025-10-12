#include "BasicIncludes.h"
#include <memory>
#include <Utils/Rect.h>

namespace render
{
	class Texture2D;
	class DrawContext;
};

namespace img
{
    class Image;
}

namespace core
{
	class MainWindow;
};

class ResourceCache;
class RenderSystem;
class UISprite;
class UITextSprite;
class FontManager;

class LoadScreen
{
	ResourceCache *cache;
    RenderSystem *rndsys;

	std::unique_ptr<UITextSprite> guitext;
    std::unique_ptr<UISprite> progress_rect;

    img::Image *progress_bg_img;
    img::Image *progress_img;

    recti progress_cliprect;

    bool draw_clouds;
    s32 last_percent = 0;
public:
    LoadScreen(ResourceCache *_cache, RenderSystem *_system, FontManager *_mgr);

    void draw(v2u screensize, const std::wstring &text, f32 dtime, bool menu_clouds,
        s32 percent, f32 scale_f, f32 *shutdown_progress=nullptr);
private:
	static void settingChangedCallback(const std::string &name, void *data);
};
