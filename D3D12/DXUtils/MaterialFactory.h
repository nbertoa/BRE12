#pragma once

#include "Material.h"

class MaterialFactory {
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

	MaterialFactory() = delete;
	MaterialFactory(const MaterialFactory&) = delete;
	const MaterialFactory& operator=(const MaterialFactory&) = delete;

	static void InitMaterials() noexcept;
	static const Material& GetMaterial(const MaterialType material) noexcept;
};
