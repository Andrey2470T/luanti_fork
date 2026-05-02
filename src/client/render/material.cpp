#include "material.h"
#include "client/media/shader.h"

std::vector<video::GLTexture *> PBRTextures::getUsedTextures() const
{
	std::vector<video::GLTexture *> textures;

	if (Metallic)
		textures.push_back(Metallic);
	if (Roughness)
		textures.push_back(Roughness);
	if (AO)
		textures.push_back(AO);
	if (Normal)
		textures.push_back(Normal);
	if (Emission)
		textures.push_back(Emission);

	return textures;
}

void MaterialStorageEntry::applyToSMaterial(ShaderSource *shdsrc, video::SMaterial *mat)
{
	if (!mat)
		return;

	mat->MaterialType = shdsrc->getShaderInfo(ShaderID).material;

	auto pbr_textures = PBR.getUsedTextures();

	if (pbr_textures.empty())
		return;

	u8 texToPut = 0;
	mat->forEachTexture([&] (auto &tex) {
		if (!tex.Texture && texToPut < pbr_textures.size())
			tex.Texture = pbr_textures[texToPut++];
	});
}

bool MaterialStorageEntry::operator==(const MaterialStorageEntry &other) const
{
	return Name == other.Name;
}

const MaterialStorageEntry &MaterialStorage::getMaterial(const std::string &name) const
{
	auto it = std::find_if(Entries.begin(), Entries.end(),
		[&] (const auto &entry) { return name == entry.Name; });

	if (it == Entries.end()) {
		static MaterialStorageEntry empty_entry;
		return empty_entry;
	}

	return *it;
}

u32 MaterialStorage::getMaterialType(const std::string &name) const
{
	auto entry = getMaterial(name);

	if (entry.Name == "")
		return 0;

	return ShdSrc->getShaderInfo(entry.ShaderID).material;
}

void MaterialStorage::applyToSMaterial(const std::string &name, video::SMaterial *mat)
{
	auto entry = getMaterial(name);

	if (entry.Name == "")
		return;

	entry.applyToSMaterial(ShdSrc, mat);
}
