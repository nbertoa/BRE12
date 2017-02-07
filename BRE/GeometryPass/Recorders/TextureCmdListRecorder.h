#pragma once

#include <GeometryPass/GeometryPassCmdListRecorder.h>

struct Material;

// CommandListRecorders that does texture mapping
class TextureCmdListRecorder : public GeometryPassCmdListRecorder {
public:
	TextureCmdListRecorder() = default;
	~TextureCmdListRecorder() = default;
	TextureCmdListRecorder(const TextureCmdListRecorder&) = delete;
	const TextureCmdListRecorder& operator=(const TextureCmdListRecorder&) = delete;
	TextureCmdListRecorder(TextureCmdListRecorder&&) = default;
	TextureCmdListRecorder& operator=(TextureCmdListRecorder&&) = default;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO(const DXGI_FORMAT* geometryBufferFormats, const std::uint32_t geometryBufferCount) noexcept;

	// Preconditions:
	// - "geometryDataVec" must not be nullptr
	// - "geometryDataCount" must be greater than zero
	// - "materials" must not be nullptr
	// - "textures" must not be nullptr
	// - "numResources" must be greater than zero
	void Init(
		const GeometryData* geometryDataVec,
		const std::uint32_t geometryDataCount,
		const Material* materials,
		ID3D12Resource** textures,
		const std::uint32_t numResources) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept final override;

	bool IsDataValid() const noexcept final override;

private:
	// Preconditions:
	// - "materials" must not be nullptr
	// - "textures" must not be nullptr
	// - "dataCount" must be greater than zero
	void InitConstantBuffers(
		const Material* materials, 
		ID3D12Resource** textures, 
		const std::uint32_t dataCount) noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE mBaseColorBufferGpuDescBegin;
};