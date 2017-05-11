#include "RenderManager.h"

#include <tbb/parallel_for.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager/CommandQueueManager.h>
#include <CommandManager/FenceManager.h>
#include <DescriptorManager\DepthStencilDescriptorManager.h>
#include <DescriptorManager\RenderTargetDescriptorManager.h>
#include <DirectXManager\DirectXManager.h>
#include <DXUtils/d3dx12.h>
#include <Input/Keyboard.h>
#include <Input/Mouse.h>
#include <ResourceManager\ResourceManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <Scene/Scene.h>
#include <SettingsManager\SettingsManager.h>

using namespace DirectX;

namespace BRE {
namespace {
void UpdateCameraAndFrameCBuffer(const float elapsedFrameTime,
                                 Camera& camera,
                                 FrameCBuffer& frameCBuffer) noexcept
{
    static float elapsedFrameTimeAccumulator = 0.0f;
    elapsedFrameTimeAccumulator += elapsedFrameTime;

    while (elapsedFrameTimeAccumulator >= SettingsManager::sSecondsPerFrame) {
        static const float translationAcceleration = 5.0f; // rate of acceleration in units/sec
        const float translationDelta = translationAcceleration;

        static const float rotationAcceleration = 10.0f;
        const float rotationDelta = rotationAcceleration;

        static std::int32_t lastXY[]{ 0UL, 0UL };
        static const float sCameraOffset{ 7.5f };
        static const float sCameraMultiplier{ 10.0f };

        camera.UpdateViewMatrix();

        frameCBuffer.mEyeWorldPosition = camera.GetPosition4f();

        MathUtils::StoreTransposeMatrix(camera.GetViewMatrix(), frameCBuffer.mViewMatrix);
        MathUtils::StoreInverseTransposeMatrix(camera.GetViewMatrix(), frameCBuffer.mInverseViewMatrix);

        MathUtils::StoreTransposeMatrix(camera.GetProjectionMatrix(), frameCBuffer.mProjectionMatrix);
        MathUtils::StoreInverseTransposeMatrix(camera.GetProjectionMatrix(), frameCBuffer.mInverseProjectionMatrix);

        // Update camera based on keyboard
        const float offset = translationDelta * (Keyboard::Get().IsKeyDown(DIK_LSHIFT) ? sCameraMultiplier : 1.0f);
        if (Keyboard::Get().IsKeyDown(DIK_W)) {
            camera.Walk(offset);
        }
        if (Keyboard::Get().IsKeyDown(DIK_S)) {
            camera.Walk(-offset);
        }
        if (Keyboard::Get().IsKeyDown(DIK_A)) {
            camera.Strafe(-offset);
        }
        if (Keyboard::Get().IsKeyDown(DIK_D)) {
            camera.Strafe(offset);
        }

        // Update camera based on mouse
        const std::int32_t x{ Mouse::Get().GetX() };
        const std::int32_t y{ Mouse::Get().GetY() };
        if (Mouse::Get().IsButtonDown(Mouse::MouseButtonsLeft)) {
            const float dx = static_cast<float>(x - lastXY[0]) / SettingsManager::sWindowWidth;
            const float dy = static_cast<float>(y - lastXY[1]) / SettingsManager::sWindowHeight;

            camera.Pitch(dy * rotationDelta);
            camera.RotateY(dx * rotationDelta);
        }

        lastXY[0] = x;
        lastXY[1] = y;

        elapsedFrameTimeAccumulator -= SettingsManager::sSecondsPerFrame;
    }
}

void CreateSwapChain(const HWND windowHandle,
                     const DXGI_FORMAT frameBufferFormat,
                     Microsoft::WRL::ComPtr<IDXGISwapChain3>& swapChain) noexcept
{

    IDXGISwapChain1* baseSwapChain{ nullptr };

    DXGI_SWAP_CHAIN_DESC1 swapChainDescriptor = {};
    swapChainDescriptor.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDescriptor.BufferCount = SettingsManager::sSwapChainBufferCount;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
#ifdef V_SYNC
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
#else 
    swapChainDescriptor.Flags = 0U;
#endif
    swapChainDescriptor.Format = frameBufferFormat;
    swapChainDescriptor.SampleDesc.Count = 1U;
    swapChainDescriptor.Scaling = DXGI_SCALING_NONE;
    swapChainDescriptor.Stereo = false;
    swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    BRE_CHECK_HR(DirectXManager::GetIDXGIFactory().CreateSwapChainForHwnd(&CommandListExecutor::Get().GetCommandQueue(),
                                                                          windowHandle,
                                                                          &swapChainDescriptor,
                                                                          nullptr,
                                                                          nullptr,
                                                                          &baseSwapChain));
    BRE_CHECK_HR(baseSwapChain->QueryInterface(IID_PPV_ARGS(swapChain.GetAddressOf())));

    BRE_CHECK_HR(swapChain->ResizeBuffers(SettingsManager::sSwapChainBufferCount,
                                          SettingsManager::sWindowWidth,
                                          SettingsManager::sWindowHeight,
                                          frameBufferFormat,
                                          swapChainDescriptor.Flags));

    // Make window association
    BRE_CHECK_HR(DirectXManager::GetIDXGIFactory().MakeWindowAssociation(windowHandle,
                                                                         DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN));

#ifdef V_SYNC
    BRE_CHECK_HR(swapChain3->SetMaximumFrameLatency(SettingsManager::sQueuedFrameCount));
#endif
}
}

using namespace DirectX;

RenderManager* RenderManager::sRenderManager{ nullptr };

RenderManager&
RenderManager::Create(Scene& scene) noexcept
{
    BRE_ASSERT(sRenderManager == nullptr);

    tbb::empty_task* parent{ new (tbb::task::allocate_root()) tbb::empty_task };
    // Reference count is 2: 1 parent task + 1 master render task
    parent->set_ref_count(2);

    sRenderManager = new (parent->allocate_child()) RenderManager(scene);
    return *sRenderManager;
}

RenderManager::RenderManager(Scene& scene)
    : mGeometryPass(scene.GetGeometryPassCommandListRecorders())
{
    mFence = &FenceManager::CreateFence(0U, D3D12_FENCE_FLAG_NONE);

    CreateFrameBuffersAndRenderTargetViews();

    CreateDepthStencilBufferAndView();

    CreateIntermediateColorBufferAndRenderTargetView(D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                     L"Intermediate Color Buffer 1",
                                                     mIntermediateColorBuffer1,
                                                     mIntermediateColorBuffer1RenderTargetView);

    CreateIntermediateColorBufferAndRenderTargetView(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                     L"Intermediate Color Buffer 2",
                                                     mIntermediateColorBuffer2,
                                                     mIntermediateColorBuffer2RenderTargetView);

    mCamera.SetFrustum(SettingsManager::sVerticalFieldOfView,
                       SettingsManager::AspectRatio(),
                       SettingsManager::sNearPlaneZ,
                       SettingsManager::sFarPlaneZ);

    InitPasses(scene);

    // Spawns master render task
    parent()->spawn(*this);
}

void
RenderManager::InitPasses(Scene& scene) noexcept
{
    // Generate recorders for all the passes
    mGeometryPass.Init(DepthStencilCpuDesc());

    ID3D12Resource* skyBoxCubeMap = scene.GetSkyBoxCubeMap();
    ID3D12Resource* diffuseIrradianceCubeMap = scene.GetDiffuseIrradianceCubeMap();
    ID3D12Resource* specularPreConvolvedCubeMap = scene.GetSpecularPreConvolvedCubeMap();
    BRE_ASSERT(skyBoxCubeMap != nullptr);
    BRE_ASSERT(diffuseIrradianceCubeMap != nullptr);
    BRE_ASSERT(specularPreConvolvedCubeMap != nullptr);

    mLightingPass.Init(*mGeometryPass.GetGeometryBuffers()[GeometryPass::BASECOLOR_METALMASK].Get(),
                       *mGeometryPass.GetGeometryBuffers()[GeometryPass::NORMAL_SMOOTHNESS].Get(),
                       *mDepthBuffer,
                       *diffuseIrradianceCubeMap,
                       *specularPreConvolvedCubeMap,
                       mIntermediateColorBuffer1RenderTargetView);

    mSkyBoxPass.Init(*skyBoxCubeMap,
                     mIntermediateColorBuffer1RenderTargetView,
                     DepthStencilCpuDesc());

    mToneMappingPass.Init(*mIntermediateColorBuffer1.Get(),
                          *mIntermediateColorBuffer2.Get(),
                          mIntermediateColorBuffer2RenderTargetView);

    mPostProcessPass.Init(*mIntermediateColorBuffer2.Get());

    // Initialize fence values for all frames to the same number.
    const std::uint64_t count{ _countof(mFenceValueByQueuedFrameIndex) };
    for (std::uint64_t i = 0UL; i < count; ++i) {
        mFenceValueByQueuedFrameIndex[i] = mCurrentFenceValue;
    }
}

void
RenderManager::Terminate() noexcept
{
    mTerminate = true;
    parent()->wait_for_all();
}

tbb::task*
RenderManager::execute()
{
    while (!mTerminate) {
        mTimer.Tick();
        UpdateCameraAndFrameCBuffer(mTimer.GetDeltaTimeInSeconds(), mCamera, mFrameCBuffer);

        BRE_ASSERT(CommandListExecutor::Get().AreTherePendingCommandListsToExecute());

        mGeometryPass.Execute(mFrameCBuffer);
        mLightingPass.Execute(mFrameCBuffer);
        mSkyBoxPass.Execute(mFrameCBuffer);
        mToneMappingPass.Execute();
        mPostProcessPass.Execute(*CurrentFrameBuffer(), CurrentFrameBufferCpuDesc());
        ExecuteFinalPass();

        SignalFenceAndPresent();
    }

    // If we need to terminate, then we terminates command list processor
    // and waits until all GPU command lists are properly executed.
    CommandListExecutor::Get().Terminate();
    FlushCommandQueue();

    return nullptr;
}

void
RenderManager::ExecuteFinalPass()
{
    ID3D12GraphicsCommandList& commandList = mFinalCommandListPerFrame.ResetWithNextCommandAllocator(nullptr);

    CD3DX12_RESOURCE_BARRIER barriers[]{
        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mGeometryPass.GetGeometryBuffers()[GeometryPass::NORMAL_SMOOTHNESS].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET),

        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mGeometryPass.GetGeometryBuffers()[GeometryPass::BASECOLOR_METALMASK].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET),

        ResourceStateManager::ChangeResourceStateAndGetBarrier(*CurrentFrameBuffer(),
        D3D12_RESOURCE_STATE_PRESENT),

        ResourceStateManager::ChangeResourceStateAndGetBarrier(*mIntermediateColorBuffer1.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET),
    };
    const std::size_t barrierCount = _countof(barriers);
    BRE_ASSERT(barrierCount == GeometryPass::BUFFERS_COUNT + 2UL);
    commandList.ResourceBarrier(_countof(barriers), barriers);

    BRE_CHECK_HR(commandList.Close());
    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);
}

void
RenderManager::CreateFrameBuffersAndRenderTargetViews() noexcept
{
    // Setup render target view
    D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor = {};
    rtvDescriptor.Format = SettingsManager::sFrameBufferRTFormat;
    rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // Create swap chain and frame buffers
    BRE_ASSERT(mSwapChain == nullptr);
    CreateSwapChain(DirectXManager::GetWindowHandle(),
                    SettingsManager::sFrameBufferFormat,
                    mSwapChain);

    // Create frame buffer render target views
    const std::size_t rtvDescriptorSize{ DirectXManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) };
    for (std::uint32_t i = 0U; i < SettingsManager::sSwapChainBufferCount; ++i) {
        BRE_CHECK_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(mFrameBuffers[i].GetAddressOf())));

        RenderTargetDescriptorManager::CreateRenderTargetView(*mFrameBuffers[i].Get(),
                                                              rtvDescriptor,
                                                              &mFrameBufferRenderTargetViews[i]);

        ResourceStateManager::AddResource(*mFrameBuffers[i].Get(), D3D12_RESOURCE_STATE_PRESENT);
    }
}

void
RenderManager::CreateDepthStencilBufferAndView() noexcept
{
    // Create the depth/stencil buffer and view.
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0U;
    depthStencilDesc.Width = SettingsManager::sWindowWidth;
    depthStencilDesc.Height = SettingsManager::sWindowHeight;
    depthStencilDesc.DepthOrArraySize = 1U;
    depthStencilDesc.MipLevels = 1U;
    depthStencilDesc.Format = SettingsManager::sDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1U;
    depthStencilDesc.SampleDesc.Quality = 0U;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = SettingsManager::sDepthStencilViewFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0U;

    CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
    mDepthBuffer = &ResourceManager::CreateCommittedResource(heapProps,
                                                             D3D12_HEAP_FLAG_NONE,
                                                             depthStencilDesc,
                                                             D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                             &clearValue,
                                                             L"Depth Stencil Buffer");

    // Create descriptor to mip level 0 of entire resource using the format of the resource.
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = SettingsManager::sDepthStencilViewFormat;
    depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    DepthStencilDescriptorManager::CreateDepthStencilView(*mDepthBuffer, depthStencilViewDesc, &mDepthBufferRenderTargetView);
}

void
RenderManager::CreateIntermediateColorBufferAndRenderTargetView(const D3D12_RESOURCE_STATES initialState,
                                                                const wchar_t* resourceName,
                                                                Microsoft::WRL::ComPtr<ID3D12Resource>& buffer,
                                                                D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView) noexcept
{
    BRE_ASSERT(resourceName != nullptr);

    // Fill resource description
    D3D12_RESOURCE_DESC resourceDescriptor = {};
    resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescriptor.Alignment = 0U;
    resourceDescriptor.Width = SettingsManager::sWindowWidth;
    resourceDescriptor.Height = SettingsManager::sWindowHeight;
    resourceDescriptor.DepthOrArraySize = 1U;
    resourceDescriptor.MipLevels = 0U;
    resourceDescriptor.SampleDesc.Count = 1U;
    resourceDescriptor.SampleDesc.Quality = 0U;
    resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resourceDescriptor.Format = SettingsManager::sColorBufferFormat;

    // Create buffer and render target view
    D3D12_RENDER_TARGET_VIEW_DESC rtvDescriptor{};
    rtvDescriptor.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDescriptor.Format = resourceDescriptor.Format;
    CD3DX12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE_DEFAULT };
    D3D12_CLEAR_VALUE clearValue = { resourceDescriptor.Format, 0.0f, 0.0f, 0.0f, 1.0f };
    ID3D12Resource* resource = &ResourceManager::CreateCommittedResource(heapProperties,
                                                                         D3D12_HEAP_FLAG_NONE,
                                                                         resourceDescriptor,
                                                                         initialState,
                                                                         &clearValue,
                                                                         resourceName);
    buffer = Microsoft::WRL::ComPtr<ID3D12Resource>(resource);

    RenderTargetDescriptorManager::CreateRenderTargetView(*buffer.Get(),
                                                          rtvDescriptor,
                                                          &renderTargetView);
}

void
RenderManager::FlushCommandQueue() noexcept
{
    ++mCurrentFenceValue;
    CommandListExecutor::Get().SignalFenceAndWaitForCompletion(*mFence,
                                                               mCurrentFenceValue,
                                                               mCurrentFenceValue);
}

void
RenderManager::SignalFenceAndPresent() noexcept
{
    BRE_ASSERT(mSwapChain != nullptr);

#ifdef V_SYNC
    static const HANDLE frameLatencyWaitableObj(mSwapChain->GetFrameLatencyWaitableObject());
    WaitForSingleObjectEx(frameLatencyWaitableObj, INFINITE, true);
    BRE_CHECK_HR(mSwapChain->Present(1U, 0U));
#else
    BRE_CHECK_HR(mSwapChain->Present(0U, 0U));
#endif

    // Add an instruction to the command queue to set a new fence point.  Because we 
    // are on the GPU time line, the new fence point won't be set until the GPU finishes
    // processing all the commands prior to this Signal().
    mFenceValueByQueuedFrameIndex[mCurrentQueuedFrameIndex] = ++mCurrentFenceValue;
    mCurrentQueuedFrameIndex = (mCurrentQueuedFrameIndex + 1U) % SettingsManager::sQueuedFrameCount;
    const std::uint64_t oldestFence{ mFenceValueByQueuedFrameIndex[mCurrentQueuedFrameIndex] };

    // If we executed command lists for all queued frames, then we need to wait
    // at least 1 of them to be completed, before continue recording command lists. 
    CommandListExecutor::Get().SignalFenceAndWaitForCompletion(*mFence,
                                                               mCurrentFenceValue,
                                                               oldestFence);
}

}

