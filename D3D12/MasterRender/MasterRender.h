#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include <tbb/task.h>
#include <vector>

#include <Camera/Camera.h>
#include <CommandListProcessor/CommandListProcessor.h>
#include <DXUtils\CBuffers.h>
#include <GeometryPass\GeometryPass.h>
#include <GlobalData\Settings.h>
#include <LightPass\LightPass.h>
#include <SkyBoxPass\SkyBoxPass.h>
#include <ToneMappingPass\ToneMappingPass.h>
#include <Timer/Timer.h>
#include <Utils/DebugUtils.h>

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

	__forceinline static const DXGI_FORMAT FrameBufferRTFormat() noexcept { return sFrameBufferRTFormat; }
	__forceinline static const DXGI_FORMAT FrameBufferFormat() noexcept { return sFrameBufferFormat; }
	__forceinline static const DXGI_FORMAT ColorBufferFormat() noexcept { return sColorBufferFormat; }
	__forceinline static const DXGI_FORMAT DepthStencilFormat() noexcept { return sDepthStencilFormat; }

private:
	explicit MasterRender(const HWND hwnd, ID3D12Device& device, Scene* scene);

	// Called when tbb::task is spawned
	tbb::task* execute() override;

	void InitCmdListRecorders(Scene* scene) noexcept;

	void CreateRtvAndDsvDescriptorHeaps() noexcept;
	void CreateRtvAndDsv() noexcept;
	void CreateExtraBuffersRtvs() noexcept;
	void CreateCommandObjects() noexcept;
	
	ID3D12Resource* CurrentFrameBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentFrameBufferView() const noexcept;
	ID3D12Resource* DepthStencilBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	void MergeTask();

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	static const DXGI_FORMAT sFrameBufferRTFormat{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
	static const DXGI_FORMAT sFrameBufferFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	static const DXGI_FORMAT sColorBufferFormat{ DXGI_FORMAT_R16G16B16A16_FLOAT };
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

	GeometryPass mGeometryPass;
	LightPass mLightPass;
	SkyBoxPass mSkyBoxPass;
	ToneMappingPass mToneMappingPass;

	ID3D12CommandAllocator* mCmdAllocs[Settings::sQueuedFrameCount]{ nullptr };
	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> mColorBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferRTVCpuDescHandle;
	ID3D12DescriptorHeap* mBuffersRTVDescHeap{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };

	FrameCBuffer mFrameCBuffer;
	
	bool mTerminate{ false };
};
