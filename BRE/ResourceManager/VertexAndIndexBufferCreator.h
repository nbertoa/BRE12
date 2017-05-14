#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

namespace BRE {
///
/// @brief Responsible to create vertex and index buffer
///
class VertexAndIndexBufferCreator {
public:
    VertexAndIndexBufferCreator() = delete;
    ~VertexAndIndexBufferCreator() = delete;
    VertexAndIndexBufferCreator(const VertexAndIndexBufferCreator&) = delete;
    const VertexAndIndexBufferCreator& operator=(const VertexAndIndexBufferCreator&) = delete;
    VertexAndIndexBufferCreator(VertexAndIndexBufferCreator&&) = delete;
    VertexAndIndexBufferCreator& operator=(VertexAndIndexBufferCreator&&) = delete;

    ///
    /// @brief Input data for buffer creation
    ///
    struct BufferCreationData {
        BufferCreationData() = default;

        ///
        /// @brief BufferCreationData constructor
        /// @param data Buffer data. Must not be nullptr
        /// @param elementCount Number of elements in the buffer. Must be greater than zero.
        /// @param elementSize Size of the elements in the buffer. Must be greater than zero.
        ///
        explicit BufferCreationData(const void* data,
                                    const std::uint32_t elementCount,
                                    const std::size_t elementSize);
        ~BufferCreationData() = default;
        BufferCreationData(const BufferCreationData&) = delete;
        const BufferCreationData& operator=(const BufferCreationData&) = delete;
        BufferCreationData(BufferCreationData&&) = delete;
        BufferCreationData& operator=(BufferCreationData&&) = delete;

        ///
        /// @brief Checks if data is valid. Typically, used with assertions
        /// @return True if valid. Otherwise, false
        ///
        bool IsDataValid() const noexcept;

        const void* mData{ nullptr };
        std::uint32_t mElementCount{ 0U };
        std::size_t mElementSize{ 0UL };
    };

    ///
    /// @brief Vertex buffer data 
    ///
    struct VertexBufferData {
        VertexBufferData() = default;
        ~VertexBufferData() = default;
        VertexBufferData(const VertexBufferData&) = default;

        const VertexBufferData& operator=(const VertexBufferData& instance);

        VertexBufferData(VertexBufferData&&) = default;
        VertexBufferData& operator=(VertexBufferData&&) = default;

        ///
        /// @brief Checks if data is valid. Typically, used with assertions
        /// @return True if valid. Otherwise, false
        ///
        bool IsDataValid() const noexcept;

        ID3D12Resource* mBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW mBufferView{};
        std::uint32_t mElementCount{ 0U };
    };

    ///
    /// @brief Index buffer data
    ///
    struct IndexBufferData {
        IndexBufferData() = default;
        ~IndexBufferData() = default;
        IndexBufferData(const IndexBufferData&) = default;

        const IndexBufferData& operator=(const IndexBufferData& instance);

        IndexBufferData(IndexBufferData&&) = default;
        IndexBufferData& operator=(IndexBufferData&&) = default;

        ///
        /// @brief Checks if data is valid. Typically, used with assertions
        /// @return True if valid. Otherwise, false
        ///
        bool IsDataValid() const noexcept;

        ID3D12Resource* mBuffer{ nullptr };
        D3D12_INDEX_BUFFER_VIEW mBufferView{};
        std::uint32_t mElementCount{ 0U };
    };

    ///
    /// @brief Creates vertex buffer
    /// @param commandList Command list to create the buffer
    /// @param bufferCreationData Input data for buffer creation
    /// @param vertexBufferData Output vertex buffer data
    /// @param uploadBuffer Upload buffer
    ///
    static void CreateVertexBuffer(ID3D12GraphicsCommandList& commandList,
                                   const BufferCreationData& bufferCreationData,
                                   VertexBufferData& vertexBufferData,
                                   Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

    ///
    /// @brief Creates index buffer
    /// @param commandList Command list to create the buffer
    /// @param bufferCreationData Input data for buffer creation
    /// @param indexBufferData Output index buffer data
    /// @param uploadBuffer Upload buffer
    ///
    static void CreateIndexBuffer(ID3D12GraphicsCommandList& commandList,
                                  const BufferCreationData& bufferCreationData,
                                  IndexBufferData& indexBufferData,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;
};
}

