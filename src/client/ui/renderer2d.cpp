#include "renderer2d.h"
#include "Render/DrawContext.h"
#include "client/media/resource.h"

void Renderer2D::setDefaultRenderState()
{
	Context->enableDepthTest(true);
	Context->setDepthFunc(render::CF_LESS);
	
	Context->enableCullFace(true);
	Context->setCullMode(render::CM_BACK);
}

void Renderer2D::setRenderState(bool alpha, bool texShader)
{
	if (alpha)
		Context->setBlendMode(img::BM_ALPHA);
    else
        Context->setBlendMode(img::BM_NORMAL);
}

void Renderer2D::setClipRect(const recti &clipRect)
{
    recti viewport = Context->getViewportSize();

    Context->enableScissorTest(true);
    Context->setScissorBox(recti(
        clipRect.ULC.X, viewport.getHeight()-clipRect.LRC.Y,
        clipRect.getWidth(), clipRect.getHeight()));
}

void Renderer2D::disableScissorTest()
{
    Context->enableScissorTest(false);
}

void Renderer2D::init2DShaders()
{
    ResourceInfo *noTex2D = resCache->getOrLoad(ResourceType::SHADER, "renderer2D_noTex");
    ResourceInfo *tex2D = resCache->getOrLoad(ResourceType::SHADER, "renderer2D");

    NoTexShader2D = dynamic_cast<ShaderResourceInfo*>(noTex2D)->data.get();
    Shader2D = dynamic_cast<ShaderResourceInfo*>(tex2D)->data.get();
}
