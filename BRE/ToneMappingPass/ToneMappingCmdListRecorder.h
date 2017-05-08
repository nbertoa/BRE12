#pragma once

#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;

class ToneMappingCmdListRecorder {
public:
    ToneMappingCmdListRecorder() = default;
    ~ToneMappingCmdListRecorder() = default;
    ToneMappingCmdListRecorder(const ToneMappingCmdListRecorder&) = delete;
    const ToneMappingCmdListRecorder& operator=(const ToneMappingCmdListRecorder&) = delete;
    ToneMappingCmdListRecorder(ToneMappingCmdListRecorder&&) = default;
    ToneMappingCmdListRecorder& operator=(ToneMappingCmdListRecorder&&) = default;

    static void InitSharedPSOAndRootSignature() noexcept;

    // Preconditions:
    // - InitSharedPSOAndRootSignature() must be called first and once
    void Init(ID3D12Resource& inputColorBuffer,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    // Preconditions:
    // - Init() must be called first
    void RecordAndPushCommandLists() noexcept;

    bool IsDataValid() const noexcept;

private:
    void InitShaderResourceViews(ID3D12Resource& inputColorBuffer) noexcept;

    CommandListPerFrame mCommandListPerFrame;

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };
    D3D12_GPU_DESCRIPTOR_HANDLE mStartPixelShaderResourceView{ 0UL };
};