// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "particles.h"
#include "collision.h"
#include "client/ao/genericCAO.h"
#include "client/clientevent.h"
#include "util/numeric.h"
#include "light.h"
#include "client/player/localplayer.h"
#include "environment.h"
#include "client/map/clientmap.h"
#include "mapnode.h"
#include "nodedef.h"
#include "client/client.h"
#include "settings.h"
#include "profiler.h"
#include "rendersystem.h"
#include "client/media/resource.h"
#include "client/mesh/meshbuffer.h"
#include "atlas.h"
#include "datatexture.h"
#include "batcher3d.h"
#include "renderer.h"

using BlendMode = ParticleParamTypes::BlendMode;

/*
	Particle
*/

bool Particle::particles_changed = false;

Particle::Particle(RenderSystem *rnd_sys,
    ResourceCache *cache,
    u32 num,
    const ParticleParameters &p, ParticleTexture *pt,
    const std::string &texture,
    v2f texpos,
    v2f texsize,
    img::color8 color,
    DataTexture *datatex,
    ParticleSpawner *parent
    ) :
        m_rnd_sys(rnd_sys),
        m_cache(cache),
        m_particle_num(num),
        m_data_tex(datatex),
		m_expiration(p.expirationtime),
		m_base_color(color),
        m_pt(pt),
		m_texpos(texpos),
		m_texsize(texsize),
		m_pos(p.pos),
		m_velocity(p.vel),
		m_acceleration(p.acc),
		m_p(p),
        m_parent(parent)
{
    calcTileRect(m_parent ? texture : m_p.texture.string);

    ByteArray newData(4*4 + 4 + 2*2 + 2*2 + 4, sizeof(ParticleSampleData));
    newData.setM4x4(m_particle_data.transform, 0);
    img::setColor8(&newData, m_particle_data.light_color, 4*4);
    newData.setV2U(m_particle_data.tile_coords, 4*4 + 4);
    newData.setV2U(m_particle_data.tile_size, 4*4 + 4 + 2*2);

    setBlending(newData);

    m_data_tex->addSample(newData);

    m_particle_data_changed = false;
    particles_changed = true;
}

Particle::~Particle()
{
    m_data_tex->removeSample(m_particle_num);
}

void Particle::step(f32 dtime, ClientEnvironment *env)
{
	m_time += dtime;

	// apply drag (not handled by collisionMoveSimple) and brownian motion
	v3f av = vecAbsolute(m_velocity);
	av -= av * (m_p.drag * dtime);
	m_velocity = av*vecSign(m_velocity) + v3f(m_p.jitter.pickWithin())*dtime;

	if (m_p.collisiondetection) {
        aabbf box(v3f(-m_p.size / 2.0f), v3f(m_p.size / 2.0f));
		v3f p_pos = m_pos * BS;
		v3f p_velocity = m_velocity * BS;
		collisionMoveResult r = collisionMoveSimple(env, env->getGameDef(),
			box, 0.0f, dtime, &p_pos, &p_velocity, m_acceleration * BS, nullptr,
			m_p.object_collision);

		f32 bounciness = m_p.bounce.pickWithin();
		if (r.collides && (m_p.collision_removal || bounciness > 0)) {
			if (m_p.collision_removal) {
				// force expiration of the particle
				m_expiration = -1.0f;
			} else if (bounciness > 0) {
				/* cheap way to get a decent bounce effect is to only invert the
				 * largest component of the velocity vector, so e.g. you don't
				 * have a rock immediately bounce back in your face when you try
				 * to skip it across the water (as would happen if we simply
				 * downscaled and negated the velocity vector). this means
				 * bounciness will work properly for cubic objects, but meshes
				 * with diagonal angles and entities will not yield the correct
				 * visual. this is probably unavoidable */
				if (av.Y > av.X && av.Y > av.Z) {
					m_velocity.Y = -(m_velocity.Y * bounciness);
				} else if (av.X > av.Y && av.X > av.Z) {
					m_velocity.X = -(m_velocity.X * bounciness);
				} else if (av.Z > av.Y && av.Z > av.X) {
					m_velocity.Z = -(m_velocity.Z * bounciness);
				} else { // well now we're in a bit of a pickle
					m_velocity = -(m_velocity * bounciness);
				}
			}
		} else {
			m_velocity = p_velocity / BS;
		}
		m_pos = p_pos / BS;
	} else {
		// apply velocity and acceleration to position
		m_pos += (m_velocity + m_acceleration * 0.5f * dtime) * dtime;
		// apply acceleration to velocity
		m_velocity += m_acceleration * dtime;
	}

	// Update lighting
    updateLight(env);

    // Update transform matrix
    updateTransform(env);

    updateDataInTexture();
}

void Particle::updateDataInTexture()
{
    if (!m_particle_data_changed)
        return;

    ByteArray newData(4*4 + 4 + 2*2 + 2*2, sizeof(ParticleSampleData));
    newData.setM4x4(m_particle_data.transform, 0);
    img::setColor8(&newData, m_particle_data.light_color, 4*4);
    newData.setV2U(m_particle_data.tile_coords, 4*4 + 4);
    newData.setV2U(m_particle_data.tile_size, 4*4 + 4 + 2*2);

    setBlending(newData);

    m_data_tex->updateSample(m_particle_num, newData);

    m_particle_data_changed = false;

    particles_changed = true;
}

void Particle::calcTileRect(const std::string &newImg)
{
    auto img = m_cache->getOrLoad<img::Image>(ResourceType::IMAGE, newImg);

    if (img) {
        auto basicPool = m_rnd_sys->getPool(true);

        rectf r;
        if (!m_parent) {
            std::optional<AtlasTileAnim> anim = std::nullopt;

            if (m_p.animation.type != TAT_NONE) {
                v2u imgSize = img->getSize();
                s32 frame_count;
                s32 anim_length;
                m_p.animation.determineParams(imgSize, &frame_count, &anim_length, nullptr);
                anim->first = anim_length;
                anim->second = frame_count;
            }
            r = basicPool->getTileRect(img, true, true, anim);
        }
        else
            r = basicPool->getTileRect(img, true, false);

        if (m_texpos != v2f(0.0f) && m_texsize != v2f(1.0f)) {
            m_particle_data.tile_coords = v2u(m_texpos.X, m_texpos.Y) + v2u(r.ULC.X, r.ULC.Y);
            m_particle_data.tile_size = v2u(m_texsize.X, m_texsize.Y);
        }
        else {
            m_particle_data.tile_coords = v2u(r.ULC.X, r.ULC.Y);
            m_particle_data.tile_size = v2u(r.getWidth(), r.getHeight());
        }
    }
}

void Particle::updateLight(ClientEnvironment *env)
{
	u8 light = 0;
	bool pos_ok;

	v3s16 p = v3s16(
		floor(m_pos.X+0.5),
		floor(m_pos.Y+0.5),
		floor(m_pos.Z+0.5)
	);
	MapNode n = env->getClientMap().getNode(p, &pos_ok);
	if (pos_ok)
		light = n.getLightBlend(env->getDayNightRatio(),
				env->getGameDef()->ndef()->getLightingFlags(n));
	else
		light = blend_light(env->getDayNightRatio(), LIGHT_SUN, 0);

	u8 m_light = decode_light(light + m_p.glow);

    // animate particle alpha in accordance with settings
    float alpha = 1.f;
    auto pt = m_parent ? m_pt : &m_p.texture;
    alpha = pt->alpha.blend(m_time / (m_expiration+0.1f));

    img::color8 res_c(img::PF_RGBA8,
        m_light * m_base_color.R() / 255,
        m_light * m_base_color.G() / 255,
        m_light * m_base_color.A() / 255, 255 * alpha);

    if (m_particle_data.light_color != res_c)
        m_particle_data_changed = true;
    m_particle_data.light_color = res_c;
}

void Particle::updateTransform(ClientEnvironment *env)
{
	v2f scale;

    if (m_pt)
        scale = m_pt->scale.blend(m_time / (m_expiration+0.1));
	else
		scale = v2f(1.f, 1.f);

    scale *= m_p.size * .5f;

    matrix4 scaleM;
    scaleM.setScale(v3f(scale.X, scale.Y, 0.0f));

	// Update position -- see #10398
	auto *player = env->getLocalPlayer();

    v3f euler_rot;
    if (m_p.vertical) {
        v3f ppos = player->getPosition() / BS;
        euler_rot.Y = std::atan2(ppos.Z - m_pos.Z, ppos.X - m_pos.X) / DEGTORAD + 90;
    } else {
        euler_rot.X = player->getPitch();
        euler_rot.Y = player->getYaw();
    }

    matrix4 rotM;
    rotM.setRotationDegrees(euler_rot);

    v3s16 camera_offset = env->getCameraOffset();

    matrix4 posM;
    posM.setTranslation(m_pos * BS - intToFloat(camera_offset, BS));

    matrix4 resM = posM * rotM * scaleM;

    if (m_particle_data.transform != resM)
        m_particle_data_changed = true;
    m_particle_data.transform = resM;
}

void Particle::setBlending(ByteArray &sampleArr)
{
    img::BlendMode blendType = img::BM_NORMAL;

    switch (getBlendMode()) {
    case BlendMode::add:
        blendType = img::BM_ADD;
        break;
    case BlendMode::sub:
        blendType = img::BM_SUBTRACTION;
        break;
    case BlendMode::alpha:
        blendType = img::BM_ALPHA;
        break;
    case BlendMode::screen:
        blendType = img::BM_SCREEN;
        break;
    default:
        break;
    }
    m_particle_data.blend_mode = (u8)blendType;
    sampleArr.setInt(m_particle_data.blend_mode, 4*4 + 4 + 2*2 + 2*2);
}

/*
	ParticleSpawner
*/

ParticleSpawner::ParticleSpawner(RenderSystem *rnd_sys, ResourceCache *cache,
    LocalPlayer *player,
    const ParticleSpawnerParameters &params,
    u16 attached_id,
    ParticleManager *p_manager
) :
    m_rnd_sys(rnd_sys),
    m_cache(cache),
    m_active(0),
    m_particlemanager(p_manager),
    m_time(0.0f),
    m_player(player),
    p(params),
    m_attached_id(attached_id)
{
	m_spawntimes.reserve(p.amount + 1);
	for (u16 i = 0; i <= p.amount; i++) {
		float spawntime = myrand_float() * p.time;
		m_spawntimes.push_back(spawntime);
	}

	size_t max_particles = 0; // maximum number of particles likely to be visible at any given time
	assert(p.time >= 0);
	if (p.time != 0) {
		auto maxGenerations = p.time / std::min(p.exptime.start.min, p.exptime.end.min);
		max_particles = p.amount / maxGenerations;
	} else {
		auto longestLife = std::max(p.exptime.start.max, p.exptime.end.max);
		max_particles = p.amount * longestLife;
	}

	p_manager->reserveParticleSpace(max_particles * 1.2);
}

namespace {
	GenericCAO *findObjectByID(ClientEnvironment *env, u16 id) {
		if (id == 0)
			return nullptr;
		return env->getGenericCAO(id);
	}
}

void ParticleSpawner::spawnParticle(ClientEnvironment *env, float radius,
    const matrix4 *attached_absolute_pos_rot_matrix)
{
	float fac = 0;
	if (p.time != 0) { // ensure safety from divide-by-zeroes
		fac = m_time / (p.time+0.1f);
	}

	auto r_pos    = p.pos.blend(fac);
	auto r_vel    = p.vel.blend(fac);
	auto r_acc    = p.acc.blend(fac);
	auto r_drag   = p.drag.blend(fac);
	auto r_radius = p.radius.blend(fac);
	auto r_jitter = p.jitter.blend(fac);
	auto r_bounce = p.bounce.blend(fac);
	v3f  attractor_origin    = p.attractor_origin.blend(fac);
	v3f  attractor_direction = p.attractor_direction.blend(fac);
	auto attractor_obj           = findObjectByID(env, p.attractor_attachment);
	auto attractor_direction_obj = findObjectByID(env, p.attractor_direction_attachment);

	auto r_exp     = p.exptime.blend(fac);
	auto r_size    = p.size.blend(fac);
	auto r_attract = p.attract.blend(fac);
	auto attract   = r_attract.pickWithin();

	v3f ppos = m_player->getPosition() / BS;
	v3f pos = r_pos.pickWithin();
	v3f sphere_radius = r_radius.pickWithin();

	// Need to apply this first or the following check
	// will be wrong for attached spawners
	if (attached_absolute_pos_rot_matrix) {
		pos *= BS;
		attached_absolute_pos_rot_matrix->transformVect(pos);
		pos /= BS;
		v3s16 camera_offset = m_particlemanager->m_env->getCameraOffset();
		pos.X += camera_offset.X;
		pos.Y += camera_offset.Y;
		pos.Z += camera_offset.Z;
	}

	if (pos.getDistanceFromSQ(ppos) > radius*radius)
		return;

	// Parameters for the single particle we're about to spawn
	ParticleParameters pp;
	pp.pos = pos;

	pp.vel = r_vel.pickWithin();
	pp.acc = r_acc.pickWithin();
	pp.drag = r_drag.pickWithin();
	pp.jitter = r_jitter;
	pp.bounce = r_bounce;

	if (attached_absolute_pos_rot_matrix) {
		// Apply attachment rotation
		pp.vel = attached_absolute_pos_rot_matrix->rotateAndScaleVect(pp.vel);
		pp.acc = attached_absolute_pos_rot_matrix->rotateAndScaleVect(pp.acc);
	}

	if (attractor_obj)
		attractor_origin += attractor_obj->getPosition() / BS;
	if (attractor_direction_obj) {
		auto *attractor_absolute_pos_rot_matrix = attractor_direction_obj->getAbsolutePosRotMatrix();
		if (attractor_absolute_pos_rot_matrix) {
			attractor_direction = attractor_absolute_pos_rot_matrix
					->rotateAndScaleVect(attractor_direction);
		}
	}

	pp.expirationtime = r_exp.pickWithin();

	if (sphere_radius != v3f()) {
		f32 l = sphere_radius.getLength();
		v3f mag = sphere_radius;
		mag.normalize();

		v3f ofs = v3f(l,0,0);
		ofs.rotateXZBy(myrand_range(0.f,360.f));
		ofs.rotateYZBy(myrand_range(0.f,360.f));
		ofs.rotateXYBy(myrand_range(0.f,360.f));

		pp.pos += ofs * mag;
	}

	if (p.attractor_kind != ParticleParamTypes::AttractorKind::none && attract != 0) {
		v3f dir;
		f32 dist = 0; /* =0 necessary to silence warning */
		switch (p.attractor_kind) {
			case ParticleParamTypes::AttractorKind::none:
				break;

			case ParticleParamTypes::AttractorKind::point: {
				dist = pp.pos.getDistanceFrom(attractor_origin);
				dir = pp.pos - attractor_origin;
				dir.normalize();
				break;
			}

			case ParticleParamTypes::AttractorKind::line: {
				// https://github.com/minetest/minetest/issues/11505#issuecomment-915612700
				const auto& lorigin = attractor_origin;
				v3f ldir = attractor_direction;
				ldir.normalize();
				auto origin_to_point = pp.pos - lorigin;
				auto scalar_projection = origin_to_point.dotProduct(ldir);
				auto point_on_line = lorigin + (ldir * scalar_projection);

				dist = pp.pos.getDistanceFrom(point_on_line);
				dir = (point_on_line - pp.pos);
				dir.normalize();
				dir *= -1; // flip it around so strength=1 attracts, not repulses
				break;
			}

			case ParticleParamTypes::AttractorKind::plane: {
				// https://github.com/minetest/minetest/issues/11505#issuecomment-915612700
				const v3f& porigin = attractor_origin;
				v3f normal = attractor_direction;
				normal.normalize();
				v3f point_to_origin = porigin - pp.pos;
				f32 factor = normal.dotProduct(point_to_origin);
				if (numericAbsolute(factor) == 0.0f) {
					dir = normal;
				} else {
					factor = numericSign(factor);
					dir = normal * factor;
				}
				dist = numericAbsolute(normal.dotProduct(pp.pos - porigin));
				dir *= -1; // flip it around so strength=1 attracts, not repulses
				break;
			}
		}

		f32 speedTowards = numericAbsolute(attract) * dist;
		v3f avel = dir * speedTowards;
		if (attract > 0 && speedTowards > 0) {
			avel *= -1;
			if (p.attractor_kill) {
				// make sure the particle dies after crossing the attractor threshold
				f32 timeToCenter = dist / speedTowards;
				if (timeToCenter < pp.expirationtime)
					pp.expirationtime = timeToCenter;
			}
		}
		pp.vel += avel;
	}

	p.copyCommon(pp);

    //ClientParticleTexRef texture;
	v2f texpos, texsize;
    img::color8 color = img::white;

	if (p.node.getContent() != CONTENT_IGNORE) {
		const ContentFeatures &f =
			m_particlemanager->m_env->getGameDef()->ndef()->get(p.node);
        if (!ParticleManager::getNodeParticleParams(p.node, f, pp,
				texpos, texsize, &color, p.node_tile))
			return;
	} else {
        if (p.texpool.size() == 0)
        //	return;
        u32 pool = p.texpool.size() == 1 ? 0
                : myrand_range(0, p.texpool.size()-1);
		texpos = v2f(0.0f, 0.0f);
		texsize = v2f(1.0f, 1.0f);
        if (p.texpool[pool].animated)
            pp.animation = p.texpool[pool].animation;
	}

	// synchronize animation length with particle life if desired
	if (pp.animation.type != TAT_NONE) {
		// FIXME: this should be moved into a TileAnimationParams class method
		if (pp.animation.type == TAT_VERTICAL_FRAMES &&
			pp.animation.vertical_frames.length < 0) {
			auto& a = pp.animation.vertical_frames;
			// we add a tiny extra value to prevent the first frame
			// from flickering back on just before the particle dies
			a.length = (pp.expirationtime / -a.length) + 0.1;
		} else if (pp.animation.type == TAT_SHEET_2D &&
				   pp.animation.sheet_2d.frame_length < 0) {
			auto& a = pp.animation.sheet_2d;
			auto frames = a.frames_w * a.frames_h;
			auto runtime = (pp.expirationtime / -a.frame_length) + 0.1;
			pp.animation.sheet_2d.frame_length = frames / runtime;
		}
	}

	// Allow keeping default random size
	if (p.size.start.max > 0.0f || p.size.end.max > 0.0f)
		pp.size = r_size.pickWithin();

	++m_active;
	m_particlemanager->addParticle(std::make_unique<Particle>(
            m_rnd_sys,
            m_cache,
            m_particlemanager->getParticleCount(),
            pp,
            dynamic_cast<ParticleTexture *>(&pp.texture),
            pp.texture.string,
			texpos,
			texsize,
			color,
            m_particlemanager->getDataTexture(),
			this
		));
}

void ParticleSpawner::step(float dtime, ClientEnvironment *env)
{
	m_time += dtime;

	static thread_local const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE;

	bool unloaded = false;
    const matrix4 *attached_absolute_pos_rot_matrix = nullptr;
	if (m_attached_id) {
		if (GenericCAO *attached = env->getGenericCAO(m_attached_id)) {
			attached_absolute_pos_rot_matrix = attached->getAbsolutePosRotMatrix();
		} else {
			unloaded = true;
		}
	}

	if (p.time != 0) {
		// Spawner exists for a predefined timespan
		for (auto i = m_spawntimes.begin(); i != m_spawntimes.end(); ) {
			if ((*i) <= m_time && p.amount > 0) {
				--p.amount;

				// Pretend to, but don't actually spawn a particle if it is
				// attached to an unloaded object or distant from player.
				if (!unloaded)
					spawnParticle(env, radius, attached_absolute_pos_rot_matrix);

				i = m_spawntimes.erase(i);
			} else {
				++i;
			}
		}
	} else {
		// Spawner exists for an infinity timespan, spawn on a per-second base

		// Skip this step if attached to an unloaded object
		if (unloaded)
			return;

		for (int i = 0; i <= p.amount; i++) {
			if (myrand_float() < dtime)
				spawnParticle(env, radius, attached_absolute_pos_rot_matrix);
		}
	}
}

/*
	ParticleManager
*/

ParticleManager::ParticleManager(RenderSystem *rnd_sys, ResourceCache *cache, ClientEnvironment *env) :
    m_rnd_sys(rnd_sys), m_cache(cache), m_env(env)
{
    m_shader = m_cache->getOrLoad<render::Shader>(ResourceType::SHADER, "particles");

    m_unit_quad_mesh = std::make_unique<MeshBuffer>(4, 6);
    Batcher3D::vType = B3DVT_SVT;
    Batcher3D::appendFace(m_unit_quad_mesh.get(), rectf(v2f(-1.0f), v2f(1.0f)), v3f(), {});
}

ParticleManager::~ParticleManager()
{
	clearAll();
}

void ParticleManager::step(float dtime)
{
	stepParticles(dtime);
	stepSpawners(dtime);
}

void ParticleManager::stepSpawners(float dtime)
{
	MutexAutoLock lock(m_spawner_list_lock);

	for (size_t i = 0; i < m_dying_particle_spawners.size();) {
		// the particlespawner owns the textures, so we need to make
		// sure there are no active particles before we free it
		if (!m_dying_particle_spawners[i]->hasActive()) {
			m_dying_particle_spawners[i] = std::move(m_dying_particle_spawners.back());
			m_dying_particle_spawners.pop_back();
		} else {
			++i;
		}
	}

	for (auto it = m_particle_spawners.begin(); it != m_particle_spawners.end();) {
		auto &ps = it->second;
		if (ps->getExpired()) {
			// same as above
			if (ps->hasActive())
				m_dying_particle_spawners.push_back(std::move(ps));
			it = m_particle_spawners.erase(it);
		} else {
			ps->step(dtime, m_env);
			++it;
		}
	}
}

void ParticleManager::stepParticles(float dtime)
{
	MutexAutoLock lock(m_particle_list_lock);

	for (size_t i = 0; i < m_particles.size();) {
		Particle &p = *m_particles[i];
		if (p.isExpired()) {
			ParticleSpawner *parent = p.getParent();
			if (parent) {
				assert(parent->hasActive());
				parent->decrActive();
			}
			// delete
			m_particles[i] = std::move(m_particles.back());
			m_particles.pop_back();
		} else {
			p.step(dtime, m_env);
			++i;
		}
	}

    if (Particle::particles_changed) {
        Particle::particles_changed = false;
        m_datatex->flush();
    }
}

void ParticleManager::clearAll()
{
	MutexAutoLock lock(m_spawner_list_lock);
	MutexAutoLock lock2(m_particle_list_lock);

	m_particle_spawners.clear();
	m_dying_particle_spawners.clear();

	m_particles.clear();
}

void ParticleManager::handleParticleEvent(ClientEvent *event, Client *client,
	LocalPlayer *player)
{
	switch (event->type) {
		case CE_DELETE_PARTICLESPAWNER: {
			deleteParticleSpawner(event->delete_particlespawner.id);
			// no allocated memory in delete event
			break;
		}
		case CE_ADD_PARTICLESPAWNER: {
			deleteParticleSpawner(event->add_particlespawner.id);

			const ParticleSpawnerParameters &p = *event->add_particlespawner.p;

			// texture pool
            /*std::vector<ClientParticleTexture> texpool;
			if (!p.texpool.empty()) {
				size_t txpsz = p.texpool.size();
				texpool.reserve(txpsz);
				for (size_t i = 0; i < txpsz; ++i) {
					texpool.emplace_back(p.texpool[i], client->tsrc());
				}
			} else {
				// no texpool in use, use fallback texture
				texpool.emplace_back(p.texture, client->tsrc());
            }*/

			addParticleSpawner(event->add_particlespawner.id,
					std::make_unique<ParticleSpawner>(
                        m_rnd_sys,
                        m_cache,
						player,
						p,
						event->add_particlespawner.attached_id,
						this)
					);

			delete event->add_particlespawner.p;
			break;
		}
		case CE_SPAWN_PARTICLE: {
			ParticleParameters &p = *event->spawn_particle;

            //ClientParticleTexRef texture;
            //std::unique_ptr<ClientParticleTexture> texstore;
			v2f texpos, texsize;
            img::color8 color = img::white;

			f32 oldsize = p.size;

			if (p.node.getContent() != CONTENT_IGNORE) {
				const ContentFeatures &f = m_env->getGameDef()->ndef()->get(p.node);
                getNodeParticleParams(p.node, f, p, texpos,
						texsize, &color, p.node_tile);
			} else {
				/* with no particlespawner to own the texture, we need
				 * to save it on the heap. it will be freed when the
				 * particle is destroyed */
                /*texstore = std::make_unique<ClientParticleTexture>(p.texture, client->tsrc());

                texture = ClientParticleTexRef(*texstore);*/
				texpos = v2f(0.0f, 0.0f);
				texsize = v2f(1.0f, 1.0f);
			}

			// Allow keeping default random size
            if (oldsize > 0.0f) {
				p.size = oldsize;
            }

            //if (texture.ref) {
				addParticle(std::make_unique<Particle>(
                        m_rnd_sys, m_cache, m_particles.size(),
                        p, nullptr, "", texpos, texsize, color, nullptr));
            //}

			delete event->spawn_particle;
			break;
		}
		default: break;
	}
}

void ParticleManager::renderParticles()
{
    auto rnd = m_rnd_sys->getRenderer();

    rnd->setRenderState(true);
    rnd->setTexture(dynamic_cast<render::Texture2D *>(m_rnd_sys->getPool(true)->getAtlas(0)->getTexture()));

    auto ctxt = rnd->getContext();
    ctxt->setShader(m_shader);

    m_shader->setUniformInt("mParticleCount", m_datatex->sampleCount);
    m_shader->setUniformInt("mSampleDim", m_datatex->sampleDim);
    m_shader->setUniformInt("mDataTexDim", m_datatex->texDim);

    rnd->setUniformBlocks(m_shader);

    ctxt->setActiveUnit(1, dynamic_cast<render::Texture *>(m_datatex->getGLTexture()));

    // Before rendering particles, it is necessary to render the previous frame to the framebuffer
    auto framebuffer = ctxt->getFrameBuffer();
    ctxt->setActiveUnit(2, framebuffer->getColorTextures().at(0));

    m_unit_quad_mesh->getVAO()->drawInstanced(
        render::PT_TRIANGLES, 4, 0, m_particles.size());
}

bool ParticleManager::getNodeParticleParams(const MapNode &n,
    const ContentFeatures &f, ParticleParameters &p,
    v2f &texpos, v2f &texsize, img::color8 *color, u8 tilenum)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return false;

	// Texture
	u8 texid;
	if (tilenum > 0 && tilenum <= 6)
		texid = tilenum - 1;
	else
		texid = myrand_range(0,5);
    const TileLayer &tile = f.tiles[texid].first;
	p.animation.type = TAT_NONE;

	float size = (myrand_range(0,8)) / 64.0f;
	p.size = BS * size;
	if (tile.scale)
		size /= tile.scale;
	texsize = v2f(size * 2.0f, size * 2.0f);
	texpos.X = (myrand_range(0,64)) / 64.0f - texsize.X;
	texpos.Y = (myrand_range(0,64)) / 64.0f - texsize.Y;

    if (tile.material_flags & MATERIAL_FLAG_HARDWARE_COLORIZED)
		*color = tile.color;
	else
		n.getColor(f, color);

	return true;
}

// The final burst of particles when a node is finally dug, *not* particles
// spawned during the digging of a node.

void ParticleManager::addDiggingParticles(IGameDef *gamedef,
	LocalPlayer *player, v3s16 pos, const MapNode &n, const ContentFeatures &f)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return;

	for (u16 j = 0; j < 16; j++) {
		addNodeParticle(gamedef, player, pos, n, f);
	}
}

// During the digging of a node particles are spawned individually by this
// function, called from Game::handleDigging() in game.cpp.

void ParticleManager::addNodeParticle(IGameDef *gamedef,
	LocalPlayer *player, v3s16 pos, const MapNode &n, const ContentFeatures &f)
{
	ParticleParameters p;
	v2f texpos, texsize;
    img::color8 color;

    if (!getNodeParticleParams(n, f, p, texpos, texsize, &color))
		return;

	p.texture.blendmode = f.alpha == ALPHAMODE_BLEND
			? BlendMode::alpha : BlendMode::clip;

	p.expirationtime = myrand_range(0, 100) / 100.0f;

	// Physics
	p.vel = v3f(
		myrand_range(-1.5f,1.5f),
		myrand_range(0.f,3.f),
		myrand_range(-1.5f,1.5f)
	);
	p.acc = v3f(
		0.0f,
		-player->movement_gravity * player->physics_override.gravity / BS,
		0.0f
	);
	p.pos = v3f(
		(f32)pos.X + myrand_range(0.f, .5f) - .25f,
		(f32)pos.Y + myrand_range(0.f, .5f) - .25f,
		(f32)pos.Z + myrand_range(0.f, .5f) - .25f
	);

	addParticle(std::make_unique<Particle>(
        m_rnd_sys, m_cache, m_particles.size(),
		p,
        nullptr, "",
		texpos,
		texsize,
        color, nullptr));
}

void ParticleManager::reserveParticleSpace(size_t max_estimate)
{
	MutexAutoLock lock(m_particle_list_lock);

	m_particles.reserve(m_particles.size() + max_estimate);
}

bool ParticleManager::addParticle(std::unique_ptr<Particle> toadd)
{
	MutexAutoLock lock(m_particle_list_lock);

	m_particles.push_back(std::move(toadd));
	return true;
}

void ParticleManager::addParticleSpawner(u64 id, std::unique_ptr<ParticleSpawner> toadd)
{
	MutexAutoLock lock(m_spawner_list_lock);

	auto &slot = m_particle_spawners[id];
	if (slot) {
		// do not kill spawners here. children are still alive
		errorstream << "ParticleManager: Failed to add spawner with id " << id
				<< ". Id already in use." << std::endl;
		return;
	}
	slot = std::move(toadd);
}

void ParticleManager::deleteParticleSpawner(u64 id)
{
	MutexAutoLock lock(m_spawner_list_lock);

	auto it = m_particle_spawners.find(id);
	if (it != m_particle_spawners.end()) {
		m_dying_particle_spawners.push_back(std::move(it->second));
		m_particle_spawners.erase(it);
	}
}
