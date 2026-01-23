// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2020 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

#include "sky.h"
#include <Render/Texture2D.h>
#include <Image/Image.h>
#include "client/mesh/meshbuffer.h"
#include "client/mesh/meshoperations.h"
#include "client/render/tilelayer.h"
#include "noise.h" // easeCurve
#include "profiler.h"
#include "util/numeric.h"
#include "client/render/rendersystem.h"
#include "client/render/renderer.h"
#include "client/media/resource.h"
#include "settings.h"
#include "batcher3d.h"
#include "client/mesh/defaultVertexTypes.h"
#include <Image/Converting.h>
#include "client/player/playercamera.h"
#include "atlas.h"
#include <Image/BlendModes.h>

MeshBuffer *init_sky_body(u32 faceCount=1)
{
    MeshBuffer *mesh = new MeshBuffer(4*faceCount, 6*faceCount);

    for (u32 k = 0; k < faceCount; k++)
        Batcher3D::face(mesh, {},
            {img::red, img::red, img::red, img::red}, {v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    mesh->uploadData();

    return mesh;
}

Sun::Sun(RenderSystem *rndsys, ResourceCache *cache)
    : m_rndsys(rndsys), m_sun_params(SkyboxDefaults::getSunDefaults())
{
    m_mesh = std::unique_ptr<MeshBuffer>(init_sky_body(m_image ? 1 : 4));

    // sunrise
    auto basicPool = m_rndsys->getPool(true);
    m_sunrise = cache->getOrLoad<img::Image>(ResourceType::IMAGE, "sunrisebg.png");

    m_sunrise_mesh = std::make_unique<MeshBuffer>(4, 6);
    rectf dest = basicPool->getTileRect(m_sunrise, false, true);

    img::color8 c = img::white;

    Batcher3D::face(m_sunrise_mesh.get(), {}, {c, c, c, c}, dest);

    m_sunrise_mesh->uploadData();
}

void Sun::setImage(const std::string &image,
    const std::string &tonemap, ResourceCache *cache)
{
    // Ignore matching textures (with modifiers) entirely,
    // but lets at least update the tonemap before hand.
    if (m_sun_params.tonemap != tonemap || !m_tonemap) {
        m_sun_params.tonemap = tonemap;

        m_tonemap = cache->getOrLoad<img::Image>(ResourceType::IMAGE, tonemap);
    }

    if (m_sun_params.texture == image && !m_first_update)
        return;

    m_first_update = false;
    m_sun_params.texture = image;

    auto last_image = m_image;
    m_image = cache->getOrLoad<img::Image>(ResourceType::IMAGE, image);

    m_mesh = std::unique_ptr<MeshBuffer>(init_sky_body(m_image ? 1 : 4));

    m_rndsys->getPool(true)->updateAllMeshUVs(m_mesh.get(), m_image, last_image, false, true);
}

void Sun::setSunriseImage(const std::string &sunglow_texture, ResourceCache *cache)
{
    // Ignore matching textures (with modifiers) entirely.
    if (m_sun_params.sunrise == sunglow_texture && m_sunrise)
        return;
    m_sun_params.sunrise = sunglow_texture;
    auto last_image = m_sunrise;
    m_sunrise = cache->getOrLoad<img::Image>(ResourceType::IMAGE, sunglow_texture);

    m_rndsys->getPool(true)->updateAllMeshUVs(m_sunrise_mesh.get(), m_sunrise, last_image, false, true);
}

static v3f getSkyBodyPosition(float horizon_position, float day_position, float orbit_tilt)
{
    v3f result = v3f(0, 0, -1);
    result.rotateXZBy(horizon_position);
    result.rotateXYBy(day_position);
    result.rotateYZBy(orbit_tilt);
    return result;
}

v3f Sun::getDirection(f32 time_of_day, f32 body_orbit_tilt)
{
    return getSkyBodyPosition(90, getWickedTimeOfDay(time_of_day) * 360 - 90, body_orbit_tilt);
}

void update_sky_body(MeshBuffer *mesh, float pos_1, float pos_2, const img::color8 &c, u32 faceN=0)
{
    /*
    * Create an array of vertices with the dimensions specified.
    * pos_1, pos_2: position of the body's vertices
    * c: color of the body
    */

    f32 polygonOffset = -1.0f+faceN*0.01f; // to prevent z-fighting
    svtSetPos(mesh, v3f(pos_1, pos_1, polygonOffset), faceN*4);
    svtSetPos(mesh, v3f(pos_2, pos_1, polygonOffset), faceN*4+1);
    svtSetPos(mesh, v3f(pos_2, pos_2, polygonOffset), faceN*4+2);
    svtSetPos(mesh, v3f(pos_1, pos_2, polygonOffset), faceN*4+3);

    svtSetColor(mesh, c, faceN*4);
    svtSetColor(mesh, c, faceN*4+1);
    svtSetColor(mesh, c, faceN*4+2);
    svtSetColor(mesh, c, faceN*4+3);
}

void Sun::draw()
{
    auto rnd = m_rndsys->getRenderer();
    rnd->setDefaultShader(true);

    auto world_matrix = rnd->getTransformMatrix(TMatrix::World);
    rnd->setTransformMatrix(TMatrix::World, world_matrix * m_rotation);

    m_rndsys->activateAtlas(m_image);
    rnd->setDefaultUniforms(1.0f, 0, 0.5f, img::BM_COUNT);

    rnd->draw(m_mesh.get());

    rnd->setTransformMatrix(TMatrix::World, world_matrix);
}

void Sun::drawSunrise()
{
    auto rnd = m_rndsys->getRenderer();
    rnd->setDefaultShader(true);

    auto world_matrix = rnd->getTransformMatrix(TMatrix::World);
    rnd->setTransformMatrix(TMatrix::World, world_matrix * m_sunrise_rotation);

    m_rndsys->activateAtlas(m_sunrise);
    rnd->setDefaultUniforms(1.0f, 0, 0.5f, img::BM_COUNT);

    rnd->draw(m_sunrise_mesh.get());

    rnd->setTransformMatrix(TMatrix::World, world_matrix);
}

void Sun::update(const img::color8 &color,
    const img::color8 &color2, float time_of_day, float body_orbit_tilt)
/* Draw sun in the sky.
     * driver: Video driver object used to draw
     * suncolor: main sun color
     * suncolor2: second sun color
     * wicked_time_of_day: current time of day, to know where should be the sun in the sky
     */
{
    // A magic number that contributes to the ratio 1.57 sun/moon size difference.
    constexpr float sunsize = 0.07;

    float wicked_time_of_day = getWickedTimeOfDay(time_of_day);

    if (!m_image) {
        const float sunsizes[4] = {
            (sunsize * 1.7f) * m_sun_params.scale,
            (sunsize * 1.2f) * m_sun_params.scale,
            (sunsize) * m_sun_params.scale,
            (sunsize * 0.7f) * m_sun_params.scale
        };
        img::color8 c1 = color;
        img::color8 c2 = color;
        c1.A(0.05 * 255);
        c2.A(0.15 * 255);
        const img::color8 colors[4] = {c1, c2, color, color2};
        for (int i = 0; i < 4; i++) {
            update_sky_body(m_mesh.get(), -sunsizes[i], sunsizes[i], colors[i], i);
        }
    } else {
        // Another magic number that contributes to the ratio 1.57 sun/moon size
        // difference.
        float d = (sunsize * 1.7) * m_sun_params.scale;

        img::color8 c = img::white;

        // Calculate offset normalized to the X dimension of a 512x1 px tonemap
        float offset = (1.0 - fabs(sin((time_of_day - 0.5) * PI))) * 511;

        if (m_tonemap) {
            g_imgmodifier->getPixelDirect(m_tonemap, offset, 0, c);
            c.A(255);
        }

        update_sky_body(m_mesh.get(), -d, d, c);
    }

    m_rotation.setRotationDegrees(v3f(wicked_time_of_day * 360 - 90, 90, body_orbit_tilt));

    m_mesh->uploadData();
}

void Sun::updateSunrise(float time_of_day)
{
    float wicked_time_of_day = getWickedTimeOfDay(time_of_day);
    float mid1 = 0.25;
    float mid = wicked_time_of_day < 0.5 ? mid1 : (1.0 - mid1);
    float a_ = 1.0f - std::fabs(wicked_time_of_day - mid) * 35.0f;
    float a = easeCurve(MYMAX(0, MYMIN(1, a_)));
    //std::cerr<<"a_="<<a_<<" a="<<a<<std::endl;
    float y = -(1.0 - a) * 0.22;

    std::array<v3f, 4> newPositions = {
        v3f(-1, -0.05, -1),
        v3f(1, -0.05, -1),
        v3f(1,   0.2, -1),
        v3f(-1,   0.2, -1)
    };

    f32 y_rot = 0;
    for (u32 i = 0; i < 4; i++) {
        v3f &pos = newPositions.at(i);
        if (wicked_time_of_day < 0.5)
            // Switch from -Z (south) to +X (east)
            y_rot = 90;
        else
            // Switch from -Z (south) to -X (west)
            y = -90;

        svtSetPos(m_sunrise_mesh.get(), pos, i);
    }

    m_sunrise_rotation.setTranslation(v3f(0, y, 0));
    m_sunrise_rotation.setRotationDegrees(v3f(0, y_rot, 0));

    m_sunrise_mesh->uploadData();
}

Moon::Moon(RenderSystem *rndsys)
    : m_rndsys(rndsys), m_moon_params(SkyboxDefaults::getMoonDefaults())
{
    m_mesh = std::unique_ptr<MeshBuffer>(init_sky_body(m_image ? 1 : 4));
}

void Moon::setImage(const std::string &image,
    const std::string &tonemap, ResourceCache *cache)
{
    // Ignore matching textures (with modifiers) entirely,
    // but lets at least update the tonemap before hand.
    if (m_moon_params.tonemap != tonemap || !m_tonemap) {
        m_moon_params.tonemap = tonemap;

        m_tonemap = cache->getOrLoad<img::Image>(ResourceType::IMAGE, tonemap);
    }

    if (m_moon_params.texture == image && !m_first_update)
        return;

    m_first_update = false;
    m_moon_params.texture = image;

    auto last_image = m_image;
    m_image = cache->getOrLoad<img::Image>(ResourceType::IMAGE, image);

    m_mesh = std::unique_ptr<MeshBuffer>(init_sky_body(m_image ? 1 : 4));

    m_rndsys->getPool(true)->updateAllMeshUVs(m_mesh.get(), m_image, last_image, false, true);
}

v3f Moon::getDirection(f32 time_of_day, f32 body_orbit_tilt)
{
    return getSkyBodyPosition(270, getWickedTimeOfDay(time_of_day) * 360 - 90, body_orbit_tilt);
}

void Moon::draw()
{
    auto rnd = m_rndsys->getRenderer();
    rnd->setDefaultShader(true);

    auto world_matrix = rnd->getTransformMatrix(TMatrix::World);
    rnd->setTransformMatrix(TMatrix::World, world_matrix * m_rotation);

    m_rndsys->activateAtlas(m_image);
    rnd->setDefaultUniforms(1.0f, 0, 0.5f, img::BM_COUNT);

    rnd->draw(m_mesh.get());

    rnd->setTransformMatrix(TMatrix::World, world_matrix);
}

void Moon::update(const img::color8 &color,
    const img::color8 &color2, float time_of_day, float body_orbit_tilt)
/* Draw sun in the sky.
     * driver: Video driver object used to draw
     * suncolor: main sun color
     * suncolor2: second sun color
     * wicked_time_of_day: current time of day, to know where should be the sun in the sky
     */
{
    // A magic number that contributes to the ratio 1.57 sun/moon size difference.
    constexpr float moonsize = 0.04;

    float wicked_time_of_day = getWickedTimeOfDay(time_of_day);

    if (!m_image) {
        const float moonsizes_1[4] = {
            (-moonsize * 1.9f) * m_moon_params.scale,
            (-moonsize * 1.3f) * m_moon_params.scale,
            (-moonsize) * m_moon_params.scale,
            (-moonsize) * m_moon_params.scale
        };
        const float moonsizes_2[4] = {
            (moonsize * 1.9f) * m_moon_params.scale,
            (moonsize * 1.3f) * m_moon_params.scale,
            (moonsize) *m_moon_params.scale,
            (moonsize * 0.6f) * m_moon_params.scale
        };
        img::color8 c1 = color;
        img::color8 c2 = color;
        c1.A(0.05 * 255);
        c2.A(0.15 * 255);
        const img::color8 colors[4] = {c1, c2, color, color2};
        for (int i = 0; i < 4; i++) {
            update_sky_body(m_mesh.get(), moonsizes_1[i], moonsizes_2[i], colors[i], i);
        }
    } else {
        // Another magic number that contributes to the ratio 1.57 sun/moon size
        // difference.
        float d = (moonsize * 1.9)  * m_moon_params.scale;

        img::color8 c = img::white;

        // Calculate offset normalized to the X dimension of a 512x1 px tonemap
        float offset = (1.0 - fabs(sin((time_of_day - 0.5) * PI))) * 511;

        if (m_tonemap) {
            g_imgmodifier->getPixelDirect(m_tonemap, offset, 0, c);
            c.A(255);
        }

        update_sky_body(m_mesh.get(), -d, d, c);
    }

    m_rotation.setRotationDegrees(v3f(-wicked_time_of_day * 360 + 90, -90, body_orbit_tilt));

    m_mesh->uploadData();
}

Stars::Stars(RenderSystem *rndsys, ResourceCache *cache)
    : m_rndsys(rndsys), m_star_params(SkyboxDefaults::getStarDefaults()),
    m_seed((u64)myrand() << 32 | myrand())
{
    m_shader = cache->getOrLoad<render::Shader>(ResourceType::SHADER, "skybox");
    m_rndsys->getRenderer()->setUniformBlocks(m_shader);
    m_mesh = std::make_unique<MeshBuffer>(true, SkyboxVType);
}
void Stars::setCount(u16 count)
{
    // Allow force updating star count at game init.
    if (m_star_params.count != count || m_first_update) {
        m_star_params.count = count;
        updateMesh();
    }
}

void Stars::draw()
{
    auto rnd = m_rndsys->getRenderer();

    auto world_matrix = rnd->getTransformMatrix(TMatrix::World);
    rnd->setTransformMatrix(TMatrix::World, world_matrix * m_sky_rotation);

    rnd->setShader(m_shader);
    rnd->setBlending(true);
    rnd->setTexture(nullptr);

    rnd->draw(m_mesh.get());

    rnd->setTransformMatrix(TMatrix::World, world_matrix);
}

void Stars::update(Renderer *rnd, float wicked_time_of_day, float body_orbit_tilt)
{
    // Tune values so that stars first appear just after the sun
    // disappears over the horizon, and disappear just before the sun
    // appears over the horizon.
    // Also tune so that stars are at full brightness from time 20000
    // to time 4000.

    float tod = wicked_time_of_day < 0.5f ? wicked_time_of_day : (1.0f - wicked_time_of_day);
    float day_opacity = std::clamp(m_star_params.day_opacity, 0.0f, 1.0f);
    float starbrightness = (0.25f - std::abs(tod)) * 20.0f;
    float alpha = std::clamp(starbrightness, day_opacity, 1.0f);

    img::colorf color_f = img::color8ToColorf(m_star_params.starcolor);
    color_f.A(color_f.A()*alpha);
    if (color_f.A() <= 0.0f) // Stars are only drawn when not fully transparent
        return;
    auto color = img::colorfToColor8(color_f);

    for (u32 i = 0; i < m_mesh->getVertexCount(); i++)
        svtSetColor(m_mesh.get(), color, i);

    auto day_rotation = matrix4().setRotationAxisRadians(2.0f * M_PI * (wicked_time_of_day - 0.25f), v3f(0.0f, 0.0f, 1.0f));
    auto orbit_rotation = matrix4().setRotationAxisRadians(body_orbit_tilt * M_PI / 180.0, v3f(1.0f, 0.0f, 0.0f));
    m_sky_rotation = orbit_rotation * day_rotation;

    m_mesh->uploadData();
}

void Stars::updateMesh()
{
    if (m_prev_star_count == m_star_params.count)
        return;

    m_prev_star_count = m_star_params.count;
    m_mesh->clear();
    // Stupid IrrLicht doesn’t allow non-indexed rendering, and indexed quad
    // rendering is slow due to lack of hardware support. So as indices are
    // 16-bit and there are 4 vertices per star... the limit is 2^16/4 = 0x4000.
    // That should be well enough actually.
    if (m_star_params.count > 0x4000) {
        warningstream << "Requested " << m_star_params.count << " stars but " << 0x4000 << " is the max\n";
        m_star_params.count = 0x4000;
    }

    m_mesh->reallocateData(4 * m_star_params.count, 6 * m_star_params.count);

    PcgRandom rgen(m_seed);
    float d = (0.006 / 2) * m_star_params.scale;
    for (u16 i = 0; i < m_star_params.count; i++) {
        v3f r = v3f(
            rgen.range(-10000, 10000),
            rgen.range(-10000, 10000),
            rgen.range(-10000, 10000)
            );
        matrix4 a;
        a.buildRotateFromTo(v3f(0, 1, 0), r);
        v3f p = a.rotateAndScaleVect(v3f(-d, 1, -d));
        v3f p1 = a.rotateAndScaleVect(v3f(d, 1, -d));
        v3f p2 = a.rotateAndScaleVect(v3f(d, 1, d));
        v3f p3 = a.rotateAndScaleVect(v3f(-d, 1, d));

        Batcher3D::face(m_mesh.get(), {p, p1, p2, p3}, {}, {});
    }

    m_mesh->uploadData();
}

Sky::Sky(RenderSystem *rndsys, ResourceCache *cache) :
    m_rndsys(rndsys), m_cache(cache)
{
	m_sky_params = SkyboxDefaults::getSkyDefaults();
    m_sun = std::make_unique<Sun>(rndsys, cache);
    m_moon = std::make_unique<Moon>(rndsys);
    m_stars = std::make_unique<Stars>(rndsys, cache);


    // skybox
    m_skybox_mesh = std::make_unique<MeshBuffer>(4 * 6, 6 * 6);

    for (u32 j = 0; j < 6; j++) {
        img::color8 c = img::white;
        // Use 1.05 rather than 1.0 to avoid colliding with the
        // sun, moon and stars, as this is a background skybox.

        std::array<v3f, 4> positions = {
            v3f(-1.05, -1.05, -1.05),
            v3f(1.05, -1.05, -1.05),
            v3f(1.05,  1.05, -1.05),
            v3f(-1.05,  1.05, -1.05)
        };

        for (v3f &pos : positions) {
            if (j == 0) { // Top texture
                pos.rotateYZBy(90);
                pos.rotateXZBy(90);
            } else if (j == 1) { // Bottom texture
                pos.rotateYZBy(-90);
                pos.rotateXZBy(90);
            } else if (j == 2) { // Left texture
                pos.rotateXZBy(90);
            } else if (j == 3) { // Right texture
                pos.rotateXZBy(-90);
            } else if (j == 4) { // Front texture, do nothing
                // Irrlicht doesn't like it when vertexes are left
                // alone and not rotated for some reason.
                pos.rotateXZBy(0);
            } else {// Back texture
                pos.rotateXZBy(180);
            }
        }

        Batcher3D::face(m_skybox_mesh.get(), positions, {c, c, c, c}, {v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    }

    m_skybox_mesh->uploadData();

    for (u8 i = 0; i < 6; i++)
        m_skybox_images[i] = nullptr;

    // cloudy fog
    m_cloudyfog_mesh = std::make_unique<MeshBuffer>(4 * 4, 6 * 4);

    for (u32 j = 0; j < 4; j++) {
        std::array<v3f, 4> positions = {
            v3f(-1, -0.02, -1),
            v3f(1, -0.02, -1),
            v3f(1, 0.45, -1),
            v3f(-1, 0.45, -1)
        };

        for (v3f &pos : positions) {
            if (j == 1)
                // Switch from -Z (south) to +X (east)
                pos.rotateXZBy(90);
            else if (j == 2)
                // Switch from -Z (south) to -X (west)
                pos.rotateXZBy(-90);
            else if (j == 3)
                // Switch from -Z (south) to +Z (north)
                pos.rotateXZBy(-180);
        }

        Batcher3D::face(m_cloudyfog_mesh.get(), positions, {}, {v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    }

    m_cloudyfog_mesh->uploadData();

    // far cloudy fog

    m_far_cloudyfog_mesh = std::make_unique<MeshBuffer>(4 * 5, 6 * 5);

    for (u32 j = 0; j < 4; j++) {
        std::array<v3f, 4> positions = {
            v3f(-1, -1.0,  -1),
            v3f(1, -1.0,  -1),
            v3f(1, -0.02, -1),
            v3f(-1, -0.02, -1)
        };

        for (v3f &pos : positions) {
            if (j == 1)
                // Switch from -Z (south) to +X (east)
                pos.rotateXZBy(90);
            else if (j == 2)
                // Switch from -Z (south) to -X (west)
                pos.rotateXZBy(-90);
            else if (j == 3)
                // Switch from -Z (south) to +Z (north)
                pos.rotateXZBy(-180);
        }

        Batcher3D::face(m_far_cloudyfog_mesh.get(), positions, {}, {v2f(0.0f, 1.0f), v2f(1.0f, 0.0f)});
    }

    Batcher3D::face(m_far_cloudyfog_mesh.get(),
        {
            v3f(-1, -1.0, -1),
            v3f(1, -1.0, -1),
            v3f(1, -1.0, 1),
            v3f(-1, -1.0, 1)
    }, {}, {v2f(1.0f, 1.0f), v2f(0.0f, 0.0f)});

    m_far_cloudyfog_mesh->uploadData();

	m_directional_colored_fog = g_settings->getBool("directional_colored_fog");
	m_sky_params.body_orbit_tilt = g_settings->getFloat("shadow_sky_body_orbit_tilt", -60., 60.);
	m_sky_params.fog_start = rangelim(g_settings->getFloat("fog_start"), 0.0f, 0.99f);

    m_stars->setCount(1000);
}

void Sky::render(PlayerCamera *camera)
{
	ScopeProfiler sp(g_profiler, "Sky::render()", SPT_AVG, PRECISION_MICRO);

	// Draw perspective skybox

    matrix4 translate;
    translate.setTranslation(camera->getPosition());

	// Draw the sky box between the near and far clip plane
    const f32 viewDistance = (camera->getNearValue() + camera->getFarValue()) * 0.5f;
    matrix4 scale;
    scale.setScale(v3f(viewDistance, viewDistance, viewDistance));

    auto rnd = m_rndsys->getRenderer();

    rnd->setRenderState(true);

    rnd->setTransformMatrix(TMatrix::World, translate * scale);

    //if (m_sunlight_seen) {
		// Abort rendering if we're in the clouds.
		// Stops rendering a pure white hole in the bottom of the skybox.
		if (m_in_clouds)
			return;

		// Draw the six sided skybox,
        auto ctxt = rnd->getContext();

        rnd->setClipRect(recti());

        bool fog_enabled = rnd->fogEnabled();
        rnd->enableFog(false);

        if (m_sky_params.textures.size() == 6) {
            ctxt->enableDepthTest(false);
            rnd->setDefaultShader();
            m_rndsys->activateAtlas(m_skybox_images.at(5));
            rnd->setDefaultUniforms(1.0f, 0, 0, img::BM_COUNT);
            rnd->draw(m_skybox_mesh.get());
            ctxt->enableDepthTest(true);
        }

		// Draw far cloudy fog thing blended with skycolor
        if (m_visible) {
            rnd->setDefaultShader(true);
            rnd->setTexture(nullptr);
            rnd->setDefaultUniforms(1.0f, 0, 0.5f, img::BM_COUNT);
            //rnd->draw(m_cloudyfog_mesh.get());
        }

		// Draw stars before sun and moon to be behind them
        if (m_stars->getVisible())
            m_stars->draw();

		// Draw sunrise/sunset horizon glow texture
		// (textures/base/pack/sunrisebg.png)
        if (m_sun->getSunriseVisible())
            m_sun->drawSunrise();

		// Draw sun
        if (m_sun->getVisible())
            m_sun->draw();

		// Draw moon
        if (m_moon->getVisible())
            m_moon->draw();

		// Draw far cloudy fog thing below all horizons in front of sun, moon
		// and stars.
        if (m_visible) {
            rnd->setDefaultShader(true);
            rnd->setTexture(nullptr);
            rnd->setDefaultUniforms(1.0f, 0, 0.5f, img::BM_COUNT);
            //rnd->draw(m_far_cloudyfog_mesh.get());
        }

        rnd->enableFog(fog_enabled);
    //}
}

void Sky::update(float time_of_day, float time_brightness,
	float direct_brightness, bool sunlight_seen,
	CameraMode cam_mode, float yaw, float pitch)
{
	// Stabilize initial brightness and color values by flooding updates
	if (m_first_update) {
		/*dstream<<"First update with time_of_day="<<time_of_day
				<<" time_brightness="<<time_brightness
				<<" direct_brightness="<<direct_brightness
				<<" sunlight_seen="<<sunlight_seen<<std::endl;*/
		m_first_update = false;
		for (u32 i = 0; i < 100; i++) {
			update(time_of_day, time_brightness, direct_brightness,
					sunlight_seen, cam_mode, yaw, pitch);
		}
		return;
	}

	m_time_of_day = time_of_day;
	m_time_brightness = time_brightness;
	m_sunlight_seen = sunlight_seen;
	m_in_clouds = false;

	bool is_dawn = (time_brightness >= 0.20 && time_brightness < 0.35);

    img::colorf bgcolor_bright_normal_f = img::color8ToColorf(m_sky_params.sky_color.day_horizon);
    img::colorf bgcolor_bright_indoor_f = img::color8ToColorf(m_sky_params.sky_color.indoors);
    img::colorf bgcolor_bright_dawn_f = img::color8ToColorf(m_sky_params.sky_color.dawn_horizon);
    img::colorf bgcolor_bright_night_f = img::color8ToColorf(m_sky_params.sky_color.night_horizon);

    img::colorf skycolor_bright_normal_f = img::color8ToColorf(m_sky_params.sky_color.day_sky);
    img::colorf skycolor_bright_dawn_f = img::color8ToColorf(m_sky_params.sky_color.dawn_sky);
    img::colorf skycolor_bright_night_f = img::color8ToColorf(m_sky_params.sky_color.night_sky);

    img::colorf cloudcolor_bright_normal_f = m_cloudcolor_day_f;
    img::colorf cloudcolor_bright_dawn_f = m_cloudcolor_dawn_f;

	float cloud_color_change_fraction = 0.95;
	if (sunlight_seen) {
		if (std::fabs(time_brightness - m_brightness) < 0.2f) {
			m_brightness = m_brightness * 0.95 + time_brightness * 0.05;
		} else {
			m_brightness = m_brightness * 0.80 + time_brightness * 0.20;
			cloud_color_change_fraction = 0.0;
		}
	} else {
		if (direct_brightness < m_brightness)
			m_brightness = m_brightness * 0.95 + direct_brightness * 0.05;
		else
			m_brightness = m_brightness * 0.98 + direct_brightness * 0.02;
	}

	m_clouds_visible = true;
	float color_change_fraction = 0.98f;
	if (sunlight_seen) {
		if (is_dawn) { // Dawn
            m_bgcolor_bright_f = m_bgcolor_bright_f.linInterp(
				bgcolor_bright_dawn_f, color_change_fraction);
            m_skycolor_bright_f = m_skycolor_bright_f.linInterp(
				skycolor_bright_dawn_f, color_change_fraction);
            m_cloudcolor_bright_f = m_cloudcolor_bright_f.linInterp(
				cloudcolor_bright_dawn_f, color_change_fraction);
		} else {
			if (time_brightness < 0.13f) { // Night
                m_bgcolor_bright_f = m_bgcolor_bright_f.linInterp(
					bgcolor_bright_night_f, color_change_fraction);
                m_skycolor_bright_f = m_skycolor_bright_f.linInterp(
					skycolor_bright_night_f, color_change_fraction);
			} else { // Day
                m_bgcolor_bright_f = m_bgcolor_bright_f.linInterp(
					bgcolor_bright_normal_f, color_change_fraction);
                m_skycolor_bright_f = m_skycolor_bright_f.linInterp(
					skycolor_bright_normal_f, color_change_fraction);
			}

            m_cloudcolor_bright_f = m_cloudcolor_bright_f.linInterp(
				cloudcolor_bright_normal_f, color_change_fraction);
		}
	} else {
        m_bgcolor_bright_f = m_bgcolor_bright_f.linInterp(
			bgcolor_bright_indoor_f, color_change_fraction);
        m_skycolor_bright_f = m_skycolor_bright_f.linInterp(
			bgcolor_bright_indoor_f, color_change_fraction);
        m_cloudcolor_bright_f = m_cloudcolor_bright_f.linInterp(
			cloudcolor_bright_normal_f, color_change_fraction);
        m_clouds_visible = false;
	}

    img::color8 bgcolor_bright = img::colorfToColor8(m_bgcolor_bright_f);
    m_bgcolor = img::color8(
        img::PF_RGBA8,
        bgcolor_bright.R() * m_brightness,
        bgcolor_bright.G() * m_brightness,
        bgcolor_bright.B() * m_brightness,
        255
	);

    img::color8 skycolor_bright = img::colorfToColor8(m_skycolor_bright_f);
    m_skycolor = img::color8(
        img::PF_RGBA8,
        skycolor_bright.R() * m_brightness,
        skycolor_bright.G() * m_brightness,
        skycolor_bright.B() * m_brightness,
        255
	);

	// Horizon coloring based on sun and moon direction during sunset and sunrise
    img::color8 pointcolor = img::color8(img::PF_RGBA8, 255, 255, 255, m_bgcolor.A());
	if (m_directional_colored_fog) {
		if (m_horizon_blend() != 0) {
			// Calculate hemisphere value from yaw, (inverted in third person front view)
			s8 dir_factor = 1;
			if (cam_mode > CAMERA_MODE_THIRD)
				dir_factor = -1;
			f32 pointcolor_blend = wrapDegrees_0_360(yaw * dir_factor + 90);
			if (pointcolor_blend > 180)
				pointcolor_blend = 360 - pointcolor_blend;
			pointcolor_blend /= 180;
			// Bound view angle to determine where transition starts and ends
			pointcolor_blend = rangelim(1 - pointcolor_blend * 1.375, 0, 1 / 1.375) *
				1.375;
			// Combine the colors when looking up or down, otherwise turning looks weird
			pointcolor_blend += (0.5 - pointcolor_blend) *
				(1 - MYMIN((90 - std::fabs(pitch)) / 90 * 1.5, 1));
			// Invert direction to match where the sun and moon are rising
			if (m_time_of_day > 0.5)
				pointcolor_blend = 1 - pointcolor_blend;
			// Horizon colors of sun and moon
			f32 pointcolor_light = rangelim(m_time_brightness * 3, 0.2, 1);

            img::colorf pointcolor_sun_f(1, 1, 1, 1);
			// Use tonemap only if default sun/moon tinting is used
			// which keeps previous behavior.
            auto sun_base_color = m_sun->getBaseColor();
            if (sun_base_color != img::black && m_default_tint) {
                pointcolor_sun_f.R(pointcolor_light *
                    (float)sun_base_color.R() / 255);
                pointcolor_sun_f.B(pointcolor_light *
                    (float)sun_base_color.B() / 255);
                pointcolor_sun_f.G(pointcolor_light *
                    (float)sun_base_color.G() / 255);
			} else if (!m_default_tint) {
                pointcolor_sun_f = img::color8ToColorf(m_sky_params.fog_sun_tint);
			} else {
                pointcolor_sun_f.R(pointcolor_light * 1);
                pointcolor_sun_f.B(pointcolor_light *
                    (0.25 + (rangelim(m_time_brightness, 0.25, 0.75) - 0.25) * 2 * 0.75));
                pointcolor_sun_f.G(pointcolor_light * (pointcolor_sun_f.B() * 0.375 +
                    (rangelim(m_time_brightness, 0.05, 0.15) - 0.05) * 10 * 0.625));
			}

            img::colorf pointcolor_moon_f;
			if (m_default_tint) {
                pointcolor_moon_f = img::colorf(
					0.5 * pointcolor_light,
					0.6 * pointcolor_light,
					0.8 * pointcolor_light,
					1
				);
			} else {
                pointcolor_moon_f = img::colorf(
                    (m_sky_params.fog_moon_tint.R() / 255.0f) * pointcolor_light,
                    (m_sky_params.fog_moon_tint.G() / 255.0f) * pointcolor_light,
                    (m_sky_params.fog_moon_tint.B() / 255.0f) * pointcolor_light,
					1
				);
			}
            auto moon_base_color = m_moon->getBaseColor();
            if (moon_base_color != img::black && m_default_tint) {
                pointcolor_moon_f.R(pointcolor_light *
                    (float)moon_base_color.R() / 255);
                pointcolor_moon_f.B(pointcolor_light *
                    (float)moon_base_color.B() / 255);
                pointcolor_moon_f.G(pointcolor_light *
                    (float)moon_base_color.G() / 255);
			}

            img::color8 pointcolor_sun = img::colorfToColor8(pointcolor_sun_f);
            img::color8 pointcolor_moon = img::colorfToColor8(pointcolor_moon_f);
			// Calculate the blend color
            pointcolor = img::color8::linInterp(pointcolor_sun, pointcolor_moon,  pointcolor_blend);
		}
        m_bgcolor = img::color8::linInterp(pointcolor,m_bgcolor, m_horizon_blend() * 0.5);
        m_skycolor = img::color8::linInterp(pointcolor, m_skycolor, m_horizon_blend() * 0.25);
	}

	float cloud_direct_brightness = 0.0f;
	if (sunlight_seen) {
		if (!m_directional_colored_fog) {
			cloud_direct_brightness = time_brightness;
			// Boost cloud brightness relative to sky, at dawn, dusk and at night
			if (time_brightness < 0.7f)
				cloud_direct_brightness *= 1.3f;
		} else {
			cloud_direct_brightness = std::fmin(m_horizon_blend() * 0.15f +
				m_time_brightness, 1.0f);
			// Set the same minimum cloud brightness at night
			if (time_brightness < 0.5f)
				cloud_direct_brightness = std::fmax(cloud_direct_brightness,
					time_brightness * 1.3f);
		}
	} else {
		cloud_direct_brightness = direct_brightness;
	}

	m_cloud_brightness = m_cloud_brightness * cloud_color_change_fraction +
		cloud_direct_brightness * (1.0 - cloud_color_change_fraction);
    m_cloudcolor_f = img::colorf(
        m_cloudcolor_bright_f.R() * m_cloud_brightness,
        m_cloudcolor_bright_f.G() * m_cloud_brightness,
        m_cloudcolor_bright_f.B() * m_cloud_brightness,
		1.0
	);
	if (m_directional_colored_fog) {
        m_cloudcolor_f = img::colorf::linInterp(img::color8ToColorf(pointcolor), m_cloudcolor_f, m_horizon_blend() * 0.25);
	}

    updateCloudyFogColor();

    img::colorf suncolor_f(1, 1, 0, 1);
    //suncolor_f.r = 1;
    //suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.7 + m_time_brightness * 0.5));
    //suncolor_f.b = MYMAX(0.0, m_brightness * 0.95);
    img::colorf suncolor2_f(1, 1, 1, 1);
    // The values below were probably meant to be suncolor2_f instead of a
    // reassignment of suncolor_f. However, the resulting colour was chosen
    // and is our long-running classic colour. So preserve, but comment-out
    // the unnecessary first assignments above.
    suncolor_f.R(1);
    suncolor_f.G(MYMAX(0.3, MYMIN(1.0, 0.85 + m_time_brightness * 0.5)));
    suncolor_f.B(MYMAX(0.0, m_brightness));

    img::colorf mooncolor_f(0.50, 0.57, 0.65, 1);
    img::colorf mooncolor2_f(0.85, 0.875, 0.9, 1);

    float wicked_time_of_day = getWickedTimeOfDay(m_time_of_day);

    img::color8 suncolor = img::colorfToColor8(suncolor_f);
    img::color8 suncolor2 = img::colorfToColor8(suncolor2_f);
    img::color8 mooncolor = img::colorfToColor8(mooncolor_f);
    img::color8 mooncolor2 = img::colorfToColor8(mooncolor2_f);

    m_sun->update(suncolor, suncolor2, time_of_day, m_sky_params.body_orbit_tilt);
    m_sun->updateSunrise(time_of_day);

    m_moon->update(mooncolor, mooncolor2, time_of_day, m_sky_params.body_orbit_tilt);

    m_stars->updateMesh();
    m_stars->update(m_rndsys->getRenderer(), wicked_time_of_day, m_sky_params.body_orbit_tilt);
}

void Sky::setSkyColors(const SkyColor &sky_color)
{
	m_sky_params.sky_color = sky_color;
}

void Sky::setHorizonTint(img::color8 sun_tint, img::color8 moon_tint,
	const std::string &use_sun_tint)
{
	// Change sun and moon tinting:
	m_sky_params.fog_sun_tint = sun_tint;
	m_sky_params.fog_moon_tint = moon_tint;
	// Faster than comparing strings every rendering frame
	if (use_sun_tint == "default")
		m_default_tint = true;
	else if (use_sun_tint == "custom")
		m_default_tint = false;
	else
		m_default_tint = true;
}

void Sky::addTextureToSkybox(const std::string &texture, u8 id)
{
	// Sanity check for more than six textures.
    if (id + 5 >= SKY_MATERIAL_COUNT)
		return;
	// Keep a list of texture names handy.
	m_sky_params.textures.emplace_back(texture);

    auto last_image = m_skybox_images[id];
    m_skybox_images[id] = m_cache->getOrLoad<img::Image>(ResourceType::IMAGE, texture);

    m_rndsys->getPool(true)->updateMeshUVs(m_skybox_mesh.get(), id*6, 6, m_skybox_images[id], last_image, false, true);
}

float getWickedTimeOfDay(float time_of_day)
{
	float nightlength = 0.415f;
	float wn = nightlength / 2;
	float wicked_time_of_day = 0;
	if (time_of_day > wn && time_of_day < 1.0f - wn)
		wicked_time_of_day = (time_of_day - wn) / (1.0f - wn * 2) * 0.5f + 0.25f;
	else if (time_of_day < 0.5f)
		wicked_time_of_day = time_of_day / wn * 0.25f;
	else
		wicked_time_of_day = 1.0f - ((1.0f - time_of_day) / wn * 0.25f);
	return wicked_time_of_day;
}

void Sky::updateCloudyFogColor()
{
    svtSetColor(m_cloudyfog_mesh.get(), m_bgcolor, 0);
    svtSetColor(m_cloudyfog_mesh.get(), m_bgcolor, 0);
    svtSetColor(m_cloudyfog_mesh.get(), m_skycolor, 0);
    svtSetColor(m_cloudyfog_mesh.get(), m_skycolor, 0);

    MeshOperations::colorizeMesh(m_far_cloudyfog_mesh.get(), m_bgcolor);

    m_cloudyfog_mesh->uploadData();
    m_far_cloudyfog_mesh->uploadData();
}
