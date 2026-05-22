#include "material.h"
#include "client/media/shader.h"

void MaterialStorageEntry::applyToSMaterial(ShaderSource *shdsrc, video::SMaterial *mat)
{
	if (!mat)
		return;

	mat->MaterialType = shdsrc->getShaderInfo(ShaderID).material;

	if (StateFlags & MF_BLEND)
		mat->BlendMode = Blend;
	if (StateFlags & MF_DEPTH)
		mat->ZBuffer = Depth;
	if (StateFlags & MF_STENCIL)
		mat->StencilBuffer = Stencil;
	if (StateFlags & MF_CULL) {
		if (!Cull.enable) {
			mat->BackfaceCulling = false;
			mat->FrontfaceCulling = false;
		}
		else {
			if (Cull.mode == video::ECM_BACK || Cull.mode == video::ECM_FRONT_AND_BACK)
				mat->BackfaceCulling = true;
			if (Cull.mode == video::ECM_FRONT || Cull.mode == video::ECM_FRONT_AND_BACK)
				mat->FrontfaceCulling = true;
		}
	}

	if (Samplers.empty())
		return;

	u8 texToPut = 0;
	mat->forEachTexture([&] (auto &tex) {
		if (!tex.Texture && texToPut < Samplers.size())
			tex.Texture = Samplers[texToPut++];
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
