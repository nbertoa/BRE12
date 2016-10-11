#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tbb/task.h>

#include <Camera/Camera.h>
#include <GeometryPass\GeometryPass.h>
#include <GlobalData\Settings.h>
#include <LightPass\LightPass.h>
#include <SkyBoxPass\SkyBoxPass.h>
#include <ShaderUtils\CBuffers.h>
#include <ToneMappingPass\ToneMappingPass.h>
#include <Timer/Timer.h>
#include <Utils/DebugUtils.h>

class CommandListProcessor;
class Scene;

// Initializes passes (geometry, light, skybox, etc) based on a Scene.
// Steps:
// - Use MasterRender::Create() to create and spawn and instance. 
// - When you want to terminate this task, you should call MasterRender::Terminate()
class MasterRender : public tbb::task {
public:
	static MasterRender* Create(const HWND hwnd, ID3D12Device& device, Scene* scene) noexcept;

	void Terminate() noexcept;
	__forceinline static const DXGI_FORMAT DepthStencilFormat() noexcept { return sDepthStencilFormat; }

private:
	static const DXGI_FORMAT sDepthStencilFormat{ DXGI_FORMAT_D24_UNORM_S8_UINT };

	explicit MasterRender(const HWND hwnd, ID3D12Device& device, Scene* scene);

	// Called when tbb::task is spawned
	tbb::task* execute() override;

	void InitPasses(Scene* scene) noexcept;

	void CreateRtvAndDsvDescriptorHeaps() noexcept;
	void CreateRtvAndDsv() noexcept;
	void CreateColorBuffer() noexcept;
	void CreateMergePassCommandObjects() noexcept;
	
	ID3D12Resource* CurrentFrameBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentFrameBufferCpuDesc() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilCpuDesc() const noexcept;

	void ExecuteMergePass();

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	HWND mHwnd{ 0 };
	ID3D12Device& mDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain{ nullptr };
	ID3D12CommandQueue* mCmdQueue{ nullptr };
			
	CommandListProcessor* mCmdListProcessor{ nullptr };
	
	// Fences data for syncrhonization purposes.
	ID3D12Fence* mFence{ nullptr };
	std::uint32_t mCurrQueuedFrameIndex{ 0U };
	std::uint64_t mFenceValueByQueuedFrameIndex[Settings::sQueuedFrameCount]{ 0UL };
	std::uint64_t mCurrFenceValue{ 0UL };

	// Passes
	GeometryPass mGeometryPass;
	LightPass mLightPass;
	SkyBoxPass mSkyBoxPass;
	ToneMappingPass mToneMappingPass;

	// Command allocarts and list needed for merge pass
	ID3D12CommandAllocator* mMergePassCmdAllocs[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mMergePassCmdList{ nullptr };
	
	// Frame buffers and depth stencil buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> mFrameBuffers[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	// Color buffer is a buffer used for intermediate computations.
	// It is used as render target (light pass) or pixel shader resource (post processing passes)
	Microsoft::WRL::ComPtr<ID3D12Resource> mColorBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferRTVCpuDescHandle;
	ID3D12DescriptorHeap* mColorBufferDescHeap{ nullptr };

	// Render target and depth stencil descriptor heaps
	ID3D12DescriptorHeap* mRenderTargetDescHeap{ nullptr };
	ID3D12DescriptorHeap* mDepthStencilDescHeap{ nullptr };

	// Per frame constant buffer.
	// We cache it here, as is is used by most passes.
	FrameCBuffer mFrameCBuffer;

	Camera mCamera;
	Timer mTimer;
	
	// When it is true, master render thread is destroyed.
	bool mTerminate{ false };
};
