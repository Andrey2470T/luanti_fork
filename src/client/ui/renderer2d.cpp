#include "renderer2d.h"
#include "Render/DrawContext.h"
#include "client/media/resource.h"
#include "Render/Texture2D.h"
#include "Utils/Matrix4.h"
#include "client/render/meshbuffer.h"
#include "extra_images.h"
#include "sprite.h"

img::ImageModifier *g_imgmodifier = new img::ImageModifier();

void Renderer2D::drawPrimitives(UIShape *shape, MeshBuffer *buf, u32 offset_n, u32 count_n)
{
    auto vao = buf->getVAO();

    u32 startVCount = 0;

    for (u32 i = 0; i < offset_n; i++)
        startVCount += primVCounts[(u8)shape->getPrimitiveType(i)];

    auto pType = shape->getPrimitiveType(offset_n);
    u8 vcount = primVCounts[(u8)pType] * count_n;
    switch(pType) {
    case UIPrimitiveType::LINE:
        vao->draw(render::PT_LINES, vcount, startVCount);
        break;
    case UIPrimitiveType::TRIANGLE:
        vao->draw(render::PT_TRIANGLES, vcount, startVCount);
        break;
    case UIPrimitiveType::RECTANGLE:
        vao->draw(render::PT_TRIANGLE_FAN, vcount, startVCount);
        break;
    case UIPrimitiveType::ELLIPSE:
        vao->draw(render::PT_TRIANGLE_FAN, vcount, startVCount);
        break;
    }
}

void Renderer2D::setBasicRenderState(bool alpha)
{
    Context->enableDepthTest(true);
    Context->setDepthFunc(CF_LESS);

    Context->enableCullFace(true);
    Context->setCullMode(CM_BACK);

    if (alpha)
        Context->setBlendMode(GLBlendMode::ALPHA);
    else
        Context->setBlendMode(GLBlendMode::NORMAL);
}

void Renderer2D::setRenderState(bool alpha, bool texShader)
{
    setBasicRenderState(alpha);

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
    ResourceInfo<render::Shader> *noTex2D = resCache->getOrLoad<render::Shader>(ResourceType::SHADER, "renderer2D_noTex");
    ResourceInfo<render::Shader> *tex2D = resCache->getOrLoad<render::Shader>(ResourceType::SHADER, "renderer2D");

    NoTexShader2D = noTex2D->data.get();
    Shader2D = tex2D->data.get();
}
