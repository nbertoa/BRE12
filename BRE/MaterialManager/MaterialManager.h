#pragma once

#include <MaterialManager\Material.h>

class MaterialManager {
public:
	enum MaterialType {
		GOLD = 0U,
		SILVER,
		COPPER,
		IRON,
		ALUMINUM,
		PLASTIC_GLASS_LOW,
		PLASTIC_HIGH,
		GLASS_HIGH,
		NUM_MATERIALS
	};

	MaterialManager() = delete;
	~MaterialManager() = delete;
	MaterialManager(const MaterialManager&) = delete;
	const MaterialManager& operator=(const MaterialManager&) = delete;
	MaterialManager(MaterialManager&&) = delete;
	MaterialManager& operator=(MaterialManager&&) = delete;

	static void Init() noexcept;

	static const Material& GetMaterial(const MaterialType material) noexcept;

private:
	static Material mMaterials[NUM_MATERIALS];
};
