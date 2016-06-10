#include "CommandManager.h"

#include <Utils/DebugUtils.h>
#include <Utils/RandomNumberGenerator.h>

std::unique_ptr<CommandManager> CommandManager::gManager = nullptr;

std::size_t CommandManager::CreateCmdList(const D3D12_COMMAND_LIST_TYPE& type, ID3D12CommandAllocator& cmdAlloc, ID3D12GraphicsCommandList* &cmdList) noexcept {
	const std::size_t id{ sizeTRand() };

	CmdListById::accessor accessor;
	mCmdListById.find(accessor, id);
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> auxCmdList;
	if (!accessor.empty()) {
		auxCmdList = accessor->second.Get();
	}
	else {		
		mMutex.lock();
		CHECK_HR(mDevice.CreateCommandList(0U, type, &cmdAlloc, nullptr, IID_PPV_ARGS(auxCmdList.GetAddressOf())));
		mMutex.unlock();

		mCmdListById.insert(accessor, id);
		accessor->second = auxCmdList;
	}
	accessor.release();

	cmdList = auxCmdList.Get();

	return id;
}

std::size_t CommandManager::CreateCmdAlloc(const D3D12_COMMAND_LIST_TYPE& type, ID3D12CommandAllocator* &cmdAlloc) noexcept {
	const std::size_t id{ sizeTRand() };
	
	CmdAllocById::accessor accessor;
	mCmdAllocById.find(accessor, id);
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> auxCmdAlloc;
	if (!accessor.empty()) {
		auxCmdAlloc = accessor->second.Get();
	}
	else {
		mMutex.lock();
		CHECK_HR(mDevice.CreateCommandAllocator(type, IID_PPV_ARGS(auxCmdAlloc.GetAddressOf())));
		mMutex.unlock();

		mCmdAllocById.insert(accessor, id);
		accessor->second = auxCmdAlloc;
	}
	accessor.release();

	cmdAlloc = auxCmdAlloc.Get();

	return id;
}

ID3D12GraphicsCommandList& CommandManager::GetCmdList(const std::size_t id) noexcept {
	CmdListById::accessor accessor;
	mCmdListById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12GraphicsCommandList* cmdList{ accessor->second.Get() };
	accessor.release();

	return *cmdList;
}

ID3D12CommandAllocator& CommandManager::GetCmdAlloc(const std::size_t id) noexcept {
	CmdAllocById::accessor accessor;
	mCmdAllocById.find(accessor, id);
	ASSERT(!accessor.empty());
	ID3D12CommandAllocator* cmdAlloc{ accessor->second.Get() };
	accessor.release();

	return *cmdAlloc;
}