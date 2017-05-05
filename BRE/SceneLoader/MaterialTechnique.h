#pragma once

struct ID3D12Resource;

class MaterialTechnique {
public:
	enum class TechniqueType {
		COLOR_MAPPING = 0,
		COLOR_NORMAL_MAPPING,
		COLOR_HEIGHT_MAPPING,
		TEXTURE_MAPPING,
		NORMAL_MAPPING,
		HEIGHT_MAPPING,	
	};

	MaterialTechnique(
		ID3D12Resource* diffuseTexture = nullptr,
		ID3D12Resource* normalTexture = nullptr,
		ID3D12Resource* heightTexture = nullptr)
		: mDiffuseTexture(diffuseTexture)
		, mNormalTexture(normalTexture)
		, mHeightTexture(heightTexture)
	{}

	ID3D12Resource* &GetDiffuseTexture() noexcept { return mDiffuseTexture; }
	ID3D12Resource* &GetNormalTexture() noexcept { return mNormalTexture; }
	ID3D12Resource* &GetHeightTexture() noexcept { return mHeightTexture; }

	TechniqueType GetType() const noexcept;

private:
	ID3D12Resource* mDiffuseTexture{ nullptr };
	ID3D12Resource* mNormalTexture{ nullptr };
	ID3D12Resource* mHeightTexture{ nullptr };
};