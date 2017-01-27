#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include <MathUtils\MathUtils.h>
#include <ResourceManager/BufferCreator.h>
#include <SettingsManager\SettingsManager.h>

struct FrameCBuffer;
class UploadBuffer;

class SkyBoxCmdListRecorder {
public:
	SkyBoxCmdListRecorder();
	~SkyBoxCmdListRecorder() = default;
	SkyBoxCmdListRecorder(const SkyBoxCmdListRecorder&) = delete;
	const SkyBoxCmdListRecorder& operator=(const SkyBoxCmdListRecorder&) = delete;
	SkyBoxCmdListRecorder(SkyBoxCmdListRecorder&&) = delete;
	SkyBoxCmdListRecorder& operator=(SkyBoxCmdListRecorder&&) = delete;

	// This method is to initialize PSO that is a shared between all this kind
	// of recorders.
	// This method is initialized by its corresponding pass.
	static void InitPSO() noexcept;

	void Init(
		const BufferCreator::VertexBufferData& vertexBufferData, 
		const BufferCreator::IndexBufferData indexBufferData,
		const DirectX::XMFLOAT4X4& worldMatrix,
		ID3D12Resource& skyBoxCubeMap,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called first
	void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept;

	bool IsDataValid() const noexcept;

private:
	void BuildBuffers(ID3D12Resource& skyBoxCubeMap) noexcept;

	ID3D12GraphicsCommandList* mCommandList{ nullptr };
	ID3D12CommandAllocator* mCommandAllocators[SettingsManager::sQueuedFrameCount]{ nullptr };
	
	BufferCreator::VertexBufferData mVertexBufferData;
	BufferCreator::IndexBufferData mIndexBufferData;
	DirectX::XMFLOAT4X4 mWorldMatrix{ MathUtils::GetIdentity4x4Matrix() };

	UploadBuffer* mFrameCBuffer[SettingsManager::sQueuedFrameCount]{ nullptr };

	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescBegin;

	D3D12_GPU_DESCRIPTOR_HANDLE mCubeMapBufferGpuDescBegin;

	D3D12_CPU_DESCRIPTOR_HANDLE mOutputColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };
};