#pragma once

#include <memory>
#include <wrl.h>

#include <EnvironmentLightPass\AmbientOcclusionCmdListRecorder.h>
#include <EnvironmentLightPass\BlurCmdListRecorder.h>
#include <EnvironmentLightPass\EnvironmentLightCmdListRecorder.h>
#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

namespace BRE {
// Pass responsible to apply ambient lighting and ambient occlusion
class EnvironmentLightPass {
public:
    EnvironmentLightPass() = default;
    ~EnvironmentLightPass() = default;
    EnvironmentLightPass(const EnvironmentLightPass&) = delete;
    const EnvironmentLightPass& operator=(const EnvironmentLightPass&) = delete;
    EnvironmentLightPass(EnvironmentLightPass&&) = delete;
    EnvironmentLightPass& operator=(EnvironmentLightPass&&) = delete;

    void Init(ID3D12Resource& baseColorMetalMaskBuffer,
              ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& depthBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    // Preconditions:
    // - Init() must be called first
    void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
    bool ValidateData() const noexcept;

    void ExecuteBeginTask() noexcept;
    void ExecuteMiddleTask() noexcept;
    void ExecuteFinalTask() noexcept;

    CommandListPerFrame mBeginCommandListPerFrame;
    CommandListPerFrame mMiddleCommandListPerFrame;
    CommandListPerFrame mFinalCommandListPerFrame;

    Microsoft::WRL::ComPtr<ID3D12Resource> mAmbientAccessibilityBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE mAmbientAccessibilityBufferRenderTargetView{ 0UL };

    Microsoft::WRL::ComPtr<ID3D12Resource> mBlurBuffer;

    std::unique_ptr<AmbientOcclusionCmdListRecorder> mAmbientOcclusionRecorder;
    std::unique_ptr<BlurCmdListRecorder> mBlurRecorder;
    std::unique_ptr<EnvironmentLightCmdListRecorder> mEnvironmentLightRecorder;
};

}

