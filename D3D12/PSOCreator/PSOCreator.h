#pragma once

#include <d3d12.h>

#include <DXUtils/D3DFactory.h>
#include <Utils/DebugUtils.h>

// Used to create Pipeline State Objects and Root Signatures (loaded from a shader file)
namespace PSOCreator {
	struct PSOData {
		PSOData() = default;

		bool ValidateData() const noexcept;

		ID3D12PipelineState* mPSO{ nullptr };
		ID3D12RootSignature* mRootSign{ nullptr };
	};

	// Common Pipeline State Objects and Root Signatures that
	// includes most common techniques (normal mapping, texture mapping, displacement mapping, etc)
	class CommonPSOData {
	public:
		CommonPSOData() = default;
		CommonPSOData(const CommonPSOData&) = delete;
		const CommonPSOData& operator=(const CommonPSOData&) = delete;

		enum Technique {
			BASIC = 0,
			TEXTURE_MAPPING,
			NORMAL_MAPPING,
			HEIGHT_MAPPING,
			TONE_MAPPING,
			PUNCTUAL_LIGHT,
			NUM_TECHNIQUES
		};

		static void Init() noexcept;
		static const PSOData& GetData(const Technique tech) noexcept;

	private:
		static PSOData mPSOData[Technique::NUM_TECHNIQUES];
	};
}
