#include "renderer2d.h"
#include "Render/DrawContext.h"
#include "client/media/resource.h"
#include "Render/Texture2D.h"
#include "Utils/Matrix4.h"
#include "client/render/meshbuffer.h"
#include "meshcreator2d.h"

void Renderer2D::draw2DLine(MeshBuffer *line)
{
    auto vao = line->getVAO();
    vao->draw(render::PT_LINES, 2);
}

void Renderer2D::draw2DRectangle(MeshBuffer *rect)
{
    auto vao = rect->getVAO();
    vao->draw(render::PT_TRIANGLE_FAN, 4);
}

void Renderer2D::draw2DImage(MeshBuffer *rect, Texture2D *tex)
{
    setTexture(tex);

    draw2DRectangle(rect);
}

void Renderer2D::draw2D9SlicedImage(Image2D9Slice *img)
{
    for (u8 i = 0; i < 9; i++)
        img->drawSlice(this, i);
}

void Renderer2D::setRenderState(bool alpha, bool texShader)
{
    Context->enableDepthTest(true);
    Context->setDepthFunc(CF_LESS);

    Context->enableCullFace(true);
    Context->setCullMode(CM_BACK);

    if (alpha)
        Context->setBlendMode(GLBlendMode::ALPHA);
    else
        Context->setBlendMode(GLBlendMode::NORMAL);

    if (texShader)
        Context->setShader(Shader2D);
    else
        Context->setShader(NoTexShader2D);
}

void Renderer2D::setTexture(Texture2D *tex)
{
    Context->setActiveUnit(0, tex);

    texture0 = tex;
}

void Renderer2D::setUniforms(f32 thickness, bool texShader)
{
    Shader *cur_shader = texShader ? Shader2D : NoTexShader2D;

    cur_shader->setUniformFloat("thickness", thickness);

    auto cur_viewport = Context->getViewportSize();
    matrix4 proj;
    f32 xInv2 = 2.0f / cur_viewport.getWidth();
    f32 yInv2 = 2.0f / cur_viewport.getHeight();
    proj.setScale({ xInv2, -yInv2, 0.0f });
    proj.setTranslation({ -1.0f, 1.0f, 0.0f });
    cur_shader->setUniform4x4Matrix("projection", proj);

    cur_shader->setUniformInt("textureUsed", (s32)texShader);
    if (texShader) {
        cur_shader->setUniformInt("texture", 0);
    }
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
