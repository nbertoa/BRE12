#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include <tbb/task.h>
#include <vector>

#include <App/CommandListProcessor.h>
#include <Camera/Camera.h>
#include <GlobalData\Settings.h>
#include <Timer/Timer.h>
#include <Utils/DebugUtils.h>
;
class Scene;
class CmdListRecorder;

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

	__forceinline static const DXGI_FORMAT BackBufferFormat() noexcept { return sBackBufferFormat; }
	__forceinline static const DXGI_FORMAT* RTVFormats() noexcept { return sRTVFormats; }
	__forceinline static const DXGI_FORMAT* GeomPassBuffersFormats() noexcept { return sGeomPassBufferFormats; }


	//
	// TODO
	//
	__forceinline static const std::uint32_t NumRenderTargets() noexcept { return GEOMBUFFERS_COUNT + 1U; }
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
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	static const DXGI_FORMAT sBackBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	static const DXGI_FORMAT sRTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
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

	// We have 2 commands lists (for frame begin and frame end), and 2
	// commands allocator per list
	ID3D12CommandAllocator* mCmdAllocFrameBegin[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocFrameEnd[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameBegin{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameEnd{ nullptr };
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	// Buffers used for geometry pass in deferred shading. They are:
	// - Normals
	// - Positions
	// - BaseColor_MetalMask
	// - Reflectance_Smoothness
	enum GeomBuffers {
		NORMAL = 0U,
		POSITION,
		BASECOLOR_METALMASK,
		REFLECTANCE_SMOOTHNESS,
		GEOMBUFFERS_COUNT
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> mGeomPassBuffers[GEOMBUFFERS_COUNT];
	D3D12_CPU_DESCRIPTOR_HANDLE mGeomPassBuffersRTVCpuDescHandles[GEOMBUFFERS_COUNT];
	ID3D12DescriptorHeap* mGeomPassBuffersRTVDescHeap{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };

	std::vector<std::unique_ptr<CmdListRecorder>> mCmdListRecorders;

	DirectX::XMFLOAT4X4 mView{ MathHelper::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathHelper::Identity4x4() };
	
	bool mTerminate{ false };
};
