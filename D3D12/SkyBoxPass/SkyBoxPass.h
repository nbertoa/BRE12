#pragma once

#include <memory>
#include <vector>

#include <SkyBoxPass\SkyBoxCmdListRecorder.h>

class CommandListProcessor;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct FrameCBuffer;

class SkyBoxPass {
public:
	using Recorder = std::unique_ptr<SkyBoxCmdListRecorder>;

	SkyBoxPass() = default;
	SkyBoxPass(const SkyBoxPass&) = delete;
	const SkyBoxPass& operator=(const SkyBoxPass&) = delete;

	// You should get recorder and fill it, before calling Init()
	__forceinline Recorder& GetRecorder() noexcept { return mRecorder; }

	// You should call this method after filling recorder and before Execute()
	void Init(
		const D3D12_CPU_DESCRIPTOR_HANDLE& colorBufferCpuDesc,
		const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferCpuDesc) noexcept;

	void Execute(CommandListProcessor& cmdListProcessor, const FrameCBuffer& frameCBuffer) noexcept;

private:
	// Method used internally for validation purposes
	bool ValidateData() const noexcept;

	// Color & Depth buffers cpu descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE mColorBufferCpuDesc{ 0UL };
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferCpuDesc{ 0UL };

	Recorder mRecorder;
};
