#pragma once

#include <memory>
#include <vector>

#include <Scene/CmdListRecorder.h>

struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;

// Used when you need to create resources and upload them to the GPU.
// You need a command list for this, and to flush command queue to 
// be sure all resources were properly uploaded.
class CmdListHelper {
public:
	CmdListHelper(ID3D12CommandQueue& cmdQueue, ID3D12Fence& fence, std::uint64_t& currentFence, ID3D12GraphicsCommandList& cmdList);
	CmdListHelper(const CmdListHelper&) = delete;
	const CmdListHelper& operator=(const CmdListHelper&) = delete;

	__forceinline ID3D12GraphicsCommandList& CmdList() noexcept { return mCmdList; }
	void ExecuteCmdList() noexcept;
private:
	ID3D12CommandQueue& mCmdQueue;
	ID3D12Fence& mFence;
	std::uint64_t& mCurrentFence;
	ID3D12GraphicsCommandList& mCmdList;
};

// You should inherit from this class and implement needed methods.
// Its generated CmdListRecorder's are used by MasterRender to 
// create/record command lists for rendering purposes.
// You should pass an instance to App constructor.
class Scene {
public:
	Scene() = default;
	Scene(const Scene&) = delete;
	const Scene& operator=(const Scene&) = delete;

	virtual void GenerateGeomPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, 
		CmdListHelper& cmdListHelper, 
		std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept = 0;

	virtual void GenerateLightPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, 
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept = 0;

	virtual void GeneratePostProcessingPassRecorders(tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, std::vector<std::unique_ptr<CmdListRecorder>>& tasks) const noexcept = 0;
};
