#pragma once

#include <memory>
#include <vector>
#include <wrl.h>

#include <EnvironmentLightPass\EnvironmentLightPass.h>
#include <CommandManager\CommandListPerFrame.h>

struct D3D12_CPU_DESCRIPTOR_HANDLE;

struct ID3D12Resource;

namespace BRE {
struct FrameCBuffer;

///
/// @brief Responsible to execute command list recorders relater with deferred shading lighting pass
///
class LightingPass {
public:
    LightingPass() = default;
    ~LightingPass() = default;
    LightingPass(const LightingPass&) = delete;
    const LightingPass& operator=(const LightingPass&) = delete;
    LightingPass(LightingPass&&) = delete;
    LightingPass& operator=(LightingPass&&) = delete;

    ///
    /// @brief Initializes the pass
    /// @param baseColorMetalMaskBuffer Geometry buffer that contains base color and metal mask.
    /// @param normalSmoothnessBuffer Geometry buffer that contains normals and smoothness factors.
    /// @param depthBuffer Depth buffer
    /// @param diffuseIrradianceCubeMap Diffuse irradiance environment cube map
    /// @param specularPreConvolvedCubeMap Specular pre convolved environment cube map
    /// @param renderTargetView Render target view
    ///
    void Init(ID3D12Resource& baseColorMetalMaskBuffer,
              ID3D12Resource& normalSmoothnessBuffer,
              ID3D12Resource& depthBuffer,
              ID3D12Resource& diffuseIrradianceCubeMap,
              ID3D12Resource& specularPreConvolvedCubeMap,
              const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Executes the pass
    /// @param frameCBuffer Constant buffer per frame, for current frame
    ///
    void Execute(const FrameCBuffer& frameCBuffer) noexcept;

private:
    ///
    /// @brief Validates internal data. Typically, used with assertions
    ///
    bool IsDataValid() const noexcept;

    ///
    /// @brief Executes begin task for the pass
    ///
    void ExecuteBeginTask() noexcept;

    ///
    /// @brief Executes final task for the pass
    ///
    void ExecuteFinalTask() noexcept;

    CommandListPerFrame mBeginCommandListPerFrame;
    CommandListPerFrame mFinalCommandListPerFrame;

    // Geometry buffers created by GeometryPass
    Microsoft::WRL::ComPtr<ID3D12Resource>* mGeometryBuffers;

    ID3D12Resource* mBaseColorMetalMaskBuffer{ nullptr };
    ID3D12Resource* mNormalSmoothnessBuffer{ nullptr };
    ID3D12Resource* mDepthBuffer{ nullptr };

    D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView{ 0UL };

    EnvironmentLightPass mEnvironmentLightPass;
};

}

