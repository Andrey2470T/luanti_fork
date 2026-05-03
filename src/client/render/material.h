#pragma once

#include "Video/DrawContext.h"
#include "Video/SMaterial.h"
#include "Video/Texture.h"
#include "tile.h"
#include "irrlichttypes_bloated.h"
#include <unordered_set>

struct PBRTextures
{
	video::GLTexture *Metallic{nullptr};
	video::GLTexture *Roughness{nullptr};
	video::GLTexture *AO{nullptr};
	video::GLTexture *Normal{nullptr};
	video::GLTexture *Emission{nullptr};

	std::vector<video::GLTexture *> getUsedTextures() const;
};

enum MaterialStateFlags : u8
{
	MF_NONE =	 0x00,
	MF_BLEND =	 0x01,
	MF_DEPTH =	 0x02,
	MF_STENCIL = 0x04,
	MF_CULL =	 0x07
};

class ShaderSource;

struct MaterialStorageEntry
{
	std::string Name{""};

	video::E_BLEND_MODE Blend{video::EBM_NONE};

	video::E_COMPARISON_FUNC Depth{video::ECFN_LESS};

	video::E_COMPARISON_FUNC Stencil{video::ECFN_LESS};

	struct {
		bool enable{true};
		video::E_CULL_MODE mode{video::ECM_BACK};
	} Cull;

	u8 StateFlags{MF_NONE};

	u32 ShaderID{0};

	PBRTextures PBR;

	void applyToLayer(TileLayer *layer) const;
	void applyToSMaterial(ShaderSource *shdsrc, video::SMaterial *mat);

	bool operator==(const MaterialStorageEntry &other) const;
};

namespace std {
	template <>
	struct hash<MaterialStorageEntry> {
		size_t operator()(const MaterialStorageEntry& entry) const {
			return hash<string>()(entry.Name);
		}
	};
}

class MaterialStorage
{
	ShaderSource *ShdSrc;
	std::unordered_set<MaterialStorageEntry> Entries;

public:
	MaterialStorage(ShaderSource *shdsrc)
		: ShdSrc(shdsrc)
	{}

	void addMaterial(const MaterialStorageEntry &entry)
	{
		Entries.emplace(entry);
	}

	const MaterialStorageEntry &getMaterial(const std::string &name) const;
	u32 getMaterialType(const std::string &name) const;

	void applyToSMaterial(const std::string &name, video::SMaterial *mat);
};
