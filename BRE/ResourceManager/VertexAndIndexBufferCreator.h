#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

class VertexAndIndexBufferCreator {
public:
    VertexAndIndexBufferCreator() = delete;
    ~VertexAndIndexBufferCreator() = delete;
    VertexAndIndexBufferCreator(const VertexAndIndexBufferCreator&) = delete;
    const VertexAndIndexBufferCreator& operator=(const VertexAndIndexBufferCreator&) = delete;
    VertexAndIndexBufferCreator(VertexAndIndexBufferCreator&&) = delete;
    VertexAndIndexBufferCreator& operator=(VertexAndIndexBufferCreator&&) = delete;

    struct BufferCreationData {
        BufferCreationData() = default;
        explicit BufferCreationData(const void* data,
                                    const std::uint32_t elementCount,
                                    const std::size_t elementSize);
        ~BufferCreationData() = default;
        BufferCreationData(const BufferCreationData&) = delete;
        const BufferCreationData& operator=(const BufferCreationData&) = delete;
        BufferCreationData(BufferCreationData&&) = delete;
        BufferCreationData& operator=(BufferCreationData&&) = delete;

        bool IsDataValid() const noexcept;

        const void* mData{ nullptr };
        std::uint32_t mElementCount{ 0U };
        std::size_t mElementSize{ 0UL };
    };

    struct VertexBufferData {
        VertexBufferData() = default;
        ~VertexBufferData() = default;
        VertexBufferData(const VertexBufferData&) = default;

        const VertexBufferData& operator=(const VertexBufferData& instance);

        VertexBufferData(VertexBufferData&&) = default;
        VertexBufferData& operator=(VertexBufferData&&) = default;

        bool IsDataValid() const noexcept;

        ID3D12Resource* mBuffer{ nullptr };
        D3D12_VERTEX_BUFFER_VIEW mBufferView{};
        std::uint32_t mElementCount{ 0U };
    };

    struct IndexBufferData {
        IndexBufferData() = default;
        ~IndexBufferData() = default;
        IndexBufferData(const IndexBufferData&) = default;

        const IndexBufferData& operator=(const IndexBufferData& instance);

        IndexBufferData(IndexBufferData&&) = default;
        IndexBufferData& operator=(IndexBufferData&&) = default;

        bool IsDataValid() const noexcept;

        ID3D12Resource* mBuffer{ nullptr };
        D3D12_INDEX_BUFFER_VIEW mBufferView{};
        std::uint32_t mElementCount{ 0U };
    };

    static void CreateVertexBuffer(ID3D12GraphicsCommandList& commandList,
                                   const BufferCreationData& bufferCreationData,
                                   VertexBufferData& vertexBufferData,
                                   Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

    static void CreateIndexBuffer(ID3D12GraphicsCommandList& commandList,
                                  const BufferCreationData& bufferCreationData,
                                  IndexBufferData& indexBufferData,
                                  Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;
};