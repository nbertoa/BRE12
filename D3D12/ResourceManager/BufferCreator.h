#pragma once

#include <d3d12.h>
#include <tbb/concurrent_queue.h>
#include <wrl.h>

namespace  BufferCreator {
	struct BufferParams {
		BufferParams() = default;
		explicit BufferParams(const void* data, const std::uint32_t elemCount, const std::size_t elemSize);
		BufferParams(const BufferParams&) = delete;
		const BufferParams& operator=(const BufferParams&) = delete;

		bool ValidateData() const noexcept;

		const void* mData{ nullptr };
		std::uint32_t mElemCount{ 0U };
		std::size_t mElemSize{ 0UL };
	};

	struct VertexBufferData {
		VertexBufferData() = default;

		bool ValidateData() const noexcept;

		ID3D12Resource* mBuffer{ nullptr };
		D3D12_VERTEX_BUFFER_VIEW mBufferView{};
		std::uint32_t mCount{ 0U };
	};
	
	void CreateBuffer(
		ID3D12GraphicsCommandList& cmdList, 
		const BufferParams& bufferParams, 
		VertexBufferData& bufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;

	struct IndexBufferData {
		IndexBufferData() = default;

		bool ValidateData() const noexcept;

		ID3D12Resource* mBuffer{ nullptr };
		D3D12_INDEX_BUFFER_VIEW mBufferView{};
		std::uint32_t mCount{ 0U };
	};

	void CreateBuffer(
		ID3D12GraphicsCommandList& cmdList, 
		const BufferParams& bufferParams, 
		IndexBufferData& bufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept;
}