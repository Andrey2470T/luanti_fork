#pragma once

#include "clientactiveobject.h"

/*
	TestCAO
*/

class TestCAO : public ClientActiveObject
{
public:
	TestCAO(Client *client, ClientEnvironment *env);
	virtual ~TestCAO() = default;

	ActiveObjectType getType() const
	{
		return ACTIVEOBJECT_TYPE_TEST;
	}

	static std::unique_ptr<ClientActiveObject> create(Client *client, ClientEnvironment *env);

	void addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr);
	void removeFromScene(bool permanent);
	void updateLight(u32 day_night_ratio);
	void updateNodePos();

	void step(float dtime, ClientEnvironment *env);

	void processMessage(const std::string &data);

	bool getCollisionBox(aabb3f *toset) const { return false; }
private:
	scene::IMeshSceneNode *m_node;
	v3f m_position;
};
