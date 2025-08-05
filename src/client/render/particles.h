// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <BasicIncludes.h>
#include <mutex>
#include <unordered_map>
#include <Render/Texture2D.h>
#include "../particles.h"

struct ClientEvent;
class ParticleManager;
class ClientEnvironment;
struct MapNode;
struct ContentFeatures;
class LocalPlayer;
class IGameDef;
class Client;
class ParticleSpawner;
class ResourceCache;
class RenderSystem;
class MeshBuffer;

struct ParticleUBOData
{
	matrix4 transform;
	img::color8 light_color;
	rectf tile_uv;
};

class Particle
{
public:
	Particle(
        RenderSystem *rnd_sys,
        ResourceCache *cache,
        u32 num,
		const ParticleParameters &p,
        ParticleTexture *pt,
		const std::string &texture,
		v2f texpos,
		v2f texsize,
		img::color8 color,
		ParticleSpawner *parent = nullptr
	);

	~Particle();

	DISABLE_CLASS_COPY(Particle)

	void step(f32 dtime, ClientEnvironment *env);

	bool isExpired () const
	{ return m_expiration < m_time; }

	ParticleSpawner *getParent() const { return m_parent; }

	ParticleParamTypes::BlendMode getBlendMode() const
    { return m_parent ? m_pt->blendmode : m_p.texture.blendmode; }
private:
    void calcTileRect(const std::string &newImg);
    void updateLight(ClientEnvironment *env);
    void updateTransform(ClientEnvironment *env);

    RenderSystem *m_rnd_sys;
    ResourceCache *m_cache;

    u32 m_particle_num = 0;
    ParticleUBOData m_particle_data;

	float m_time = 0.0f;
	float m_expiration;

	// Color without lighting
	img::color8 m_base_color;

    ParticleTexture *m_pt;
	
	v2f m_texpos;
	v2f m_texsize;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;

	const ParticleParameters m_p;

	f32 m_animation_time = 0.0f;
	int m_animation_frame = 0;

	ParticleSpawner *m_parent = nullptr;
};

class ParticleSpawner
{
public:
	ParticleSpawner(LocalPlayer *player,
		const ParticleSpawnerParameters &params,
		u16 attached_id,
		//std::vector<ClientParticleTexture> &&texpool,
		ParticleManager *p_manager);

	void step(f32 dtime, ClientEnvironment *env);

	bool getExpired() const
	{ return p.amount <= 0 && p.time != 0; }

	bool hasActive() const { return m_active != 0; }
	void decrActive() { m_active -= 1; }

private:
	void spawnParticle(ClientEnvironment *env, f32 radius,
        const matrix4 *attached_absolute_pos_rot_matrix);

	size_t m_active;
	ParticleManager *m_particlemanager;
	f32 m_time;
	LocalPlayer *m_player;
	ParticleSpawnerParameters p;
	//std::vector<ClientParticleTexture> m_texpool;
	std::vector<f32> m_spawntimes;
	u16 m_attached_id;
};

/*class ParticleBuffer : public scene::ISceneNode
{
    friend class ParticleManager;
public:
    ParticleBuffer(ClientEnvironment *env, const video::SMaterial &material);

	// for pointer stability
    DISABLE_CLASS_COPY(ParticleBuffer)

	/// Reserves one more slot for a particle (4 vertices, 6 indices)
	/// @return particle index within buffer
    std::optional<u16> allocate();
	/// Frees the particle at `index`
    void release(u16 index);

	/// @return video::S3DVertex[4]
    video::S3DVertex *getVertices(u16 index);

    inline bool isEmpty() const {
        return m_free_list.size() == m_count;
    }

    virtual video::SMaterial &getMaterial(u32 num) override {
        return m_mesh_buffer->getMaterial();
    }
    virtual u32 getMaterialCount() const override {
        return 1;
    }

    virtual const core::aabbox3df &getBoundingBox() const override;

    virtual void render() override;

    virtual void OnRegisterSceneNode() override;

	// we have 16-bit indices
    static constexpr u16 MAX_PARTICLES_PER_BUFFER = 16000;

private:
    irr_ptr<scene::SMeshBuffer> m_mesh_buffer;
	// unused (e.g. expired) particle indices for re-use
    std::vector<u16> m_free_list;
	// for automatic deletion when unused for a while. is reset on allocate().
    float m_usage_timer = 0;
	// total count of contained particles
    u16 m_count = 0;
    mutable bool m_bounding_box_dirty = true;
};*/

/**
 * Class doing particle as well as their spawners handling
 */
class ParticleManager
{
	friend class ParticleSpawner;
public:
	ParticleManager(ClientEnvironment* env);
	DISABLE_CLASS_COPY(ParticleManager)
	~ParticleManager();

	void step (f32 dtime);

	void handleParticleEvent(ClientEvent *event, Client *client,
			LocalPlayer *player);

	void addDiggingParticles(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
		const MapNode &n, const ContentFeatures &f);

	void addNodeParticle(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
		const MapNode &n, const ContentFeatures &f);

	void reserveParticleSpace(size_t max_estimate);

	/**
	 * This function is only used by client particle spawners
	 *
	 * We don't need to check the particle spawner list because client ID will
	 * never overlap (u64)
	 * @return new id
	 */
	u64 generateSpawnerId()
	{
		return m_next_particle_spawner_id++;
	}

protected:
	static bool getNodeParticleParams(const MapNode &n, const ContentFeatures &f,
        ParticleParameters &p, render::Texture2D **texture, v2f &texpos,
		v2f &texsize, img::color8 *color, u8 tilenum = 0);

	static video::SMaterial getMaterialForParticle(const Particle *texture);

	bool addParticle(std::unique_ptr<Particle> toadd);

private:
	void addParticleSpawner(u64 id, std::unique_ptr<ParticleSpawner> toadd);
	void deleteParticleSpawner(u64 id);

    void stepParticles(f32 dtime);
	void stepSpawners(f32 dtime);
	void stepBuffers(f32 dtime);

	void clearAll();

    std::unique_ptr<MeshBuffer> m_unit_quad_mesh;
  
	std::vector<std::unique_ptr<Particle>> m_particles;
	std::unordered_map<u64, std::unique_ptr<ParticleSpawner>> m_particle_spawners;
	std::vector<std::unique_ptr<ParticleSpawner>> m_dying_particle_spawners;
	//std::vector<irr_ptr<ParticleBuffer>> m_particle_buffers;

	// Start the particle spawner ids generated from here after u32_max.
	// lower values are for server sent spawners.
    u64 m_next_particle_spawner_id = static_cast<u64>(T_MAX(u32)) + 1;

	ClientEnvironment *m_env;

	IntervalLimiter m_buffer_gc;

	std::mutex m_particle_list_lock;
	std::mutex m_spawner_list_lock;
};
