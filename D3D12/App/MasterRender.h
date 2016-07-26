#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include <tbb/task.h>
#include <vector>

#include <App/CommandListProcessor.h>
#include <Camera/Camera.h>
#include <RenderTask\CmdListRecorder.h>
#include <Timer/Timer.h>

class Scene;

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

private:
	explicit MasterRender(const HWND hwnd, ID3D12Device& device, Scene* scene);

	// Called when tbb::task is spawned
	tbb::task* execute() override;

	void InitCmdListRecorders(Scene* scene) noexcept;

	void CreateRtvAndDsvDescriptorHeaps() noexcept;
	void CreateRtvAndDsv() noexcept;
	void CreateCommandObjects() noexcept;
	
	ID3D12Resource* CurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	HWND mHwnd{ 0 };
	ID3D12Device& mDevice;
	IDXGISwapChain3* mSwapChain{ nullptr };

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

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };

	std::vector<std::unique_ptr<CmdListRecorder>> mCmdListRecorders;

	DirectX::XMFLOAT4X4 mView{ MathHelper::Identity4x4() };
	DirectX::XMFLOAT4X4 mProj{ MathHelper::Identity4x4() };
	
	bool mTerminate{ false };
};
