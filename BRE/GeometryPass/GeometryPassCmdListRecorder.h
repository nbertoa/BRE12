#pragma once

#include <d3d12.h>
#include <DirectXMath.h>

#include <DXUtils/D3DFactory.h>
#include <ResourceManager/BufferCreator.h>
#include <SettingsManager\SettingsManager.h>

struct FrameCBuffer;
class UploadBuffer;

// This class has common data and functionality to record command lists for deferred shading geometry pass.
// Steps:
// - Inherit from it and reimplement RecordAndPushCommandLists() method
// - Call RecordAndPushCommandLists() to create command lists to execute in the GPU
class GeometryPassCmdListRecorder {
public:
	struct GeometryData {
		GeometryData() = default;

		BufferCreator::VertexBufferData mVertexBufferData;
		BufferCreator::IndexBufferData mIndexBufferData;
		std::vector<DirectX::XMFLOAT4X4> mWorldMatrices;
	};

	GeometryPassCmdListRecorder();
	virtual ~GeometryPassCmdListRecorder() {}

	GeometryPassCmdListRecorder(const GeometryPassCmdListRecorder&) = delete;
	const GeometryPassCmdListRecorder& operator=(const GeometryPassCmdListRecorder&) = delete;
	GeometryPassCmdListRecorder(GeometryPassCmdListRecorder&&) = default;
	GeometryPassCmdListRecorder& operator=(GeometryPassCmdListRecorder&&) = default;

	// This method must be called before calling RecordAndPushCommandLists()
	void InitInternal(
		const D3D12_CPU_DESCRIPTOR_HANDLE* geometryBuffersCpuDescs,
		const std::uint32_t geometryBuffersCpuDescCount,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	// Record command lists and push them to the queue.
	virtual void RecordAndPushCommandLists(const FrameCBuffer& frameCBuffer) noexcept = 0;

	// This method validates all data (nullptr's, etc)
	// When you inherit from this class, you should reimplement it to include
	// new members
	virtual bool IsDataValid() const noexcept;

protected:
	// 1 command allocater per queued frame.
	ID3D12CommandAllocator* mCmdAlloc[SettingsManager::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	std::uint32_t mCurrFrameIndex{ 0U };

	// Base command data. Once you inherits from this class, you should add
	// more class members that represent the extra information you need (like resources, for example)

	std::vector<GeometryData> mGeometryDataVec;

	// Frame CBuffer info per queued frame.
	UploadBuffer* mFrameCBuffer[SettingsManager::sQueuedFrameCount]{ nullptr };

	// Object CBuffer info
	UploadBuffer* mObjectCBuffer{ nullptr };
	D3D12_GPU_DESCRIPTOR_HANDLE mObjectCBufferGpuDescBegin;

	// Material CBuffer info
	D3D12_GPU_DESCRIPTOR_HANDLE mMaterialsCBufferGpuDescBegin;
	UploadBuffer* mMaterialsCBuffer{ nullptr };
	
	// Geometry & depth buffers cpu descriptors
	const D3D12_CPU_DESCRIPTOR_HANDLE* mGeometryBuffersCpuDescs{ nullptr };
	std::uint32_t mGeometryBuffersCpuDescCount{ 0U };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };
};