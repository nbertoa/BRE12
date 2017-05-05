#pragma once

#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>
#include <Utils\DebugUtils.h>

class MaterialProperties;
class MaterialTechnique;
class Model;

class DrawableObject {
public:
	DrawableObject(
		const Model& model,
		const MaterialProperties& materialProperties,
		const MaterialTechnique& materialTechnique,
		const DirectX::XMFLOAT4X4& worldMatrix)
		: mModel(&model)
		, mMaterialProperties(&materialProperties)
		, mMaterialTechnique(&materialTechnique)
		, mWorldMatrix(worldMatrix)
	{}

	const Model& GetModel() const noexcept {
		ASSERT(mModel != nullptr);
		return *mModel;
	}

	const MaterialProperties& GetMaterialProperties() const noexcept {
		ASSERT(mMaterialProperties != nullptr);
		return *mMaterialProperties;
	}

	const MaterialTechnique& GetMaterialTechnique() const noexcept {
		ASSERT(mMaterialTechnique != nullptr);
		return *mMaterialTechnique;
	}

	const DirectX::XMFLOAT4X4& GetWorldMatrix() const noexcept { return mWorldMatrix; }

private:
	const Model* mModel{ nullptr };
	const MaterialProperties* mMaterialProperties{ nullptr };
	const MaterialTechnique* mMaterialTechnique{ nullptr };
	DirectX::XMFLOAT4X4 mWorldMatrix{ MathUtils::GetIdentity4x4Matrix() };
};
