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

enum class MaterialBlendMode : u8
{
	NORMAL,
	ALPHA,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVISION,
	SCREEN,
	OVERLAY
};

class ShaderSource;

struct MaterialStorageEntry
{
	std::string Name{""};

	struct {
		bool enable{false};
		MaterialBlendMode mode{MaterialBlendMode::NORMAL};
	} Blend;

	struct {
		bool enable{true};
		video::E_COMPARISON_FUNC func{video::ECFN_LESS};
	} Depth;

	struct {
		bool enable{false};
		video::E_COMPARISON_FUNC func{video::ECFN_LESS};
	} Stencil;

	struct {
		bool enable{true};
		video::E_CULL_MODE mode{video::ECM_BACK};
	} Cull;

	u32 ShaderID{0};

	f32 LineWidth{1.0f};

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
