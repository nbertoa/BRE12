#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

// Recorder that does texture mapping
class TextureCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	explicit TextureCmdListRecorder(ID3D12Device& device);

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	// This method must be called before calling RecordAndPushCommandLists()
	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t numGeomData,
		const Material* materials,
		ID3D12Resource** textures,
		const std::uint32_t numResources) noexcept;

	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept override;

	bool ValidateData() const noexcept override;

private:
	void BuildBuffers(const Material* materials, ID3D12Resource** textures, const std::uint32_t dataCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mTexturesBufferGpuDescHandleBegin;
};