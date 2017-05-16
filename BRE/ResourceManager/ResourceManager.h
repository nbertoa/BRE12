#pragma once

#include <d3d12.h>
#include <mutex>
#include <tbb/concurrent_unordered_set.h>

#include <ResourceManager/UploadBuffer.h>

namespace BRE {
///
/// @brief Responsible to create textures, buffers and resources.
///
class ResourceManager {
public:
    ResourceManager() = delete;
    ~ResourceManager() = delete;
    ResourceManager(const ResourceManager&) = delete;
    const ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    ///
    /// @brief Releases all resources
    ///
    static void Clear() noexcept;

    ///
    /// @brief Loads texture from file
    /// @param textureFilename Texture filename. Must be not nullptr
    /// @param commandList Command list used to upload texture content to GPU.
    /// It must be executed after this function call to upload texture content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadBuffer Upload buffer to upload the texture content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadBuffer after it knows the copy has been executed.
    /// @param resourceName Resource name. If it is nullptr, then it will have the default name.
    ///
    static ID3D12Resource& LoadTextureFromFile(const char* textureFilename,
                                               ID3D12GraphicsCommandList& commandList,
                                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
                                               const wchar_t* resourceName) noexcept;

    ///
    /// @brief Creates default buffer
    /// @param sourceData Source data for the buffer
    /// @param sourceDataSize Source data size for the buffer
    /// @param commandList Command list used to upload buffer content to GPU.
    /// It must be executed after this function call to upload buffer content to GPU.
    /// It must be in recording state before calling this method.
    /// @param uploadBuffer Upload buffer to upload the buffer content.
    /// It has to be kept alive after the function call because
    /// the command list has not been executed yet that performs the actual copy.
    /// The caller can Release the uploadBuffer after it knows the copy has been executed.
    /// @param resourceName Resource name. If it is nullptr, then it will have the default name.
    ///
    static ID3D12Resource& CreateDefaultBuffer(const void* sourceData,
                                               const std::size_t sourceDataSize,
                                               ID3D12GraphicsCommandList& commandList,
                                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
                                               const wchar_t* resourceName) noexcept;

    ///
    /// @brief Creates committed resource
    /// @param heapProperties Heap properties
    /// @param heapFlags Heap flags
    /// @param resourceDescriptor Resource descriptor
    /// @param resourceStates Resource states
    /// @param clearValue Clear value
    /// @param resourceName Resource name. If it is nullptr, then it will have the default name.
    ///
    static ID3D12Resource& CreateCommittedResource(const D3D12_HEAP_PROPERTIES& heapProperties,
                                                   const D3D12_HEAP_FLAGS& heapFlags,
                                                   const D3D12_RESOURCE_DESC& resourceDescriptor,
                                                   const D3D12_RESOURCE_STATES& resourceStates,
                                                   const D3D12_CLEAR_VALUE* clearValue,
                                                   const wchar_t* resourceName) noexcept;

private:
    static tbb::concurrent_unordered_set<ID3D12Resource*> mResources;

    static std::mutex mMutex;
};

}

