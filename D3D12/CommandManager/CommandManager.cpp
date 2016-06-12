#include "CommandManager.h"

#include <Utils/DebugUtils.h>
#include <Utils/RandomNumberGenerator.h>

std::unique_ptr<CommandManager> CommandManager::gManager = nullptr;

std::size_t CommandManager::CreateCmdQueue(const D3D12_COMMAND_QUEUE_DESC& desc, ID3D12CommandQueue* &cmdQueue) noexcept {
	const std::size_t id{ sizeTRand() };

	CmdQueueById::accessor accessor;
#ifdef _DEBUG
	mCmdQueueById.find(accessor, id);
	ASSERT(accessor.empty());
#endif

	mMutex.lock();
	CHECK_HR(mDevice.CreateCommandQueue(&desc, IID_PPV_ARGS(&cmdQueue)));
	mMutex.unlock();

	mCmdQueueById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12CommandQueue>(cmdQueue);
	accessor.release();

	return id;
}

std::size_t CommandManager::CreateCmdList(const D3D12_COMMAND_LIST_TYPE& type, ID3D12CommandAllocator& cmdAlloc, ID3D12GraphicsCommandList* &cmdList) noexcept {
	const std::size_t id{ sizeTRand() };

	CmdListById::accessor accessor;
#ifdef _DEBUG
	mCmdListById.find(accessor, id);
	ASSERT(accessor.empty());
#endif

	mMutex.lock();
	CHECK_HR(mDevice.CreateCommandList(0U, type, &cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList)));
	mMutex.unlock();

	mCmdListById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>(cmdList);
	accessor.release();

	return id;
}

std::size_t CommandManager::CreateCmdAlloc(const D3D12_COMMAND_LIST_TYPE& type, ID3D12CommandAllocator* &cmdAlloc) noexcept {
	const std::size_t id{ sizeTRand() };
	
	CmdAllocById::accessor accessor;
#ifdef _DEBUG
	mCmdAllocById.find(accessor, id);
	ASSERT(accessor.empty());
#endif

	mMutex.lock();
	CHECK_HR(mDevice.CreateCommandAllocator(type, IID_PPV_ARGS(&cmdAlloc)));
	mMutex.unlock();

	mCmdAllocById.insert(accessor, id);
	accessor->second = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>(cmdAlloc);
	accessor.release();

	return id;
}

ID3D12CommandQueue& CommandManager::GetCmdQueue(const std::size_t id) noexcept {
	CmdQueueById::accessor accessor;
	mCmdQueueById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12CommandQueue* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}

ID3D12GraphicsCommandList& CommandManager::GetCmdList(const std::size_t id) noexcept {
	CmdListById::accessor accessor;
	mCmdListById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12GraphicsCommandList* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}

ID3D12CommandAllocator& CommandManager::GetCmdAlloc(const std::size_t id) noexcept {
	CmdAllocById::accessor accessor;
	mCmdAllocById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12CommandAllocator* elem{ accessor->second.Get() };
	accessor.release();

	return *elem;
}