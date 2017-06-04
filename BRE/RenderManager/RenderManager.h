#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tbb/task.h>
#include <wrl.h>

#include <ApplicationSettings\ApplicationSettings.h>
#include <CommandManager\CommandListPerFrame.h>
#include <Camera/Camera.h>
#include <EnvironmentLightPass\EnvironmentLightPass.h>
#include <GeometryPass\GeometryPass.h>
#include <PostProcesspass\PostProcesspass.h>
#include <ReflectionPass\ReflectionPass.h>
#include <SkyBoxPass\SkyBoxPass.h>
#include <ShaderUtils\CBuffers.h>
#include <ToneMappingPass\ToneMappingPass.h>
#include <Timer/Timer.h>

namespace BRE {
class CommandListExecutor;
class Scene;

///
/// @brief Responsible to initialize passes (geometry, light, skybox, etc) based on a Scene.
///
/// Steps:
/// - Use RenderManager::Create() to create and spawn and instance. 
/// - When you want to terminate this task, you should call RenderManager::Terminate()
///
class RenderManager : public tbb::task {
public:
    ///
    /// @brief Creates a RenderManager
    ///
    /// This mtehod must be called once.
    ///
    /// @param scene Scene to create the RenderManager
    /// @return Render manager
    ///
    static RenderManager& Create(Scene& scene) noexcept;

    ~RenderManager() = default;
    RenderManager(const RenderManager&) = delete;
    const RenderManager& operator=(const RenderManager&) = delete;
    RenderManager(RenderManager&&) = delete;
    RenderManager& operator=(RenderManager&&) = delete;

    ///
    /// @brief Terminate render manager
    ///
    void Terminate() noexcept;

private:
    explicit RenderManager(Scene& scene);

    ///
    /// @brief Executes the tbb task.
    ///
    /// This method is called when tbb::task is spawned
    ///
    tbb::task* execute() final override;

    static RenderManager* sRenderManager;

    ///
    /// @brief Initialize passes
    /// @param scene Scene to initialize passes
    ///
    void InitPasses(Scene& scene) noexcept;

    ///
    /// @brief Creates frame buffers and render target views
    ///
    void CreateFrameBuffersAndRenderTargetViews() noexcept;

    ///
    /// @brief Creates depth stencil buffer and view
    ///
    void CreateDepthStencilBufferAndView() noexcept;

    ///
    /// @brief Creates intermediate color buffer and render target view
    /// @param initialState Initial state of the buffers
    /// @param resourceName Resource name
    /// @param buffer Output color buffer
    /// @param renderTargetView Output render target view
    ///
    void CreateIntermediateColorBufferAndRenderTargetView(const D3D12_RESOURCE_STATES initialState,
                                                          const wchar_t* resourceName,
                                                          ID3D12Resource* &buffer,
                                                          D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept;

    ///
    /// @brief Get current frame buffer
    /// @return Frame buffer
    ///
    ID3D12Resource* GetCurrentFrameBuffer() const noexcept
    {
        BRE_ASSERT(mSwapChain != nullptr);
        return mFrameBuffers[mSwapChain->GetCurrentBackBufferIndex()];
    }

    ///
    /// @brief Get current frame buffer CPU descriptor
    /// @return CPU descriptor
    ///
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentFrameBufferCpuDesc() const noexcept
    {
        return mFrameBufferRenderTargetViews[mSwapChain->GetCurrentBackBufferIndex()];
    }

    ///
    /// @brief Get depth stencil CPU descriptor
    /// @return CPU descriptor
    ///
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilCpuDesc() const noexcept
    {
        return mDepthBufferRenderTargetView;
    }

    ///
    /// @brief Records pre pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPrePassCommandLists() noexcept;

    ///
    /// @brief Records post pass command lists and pushes them to 
    /// the CommandListExecutor.
    /// @return The number of recorded command lists
    ///
    std::uint32_t RecordAndPushPostPassCommandLists() noexcept;

    ///
    /// @brief Flushes command queue
    ///
    void FlushCommandQueue() noexcept;

    ///
    /// @brief Presents current frame and continue with the next frame.
    ///
    void PresentCurrentFrameAndBeginNextFrame() noexcept;

    Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain{ nullptr };

    // Fences data for synchronization purposes.
    ID3D12Fence* mFence{ nullptr };
    std::uint32_t mCurrentQueuedFrameIndex{ 0U };
    std::uint64_t mFenceValueByQueuedFrameIndex[ApplicationSettings::sQueuedFrameCount]{ 0UL };
    std::uint64_t mCurrentFenceValue{ 0UL };

    // Passes
    GeometryPass mGeometryPass;
    EnvironmentLightPass mEnvironmentLightPass;
    ReflectionPass mReflectionPass;
    SkyBoxPass mSkyBoxPass;
    ToneMappingPass mToneMappingPass;
    PostProcessPass mPostProcessPass;

    CommandListPerFrame mPrePassCommandListPerFrame;
    CommandListPerFrame mPostPassCommandListPerFrame;

    ID3D12Resource* mFrameBuffers[ApplicationSettings::sSwapChainBufferCount]{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mFrameBufferRenderTargetViews[ApplicationSettings::sSwapChainBufferCount]{ 0UL };

    ID3D12Resource* mDepthBuffer{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mDepthBufferRenderTargetView{ 0UL };

    // Buffers used for intermediate computations.
    // They are used as render targets (light pass) or pixel shader resources (post processing passes)
    ID3D12Resource* mIntermediateColorBuffer1{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mIntermediateColorBuffer1RenderTargetView;
    ID3D12Resource* mIntermediateColorBuffer2{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE mIntermediateColorBuffer2RenderTargetView;

    // We cache it here, as is is used by most passes.
    FrameCBuffer mFrameCBuffer;

    Camera mCamera;
    Timer mTimer;

    // When it is true, master render thread is destroyed.
    bool mTerminate{ false };
};
}