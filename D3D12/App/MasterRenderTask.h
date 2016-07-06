#pragma once

#include <d3d12.h>
#include <memory>
#include <tbb/task.h>
#include <vector>

#include <App/CommandListProcessor.h>
#include <RenderTask\CmdBuilderTask.h>
#include <RenderTask\InitTask.h>
#include <Timer/Timer.h>

// You should use the static method to span a new command list processor thread.
// Execute methods will check for command lists in its queue and execute them in cmdQueue.
// It will execute at most mMaxNumCmdLists per execution operation.
class MasterRenderTask : public tbb::task {
public:
	static tbb::empty_task* Create(MasterRenderTask* &masterRenderTask);

	MasterRenderTask() = default;

	__forceinline std::vector<std::unique_ptr<InitTask>>& GetInitTasks() noexcept { return mInitTasks; }
	__forceinline std::vector<std::unique_ptr<CmdBuilderTask>>& GetCmdBuilderTasks() noexcept { return mCmdBuilderTasks; }

	// Execute in order
	void Init(const HWND hwnd) noexcept;
	void ExecuteInitTasks() noexcept;
	
	void Terminate() noexcept { mTerminate = true; }

	// Called when spawned
	tbb::task* execute() override;

private:
	void InitSystems() noexcept;
	
	void ExecuteCmdBuilderTasks() noexcept;
	void Finalize() noexcept;

	void CreateRtvAndDsvDescriptorHeaps() noexcept;
	void CreateRtvAndDsv() noexcept;
	void CreateCommandObjects() noexcept;

	void CalculateFrameStats() noexcept;

	ID3D12Resource* CurrentBackBuffer() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const noexcept;

	void FlushCommandQueue() noexcept;
	void SignalFenceAndPresent() noexcept;

	HWND mHwnd{ 0 };

	Timer mTimer;

	tbb::empty_task* mCmdListProcessorParent{ nullptr };
	CommandListProcessor* mCmdListProcessor{ nullptr };

	ID3D12CommandQueue* mCmdQueue{ nullptr };
	ID3D12Fence* mFence{ nullptr };
	std::uint64_t mFenceByFrameIndex[Settings::sSwapChainBufferCount]{ 0UL };
	std::uint64_t mCurrentFence{ 0UL };

	// We have 2 commands lists (for frame begin and frame end), and 2
	// commands allocator per list
	ID3D12CommandAllocator* mCmdAllocFrameBegin[Settings::sSwapChainBufferCount]{ nullptr };
	ID3D12CommandAllocator* mCmdAllocFrameEnd[Settings::sSwapChainBufferCount]{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameBegin{ nullptr };
	ID3D12GraphicsCommandList* mCmdListFrameEnd{ nullptr };

	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[Settings::sSwapChainBufferCount];
	ID3D12Resource* mDepthStencilBuffer{ nullptr };

	ID3D12DescriptorHeap* mRtvHeap{ nullptr };
	ID3D12DescriptorHeap* mDsvHeap{ nullptr };

	std::vector<std::unique_ptr<InitTask>> mInitTasks;
	std::vector<std::unique_ptr<CmdBuilderTask>> mCmdBuilderTasks;
	
	bool mTerminate{ false };
};
