#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

namespace  BufferCreator {
	struct BufferData {
		BufferData() = default;
		explicit BufferData(const void* data, const std::uint32_t elemCount, const std::size_t elemSize);
		~BufferData() = default;
		BufferData(const BufferData&) = delete;
		const BufferData& operator=(const BufferData&) = delete;
		BufferData(BufferData&&) = delete;
		BufferData& operator=(BufferData&&) = delete;

		bool IsDataValid() const noexcept;

		const void* mData{ nullptr };
		std::uint32_t mElemCount{ 0U };
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
	
	void CreateVertexBuffer(
		ID3D12GraphicsCommandList& commandList, 
		const BufferData& bufferData,
		VertexBufferData& vertexBufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

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

	void CreateIndexBuffer(
		ID3D12GraphicsCommandList& commandList, 
		const BufferData& bufferData, 
		IndexBufferData& indexBufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;
}