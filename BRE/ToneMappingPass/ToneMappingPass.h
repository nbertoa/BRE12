#pragma once

#include <memory>

#include <CommandManager\CommandListPerFrame.h>
#include <ToneMappingPass\ToneMappingCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

class ToneMappingPass {
public:
	ToneMappingPass() = default;
	~ToneMappingPass() = default; 
	ToneMappingPass(const ToneMappingPass&) = delete;
	const ToneMappingPass& operator=(const ToneMappingPass&) = delete;
	ToneMappingPass(ToneMappingPass&&) = delete;
	ToneMappingPass& operator=(ToneMappingPass&&) = delete;

	void Init(
		ID3D12Resource& inputColorBuffer,
		ID3D12Resource& outputColorBuffer,
		const D3D12_CPU_DESCRIPTOR_HANDLE& outputBufferCpuDesc) noexcept;

	// Preconditions:
	// - Init() must be called before
	void Execute() noexcept;

private:
	bool IsDataValid() const noexcept;

	void ExecuteBeginTask() noexcept;

	CommandListPerFrame mCommandListPerFrame;
	
	ID3D12Resource* mInputColorBuffer{ nullptr };
	ID3D12Resource* mOutputColorBuffer{ nullptr };

	std::unique_ptr<ToneMappingCmdListRecorder> mCommandListRecorder;
};
