#pragma once

#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <tbb/task.h>
#include <wrl.h>

class CommandListProcessor : public tbb::task {
public:
	static tbb::empty_task* Create(CommandListProcessor* &cmdListProcessor, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& cmdQueue);

	CommandListProcessor(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& cmdQueue);
	
	tbb::task* execute() override;

	__forceinline tbb::concurrent_queue<ID3D12CommandList*>* cmdListQueue() noexcept { return &mCmdListQueue; }

private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>& mCmdQueue;
	tbb::concurrent_queue<ID3D12CommandList*> mCmdListQueue;
};
