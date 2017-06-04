#include "SkyBoxPass.h"

#include <d3d12.h>

#include <CommandListExecutor/CommandListExecutor.h>
#include <CommandManager\CommandAllocatorManager.h>
#include <CommandManager\CommandListManager.h>
#include <DXUtils/d3dx12.h>
#include <ModelManager\Mesh.h>
#include <ModelManager\Model.h>
#include <ModelManager\ModelManager.h>
#include <ResourceStateManager\ResourceStateManager.h>
#include <SkyBoxPass\SkyBoxCommandListRecorder.h>
#include <ShaderUtils\CBuffers.h>
#include <Utils\DebugUtils.h>

using namespace DirectX;

namespace BRE {
namespace {
///
/// @brief Create command objects
/// @param commandAllocator Output command allocator
/// @param commandList Output command list
///
void
CreateCommandObjects(ID3D12CommandAllocator* &commandAllocator,
                     ID3D12GraphicsCommandList* &commandList) noexcept
{
    // Create command allocators and command list
    commandAllocator = &CommandAllocatorManager::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    commandList = &CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, *commandAllocator);
    commandList->Close();
}

///
/// @brief Creates and gets sky box sphere model
/// @param commandAllocator Command allocator to use with the command list
/// @param commandList Command list to execute the sphere model creation
/// @return Sphere model
///
Model&
CreateAndGetSkyBoxSphereModel(ID3D12CommandAllocator& commandAllocator,
                              ID3D12GraphicsCommandList& commandList)
{
    BRE_CHECK_HR(commandList.Reset(&commandAllocator, nullptr));

    ID3D12Resource* uploadVertexBuffer;
    ID3D12Resource* uploadIndexBuffer;
    Model* model = &ModelManager::CreateSphere(3000U, 
                                               50U, 
                                               50U, 
                                               commandList, 
                                               uploadVertexBuffer, 
                                               uploadIndexBuffer);

    commandList.Close();
    CommandListExecutor::Get().ExecuteCommandListAndWaitForCompletion(commandList);

    return *model;
}
}

void
SkyBoxPass::Init(ID3D12Resource& skyBoxCubeMap,
                 ID3D12Resource& depthBuffer,
                 const D3D12_CPU_DESCRIPTOR_HANDLE& renderTargetView,
                 const D3D12_CPU_DESCRIPTOR_HANDLE& depthBufferView) noexcept
{
    BRE_ASSERT(IsDataValid() == false);

    mDepthBuffer = &depthBuffer;

    ID3D12CommandAllocator* commandAllocator;
    ID3D12GraphicsCommandList* commandList;
    CreateCommandObjects(commandAllocator, commandList);

    Model& model = CreateAndGetSkyBoxSphereModel(*commandAllocator, *commandList);
    const std::vector<Mesh>& meshes(model.GetMeshes());
    BRE_ASSERT(meshes.size() == 1UL);

    // Build world matrix
    const Mesh& mesh{ meshes[0] };
    XMFLOAT4X4 worldMatrix;
    MathUtils::ComputeMatrix(worldMatrix,
                             0.0f,
                             0.0f,
                             0.0f,
                             1.0f,
                             1.0f,
                             1.0f,
                             0.0f,
                             0.0f,
                             0.0f);

    SkyBoxCommandListRecorder::InitSharedPSOAndRootSignature();

    mCommandListRecorder.reset(new SkyBoxCommandListRecorder());
    mCommandListRecorder->Init(mesh.GetVertexBufferData(),
                               mesh.GetIndexBufferData(),
                               worldMatrix,
                               skyBoxCubeMap,
                               renderTargetView,
                               depthBufferView);

    BRE_ASSERT(IsDataValid());
}

std::uint32_t
SkyBoxPass::Execute(const FrameCBuffer& frameCBuffer) noexcept
{
    BRE_ASSERT(IsDataValid());

    std::uint32_t commandListCount = 0U;

    commandListCount += RecordAndPushPrePassCommandLists();
    commandListCount += mCommandListRecorder->RecordAndPushCommandLists(frameCBuffer);

    return commandListCount;
}

bool
SkyBoxPass::IsDataValid() const noexcept
{
    const bool b =
        mCommandListRecorder.get() != nullptr &&
        mDepthBuffer != nullptr;

    return b;
}

std::uint32_t
SkyBoxPass::RecordAndPushPrePassCommandLists() noexcept
{
    BRE_ASSERT(IsDataValid());

    CD3DX12_RESOURCE_BARRIER barriers[1U];
    std::uint32_t barrierCount = 0UL;
    if (ResourceStateManager::GetResourceState(*mDepthBuffer) != D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        barriers[barrierCount] = ResourceStateManager::ChangeResourceStateAndGetBarrier(*mDepthBuffer,
                                                                                        D3D12_RESOURCE_STATE_DEPTH_WRITE);
        ++barrierCount;
    }

    if (barrierCount > 0UL) {
        ID3D12GraphicsCommandList& commandList = mPrePassCommandListPerFrame.ResetCommandListWithNextCommandAllocator(nullptr);
        commandList.ResourceBarrier(barrierCount, barriers);
        BRE_CHECK_HR(commandList.Close());
        CommandListExecutor::Get().PushCommandList(commandList);

        return 1U;
    }

    return 0U;
}
}