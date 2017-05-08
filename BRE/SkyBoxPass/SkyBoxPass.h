#pragma once

#include <memory>

#include <SkyBoxPass\SkyBoxCmdListRecorder.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

namespace BRE {
struct FrameCBuffer;

class SkyBoxPass {
public:
    SkyBoxPass() = default;
    ~SkyBoxPass() = default;
    SkyBoxPass(const SkyBoxPass&) = delete;
    const SkyBoxPass& operator=(const SkyBoxPass&) = delete;
    SkyBoxPass(SkyBoxPass&&) = delete;
    SkyBoxPass& operator=(SkyBoxPass&&) = delete;

    void Init(ID3D12Resource& skyBoxCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
              const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept;

    // Preconditions:
    // - Init() must be called first
    void Execute(const FrameCBuffer& frameCBuffer) const noexcept;

private:
    bool IsDataValid() const noexcept;

    std::unique_ptr<SkyBoxCmdListRecorder> mCommandListRecorder;
};

}

