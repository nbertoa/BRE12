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

	// Preconditions:
	// - Create() must be called once
	static MaterialManager& Create() noexcept;

	// Preconditions:
	// - Create() must be called before this method
	static MaterialManager& Get() noexcept;

	~MaterialManager() = default;
	MaterialManager(const MaterialManager&) = delete;
	const MaterialManager& operator=(const MaterialManager&) = delete;
	MaterialManager(MaterialManager&&) = delete;
	MaterialManager& operator=(MaterialManager&&) = delete;

	const Material& GetMaterial(const MaterialType material) noexcept;

private:
	MaterialManager();

	Material mMaterials[NUM_MATERIALS];
};
