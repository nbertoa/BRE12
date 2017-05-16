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
    /// @param commandList Command list used to create the texture
    /// @param uploadBuffer Upload buffer to create the texture
    /// @param resourceName Resource name. If it is nullptr, then it will have the default name.
    ///
    static ID3D12Resource& LoadTextureFromFile(const char* textureFilename,
                                               ID3D12GraphicsCommandList& commandList,
                                               Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
                                               const wchar_t* resourceName) noexcept;

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.

    ///
    /// @brief Creates default buffer
    /// @param commandList Command list used to create the texture
    /// @param sourceData Source data for the buffer
    /// @param sourceDataSize Source data size for the buffer
    /// @param uploadBuffer Upload buffer to create the texture
    /// @param resourceName Resource name. If it is nullptr, then it will have the default name.
    ///
    static ID3D12Resource& CreateDefaultBuffer(ID3D12GraphicsCommandList& commandList,
                                               const void* sourceData,
                                               const std::size_t sourceDataSize,
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

