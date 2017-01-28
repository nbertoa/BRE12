#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include <DXUtils/D3DFactory.h>
#include <ResourceManager/VertexAndIndexBufferCreator.h>
#include <SettingsManager\SettingsManager.h>

struct FrameCBuffer;
class UploadBuffer;

// Responsible of command lists recording to be executed by CommandListExecutor.
// This class has common data and functionality to record command lists for deferred shading light pass.
// Steps:
// - Inherit from it and reimplement RecordAndPushCommandLists() method
// - Call RecordAndPushCommandLists() to create command lists to execute in the GPU
class LightingPassCmdListRecorder {
public:
	struct GeometryData {
		GeometryData() = default;

		VertexAndIndexBufferCreator::VertexBufferData mVertexBufferData;
		VertexAndIndexBufferCreator::IndexBufferData mIndexBufferData;
		std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
	};

	LightingPassCmdListRecorder();
	virtual ~LightingPassCmdListRecorder() {}

	LightingPassCmdListRecorder(const LightingPassCmdListRecorder&) = delete;
	const LightingPassCmdListRecorder& operator=(const LightingPassCmdListRecorder&) = delete;
	LightingPassCmdListRecorder(LightingPassCmdListRecorder&&) = default;
	LightingPassCmdListRecorder& operator=(LightingPassCmdListRecorder&&) = default;

	// Preconditions:
	// - "geometryBuffers" must not be nullptr
	// - "geometryBuffersCount" must be greater than zero
	// - "lights" must not be nullptr
	// - "numLights" must be greater than zero
	virtual void Init(
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		ID3D12Resource& depthBuffer,
		const void* lights,
		const std::uint32_t numLights) noexcept = 0;

	void InitInternal(const D3D12_CPU_DESCRIPTOR_HANDLE outputColorBufferCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called first
	// - InitInternal() must be called first
	virtual void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept = 0;

	// This method validates all data (nullptr's, etc)
	// When you inherit from this class, you should reimplement it to include
	// new members
	virtual bool IsDataValid() const noexcept;

protected:
	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };
	std::uint32_t mCurrentFrameIndex{ 0U };

	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)
	
	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };

	std::uint32_t mNumLights{ 0U };

	UploadBuffer* mFrameCBuffer[SettingsManager::sQueuedFrameCount]{ nullptr };
	UploadBuffer* mImmutableCBuffer{ nullptr };

	UploadBuffer* mLightsBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mLightsBufferGpuDescBegin{ 0UL };
};