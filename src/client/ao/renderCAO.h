#include "genericCAO.h"

class Nametag;
struct MinimapMarker;
class RenderSystem;

// CAO able to be rendered and animated
class RenderCAO : public GenericCAO
{
private:
    //scene::ISceneManager *m_smgr = nullptr;

    aabbf m_selection_box = aabbf(-BS/3.,-BS/3.,-BS/3., BS/3.,BS/3.,BS/3.);
    //scene::IMeshSceneNode *m_meshnode = nullptr;
    //scene::IAnimatedMeshSceneNode *m_animated_meshnode = nullptr;
    //WieldMeshSceneNode *m_wield_meshnode = nullptr;
    //scene::IBillboardSceneNode *m_spritenode = nullptr;

	Nametag *m_nametag = nullptr;
	MinimapMarker *m_marker = nullptr;

	// Spritesheet/animation stuff
	v2f m_tx_size = v2f(1,1);
	v2s16 m_tx_basepos;
	bool m_initial_tx_basepos_set = false;
	bool m_tx_select_horiz_by_yawpitch = false;
	v2f m_animation_range;
	float m_animation_speed = 15.0f;
	float m_animation_blend = 0.0f;
	bool m_animation_loop = true;
	// stores position and rotation for each bone name
	BoneOverrideMap m_bone_override;

	int m_anim_frame = 0;
	int m_anim_num_frames = 1;
	float m_anim_framelength = 0.2f;
	float m_anim_timer = 0.0f;

	float m_reset_textures_timer = -1.0f;
	// stores texture modifier before punch update
	std::string m_previous_texture_modifier = "";
	// last applied texture modifier
	std::string m_current_texture_modifier = "";
	bool m_visuals_expired = false;
	float m_step_distance_counter = 0.0f;
    img::color8 m_last_light = img::white;
	// Material
    //video::E_MATERIAL_TYPE m_material_type;
	f32 m_material_type_param;

    bool m_is_visible = false;

    ItemGroupList m_armor_groups;

	bool visualExpiryRequired(const ObjectProperties &newprops) const;

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

    //void addToScene() override;

    void expireVisuals()
	{
		m_visuals_expired = true;
	}

    void setAttachment(object_t parent_id, const std::string &bone, v3f position,
        v3f rotation, bool force_visible) override;

    void step(float dtime, ClientEnvironment *env) override;

    void updateLight(u32 day_night_ratio);

    void setNodeLight(const img::color8 &light);

	/* Get light position(s).
	 * returns number of positions written into pos[], which must have space
	 * for at least 3 vectors. */
	u16 getLightPosition(v3s16 *pos);

	void updateNametag();

	void updateMarker();

	void updateNodePos();

	void updateTexturePos();

	// ffs this HAS TO BE a string copy! See #5739 if you think otherwise
	// Reason: updateTextures(m_previous_texture_modifier);
	void updateTextures(std::string mod);

	void updateAnimation();

	void updateAnimationSpeed();

	void updateBones(f32 dtime);

	void updateMeshCulling();

    void processMessage(const std::string &data) override;
    /*bool directReportPunch(v3f dir, const ItemStack *punchitem=NULL,
            float time_from_last_punch=1000000) override;

    std::string debugInfoText() override;

    std::string infoText() override
    {
        return m_prop.infotext;
    }*/
};
