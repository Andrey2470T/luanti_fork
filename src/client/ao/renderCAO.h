#include "genericCAO.h"
#include <Render/DrawContext.h>
#include "client/render/tilelayer.h"

class Nametag;
class RenderSystem;
class ResourceCache;
class Model;
class AnimationManager;
class BoneAnimation;

// CAO able to be rendered and animated
class RenderCAO : public GenericCAO
{
protected:
    RenderSystem *m_rndsys;
    ResourceCache *m_cache;

    AnimationManager *m_anim_mgr;
    Model *m_model = nullptr;
    TileLayer m_tile_layer;

    // stores texture modifier before punch update
    std::string m_previous_texture_modifier = "";
    // last applied texture modifier
    std::string m_current_texture_modifier = "";

    void updateLayerUVs(std::string new_texture, u8 layer_id);
private:
    aabbf m_selection_box = aabbf(-BS/3.,-BS/3.,-BS/3., BS/3.,BS/3.,BS/3.);

    Nametag *m_nametag = nullptr;
    std::optional<v3f> m_marker;

	// Spritesheet/animation stuff
	v2f m_tx_size = v2f(1,1);
	v2s16 m_tx_basepos;
	bool m_initial_tx_basepos_set = false;
	bool m_tx_select_horiz_by_yawpitch = false;

	float m_reset_textures_timer = -1.0f;
	bool m_visuals_expired = false;
	float m_step_distance_counter = 0.0f;
    img::color8 m_last_light = img::white;
	// Material
    //video::E_MATERIAL_TYPE m_material_type;
	f32 m_material_type_param;

    bool m_is_visible = false;

    bool m_update_anim_start_time = false;

    ItemGroupList m_armor_groups;

    virtual bool visualExpiryRequired(const ObjectProperties &newprops) = 0;

    void initTileLayer();

public:
    RenderCAO(Client *client, ClientEnvironment *env);

    ~RenderCAO();

    bool getCollisionBox(aabbf *toset) const override;

	bool collideWithObjects() const override;

    virtual bool getSelectionBox(aabbf *toset) const override;

    f32 getStepHeight() const
	{
		return m_prop.stepheight;
	}

    const ItemGroupList &getGroups() const
    {
        return m_armor_groups;
    }

    Model *getModel() const
    {
        return m_model;
    }

    bool isLocalPlayer() const override
	{
		return m_is_local_player;
	}

    bool isPlayer() const
	{
		return m_is_player;
	}

    bool isVisible() const
	{
		return m_is_visible;
	}

    void setVisible(bool toset)
	{
		m_is_visible = toset;
	}

	void setChildrenVisible(bool toset);

    virtual void addMesh() = 0;
    //void addToScene() override;

    void expireVisuals()
	{
		m_visuals_expired = true;
    }

    void setAttachment(object_t parent_id, const std::string &bone, v3f position,
        v3f rotation, bool force_visible) override;

    void step(float dtime, ClientEnvironment *env) override;

    void updateLight();

	/* Get light position(s).
	 * returns number of positions written into pos[], which must have space
	 * for at least 3 vectors. */
	u16 getLightPosition(v3s16 *pos);

    void setNodeLight();

	void updateNametag();

	void updateMarker();

    void updateTexturePos() {}

	// ffs this HAS TO BE a string copy! See #5739 if you think otherwise
	// Reason: updateTextures(m_previous_texture_modifier);
    virtual void updateAppearance(std::string mod) = 0;

    void setAnimation(v2i range, f32 speed, bool loop);

    void setAnimationSpeed(f32 speed);

    void updateBones(f32 dtime);

    void updateMeshCulling();

    void processMessage(const std::string &data) override;
    bool directReportPunch(v3f dir, const ItemStack *punchitem=NULL,
            float time_from_last_punch=1000000) override;

    std::string debugInfoText() override;

    std::string infoText() override
    {
        return m_prop.infotext;
    }
};

class SpriteRenderCAO : public RenderCAO
{
public:
    SpriteRenderCAO(Client *client, ClientEnvironment *env)
        : RenderCAO(client, env)
    {}

    void addMesh() override;
    void updateAppearance(std::string mod) override;
};

class UprightSpriteRenderCAO : public RenderCAO
{
    UprightSpriteRenderCAO(Client *client, ClientEnvironment *env)
        : RenderCAO(client, env)
    {}

    void addMesh() override;
    void updateAppearance(std::string mod) override;
};

class CubeRenderCAO : public RenderCAO
{
    CubeRenderCAO(Client *client, ClientEnvironment *env)
        : RenderCAO(client, env)
    {}

    void addMesh() override;
    void updateAppearance(std::string mod) override;
};

class MeshRenderCAO : public RenderCAO
{
    MeshRenderCAO(Client *client, ClientEnvironment *env)
        : RenderCAO(client, env)
    {}

    void addMesh() override;
    void updateAppearance(std::string mod) override;
};

class WieldItemRenderCAO : public RenderCAO
{
    WieldItemRenderCAO(Client *client, ClientEnvironment *env)
        : RenderCAO(client, env)
    {}

    void addMesh() override;
    void updateAppearance(std::string mod) override {};
};

