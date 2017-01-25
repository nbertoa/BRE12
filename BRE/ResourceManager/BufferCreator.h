#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

namespace  BufferCreator {
	struct BufferParams {
		BufferParams() = default;
		explicit BufferParams(const void* data, const std::uint32_t elemCount, const std::size_t elemSize);
		~BufferParams() = default;
		BufferParams(const BufferParams&) = delete;
		const BufferParams& operator=(const BufferParams&) = delete;
		BufferParams(BufferParams&&) = delete;
		BufferParams& operator=(BufferParams&&) = delete;

		bool IsDataValid() const noexcept;

		const void* mData{ nullptr };
		std::uint32_t mElemCount{ 0U };
		std::size_t mElemSize{ 0UL };
	};

	struct VertexBufferData {
		VertexBufferData() = default;
		~VertexBufferData() = default;
		VertexBufferData(const VertexBufferData&) = default;

		const VertexBufferData& operator=(const VertexBufferData& instance) {
			if (this == &instance) {
				return *this;
			}

			mBuffer = instance.mBuffer;
			mBufferView = instance.mBufferView;
			mCount = instance.mCount;

			return *this;
		}

		VertexBufferData(VertexBufferData&&) = default;
		VertexBufferData& operator=(VertexBufferData&&) = default;

		bool IsDataValid() const noexcept;

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
		~IndexBufferData() = default;
		IndexBufferData(const IndexBufferData&) = default;

		const IndexBufferData& operator=(const IndexBufferData& instance) {
			if (this == &instance) {
				return *this;
			}

			mBuffer = instance.mBuffer;
			mBufferView = instance.mBufferView;
			mCount = instance.mCount;

			return *this;
		}

		IndexBufferData(IndexBufferData&&) = default;
		IndexBufferData& operator=(IndexBufferData&&) = default;

		bool IsDataValid() const noexcept;

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