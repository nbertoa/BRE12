#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include <tbb/task.h>
#include <vector>

#include <Camera/Camera.h>
#include <DXUtils\CBuffers.h>
#include <GlobalData\Settings.h>
#include <MasterRender/CommandListProcessor.h>
#include <Timer/Timer.h>
#include <Utils/DebugUtils.h>

class GeometryPassCmdListRecorder;
class LightPassCmdListRecorder;
class Scene;
class SkyBoxCmdListRecorder;

// It has the responsibility to build CmdListRecorder's and also execute them 
// (to record command lists and push to the queue provided by CommandListProcessor)
// Steps:
// - Use MasterRender::Create() to create and spawn and instance.
// - When you spawn it, execute() method is automatically called. 
// - When you want to terminate this task, you should call MasterRender::Terminate()
class MasterRender : public tbb::task {
public:
	static MasterRender* Create(const HWND hwnd, ID3D12Device& device, Scene* scene) noexcept;

	void Terminate() noexcept;

	__forceinline static const DXGI_FORMAT BackBufferRTFormat() noexcept { return sBackBufferRTFormat; }
	__forceinline static const DXGI_FORMAT BackBufferFormat() noexcept { return sBackBufferFormat; }
	__forceinline static const DXGI_FORMAT* GeomPassBuffersFormats() noexcept { return sGeomPassBufferFormats; }

	__forceinline static const std::uint32_t NumRenderTargets() noexcept { return GEOMBUFFERS_COUNT; }
	__forceinline static const DXGI_FORMAT DepthStencilFormat() noexcept { return sDepthStencilFormat; }

private:
	explicit MasterRender(const HWND hwnd, ID3D12Device& device, Scene* scene);

	// Called when tbb::task is spawned
	tbb::task* execute() override;

	void InitCmdListRecorders(Scene* scene) noexcept;

	void CreateRtvAndDsvDescriptorHeaps() noexcept;
	void CreateRtvAndDsv() noexcept;
	void CreateGeometryPassRtvs() noexcept;
	void CreateCommandObjects() noexcept;
	
	ID3D12Resource* CurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	ID3D12Resource* DepthStencilBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	void BeginFrameTask();
	void MiddleFrameTask();
	void EndFrameTask();

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	static const DXGI_FORMAT sBackBufferRTFormat{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
	static const DXGI_FORMAT sBackBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	static const DXGI_FORMAT sGeomPassBufferFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	static const DXGI_FORMAT sDepthStencilFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT };

	HWND mHwnd{ 0 };
	ID3D12Device& mDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain{ nullptr };

	Camera mCamera;

	Timer mTimer;
		
	CommandListProcessor* mCmdListProcessor{ nullptr };

	ID3D12CommandQueue* mCmdQueue{ nullptr };
	ID3D12Fence* mFence{ nullptr };
	std::uint32_t mCurrQueuedFrameIndex{ 0U };
	std::uint64_t mFenceByQueuedFrameIndex[Settings::sQueuedFrameCount]{ 0UL };
	std::uint64_t mCurrentFence{ 0UL };

	// We have 3 commands lists (for frame begin, frame middle and frame end), and 2
	// commands allocator per list
	ID3D12CommandAllocator* mCmdAllocFrameBegin[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocFrameMiddle[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocFrameEnd[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameBegin{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameMiddle{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameEnd{ nullptr };
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	// Buffers used for geometry pass in deferred shading.
	enum GeomBuffers {
		NORMAL_SMOOTHNESS_DEPTH = 0U,		
		BASECOLOR_METALMASK,
		SPECULARREFLECTION,
		GEOMBUFFERS_COUNT
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> mGeomPassBuffers[GEOMBUFFERS_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE mGeomPassBuffersRTVCpuDescHandles[GEOMBUFFERS_COUNT];
	ID3D12DescriptorHeap* mGeomPassBuffersRTVDescHeap{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };

	std::vector<std::unique_ptr<GeometryPassCmdListRecorder>> mGeomPassCmdListRecorders;

	std::vector<std::unique_ptr<LightPassCmdListRecorder>> mLightPassCmdListRecorders;

	std::unique_ptr<SkyBoxCmdListRecorder> mSkyBoxCmdListRecorder;

	FrameCBuffer mFrameCBuffer;
	
	bool mTerminate{ false };
};
