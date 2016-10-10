#pragma once

#include <memory>
#include <vector>

#include <GeometryPass/GeometryPassCmdListRecorder.h>
#include <LightPass/LightPassCmdListRecorder.h>
#include <SkyBoxPass/SkyBoxCmdListRecorder.h>

struct ID3D12CommandAllocator;
struct ID3D12CommandQueue;
struct ID3D12Fence;
struct ID3D12GraphicsCommandList;

// You should inherit from this class and implement needed methods.
// Its generated CmdListRecorder's are used by MasterRender to 
// create/record command lists for rendering purposes.
// You should pass an instance to App constructor.
class Scene {
public:
	Scene() = default;
	Scene(const Scene&) = delete;
	const Scene& operator=(const Scene&) = delete;

	// This method is called internally by the App.
	// User must not call it.
	// It must be called before generating recorders
	void Init() noexcept;
	
	virtual void GenerateGeomPassRecorders(
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, 
		std::vector<std::unique_ptr<GeometryPassCmdListRecorder>>& tasks) noexcept = 0;

	virtual void GenerateLightPassRecorders(
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue, 
		Microsoft::WRL::ComPtr<ID3D12Resource>* geometryBuffers,
		const std::uint32_t geometryBuffersCount,
		std::vector<std::unique_ptr<LightPassCmdListRecorder>>& tasks) noexcept = 0;

	virtual void GenerateSkyBoxRecorder(
		ID3D12CommandQueue& cmdQueue,
		tbb::concurrent_queue<ID3D12CommandList*>& cmdListQueue,
		std::unique_ptr<SkyBoxCmdListRecorder>& task) noexcept = 0;

protected:
	// Method used when the command list is ready to be closed
	// and executed. It waits until GPU finishes command list execution.
	void ExecuteCommandList(ID3D12CommandQueue& cmdQueue) noexcept;

	// Used to validate scene data is properly initialized.
	bool ValidateData() const;

	ID3D12CommandAllocator* mCmdAlloc{ nullptr };
	ID3D12GraphicsCommandList* mCmdList{ nullptr };
	ID3D12Fence* mFence{ nullptr };
};
