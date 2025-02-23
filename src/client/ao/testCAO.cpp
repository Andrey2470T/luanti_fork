#include "testcao.h"

// Prototype
static TestCAO proto_TestCAO(nullptr, nullptr);

TestCAO::TestCAO(Client *client, ClientEnvironment *env):
	ClientActiveObject(0, client, env),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
	if (!client)
		ClientActiveObject::registerType(getType(), create);
}

std::unique_ptr<ClientActiveObject> TestCAO::create(Client *client, ClientEnvironment *env)
{
	return std::make_unique<TestCAO>(client, env);
}

void TestCAO::addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr)
{
	if(m_node != NULL)
		return;

	//video::IVideoDriver* driver = smgr->getVideoDriver();

	scene::SMesh *mesh = new scene::SMesh();
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(-BS/2,-BS/4,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2,-BS/4,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2,BS/4,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2,BS/4,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().BackfaceCulling = false;
	buf->getMaterial().TextureLayers[0].Texture = tsrc->getTextureForMesh("rat.png");
	buf->getMaterial().TextureLayers[0].MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
	buf->getMaterial().TextureLayers[0].MagFilter = video::ETMAGF_NEAREST;
	buf->getMaterial().FogEnable = true;
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	updateNodePos();
}

void TestCAO::removeFromScene(bool permanent)
{
	if (!m_node)
		return;

	m_node->remove();
	m_node = NULL;
}

void TestCAO::updateLight(u32 day_night_ratio)
{
}

void TestCAO::updateNodePos()
{
	if (!m_node)
		return;

	m_node->setPosition(m_position);
	//m_node->setRotation(v3f(0, 45, 0));
}

void TestCAO::step(float dtime, ClientEnvironment *env)
{
	if(m_node)
	{
		v3f rot = m_node->getRotation();
		//infostream<<"dtime="<<dtime<<", rot.Y="<<rot.Y<<std::endl;
		rot.Y += dtime * 180;
		m_node->setRotation(rot);
	}
}

void TestCAO::processMessage(const std::string &data)
{
	infostream<<"TestCAO: Got data: "<<data<<std::endl;
	std::istringstream is(data, std::ios::binary);
	u16 cmd;
	is>>cmd;
	if(cmd == 0)
	{
		v3f newpos;
		is>>newpos.X;
		is>>newpos.Y;
		is>>newpos.Z;
		m_position = newpos;
		updateNodePos();
	}
}
