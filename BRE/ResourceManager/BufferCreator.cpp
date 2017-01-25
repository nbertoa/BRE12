#include "BufferCreator.h"

#include <ResourceManager/ResourceManager.h>
#include <Utils/DebugUtils.h>

namespace {
	void CreateVertexBuffer(
		ID3D12GraphicsCommandList& cmdList, 
		const BufferCreator::BufferParams& bufferParams, 
		BufferCreator::VertexBufferData& bufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) {
		ASSERT(bufferParams.IsDataValid());

		// Create buffer
		const std::uint32_t byteSize{ bufferParams.mElemCount * static_cast<std::uint32_t>(bufferParams.mElemSize) };
		ResourceManager::Get().CreateDefaultBuffer(cmdList, bufferParams.mData, byteSize, bufferData.mBuffer, uploadBuffer);
		bufferData.mCount = bufferParams.mElemCount;

		// Fill view
		bufferData.mBufferView.BufferLocation = bufferData.mBuffer->GetGPUVirtualAddress();
		bufferData.mBufferView.SizeInBytes = byteSize;
		bufferData.mBufferView.StrideInBytes = static_cast<std::uint32_t>(bufferParams.mElemSize);

		ASSERT(bufferData.IsDataValid());
	}

	void CreateIndexBuffer(
		ID3D12GraphicsCommandList& cmdList, 
		const BufferCreator::BufferParams& bufferParams, 
		BufferCreator::IndexBufferData& bufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) {
		ASSERT(bufferParams.IsDataValid());

		// Create buffer
		const std::uint32_t elemSize{ static_cast<std::uint32_t>(bufferParams.mElemSize) };
		const std::uint32_t byteSize{ bufferParams.mElemCount * elemSize };
		ResourceManager::Get().CreateDefaultBuffer(cmdList, bufferParams.mData, byteSize, bufferData.mBuffer, uploadBuffer);
		bufferData.mCount = bufferParams.mElemCount;

		// Set index format
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
		switch (elemSize)
		{
		case 1U:
			format = DXGI_FORMAT_R8_UINT;
			break;
		case 2U:
			format = DXGI_FORMAT_R16_UINT;
			break;
		case 4U:
			format = DXGI_FORMAT_R32_UINT;
			break;
		default:
			break;
		}
		ASSERT(format != DXGI_FORMAT_UNKNOWN);

		// Fill view
		bufferData.mBufferView.BufferLocation = bufferData.mBuffer->GetGPUVirtualAddress();
		bufferData.mBufferView.Format = format; 
		bufferData.mBufferView.SizeInBytes = byteSize;

		ASSERT(bufferData.IsDataValid());
	}
}

namespace BufferCreator {
	BufferParams::BufferParams(const void* data, const std::uint32_t elemCount, const std::size_t elemSize)
		: mData(data)
		, mElemCount(elemCount)
		, mElemSize(elemSize)
	{
	}

	bool BufferParams::IsDataValid() const noexcept {
		return mData != nullptr && mElemCount != 0U && mElemSize != 0UL;
	}

	bool VertexBufferData::IsDataValid() const noexcept {
		D3D12_VERTEX_BUFFER_VIEW invalidView{};

		return 
			mBuffer != nullptr && 
			mCount != 0U && 
			mBufferView.BufferLocation != invalidView.BufferLocation &&
			mBufferView.SizeInBytes != invalidView.SizeInBytes &&
			mBufferView.StrideInBytes != invalidView.StrideInBytes;
	}

	void CreateBuffer(
		ID3D12GraphicsCommandList& cmdList, 
		const BufferParams& bufferParams, 
		VertexBufferData& bufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept {
		ASSERT(bufferParams.IsDataValid());
		CreateVertexBuffer(cmdList, bufferParams, bufferData, uploadBuffer);
		ASSERT(bufferData.IsDataValid());
	}

	bool IndexBufferData::IsDataValid() const noexcept {
		D3D12_INDEX_BUFFER_VIEW invalidView{};

		return
			mBuffer != nullptr && 
			mCount != 0U && 
			mBufferView.BufferLocation != invalidView.BufferLocation &&
			mBufferView.Format != invalidView.Format &&
			mBufferView.SizeInBytes != invalidView.SizeInBytes;
	}

	void CreateBuffer(
		ID3D12GraphicsCommandList& cmdList, 
		const BufferParams& bufferParams, 
		IndexBufferData& bufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept {
		ASSERT(bufferParams.IsDataValid());
		CreateIndexBuffer(cmdList, bufferParams, bufferData, uploadBuffer);
		ASSERT(bufferData.IsDataValid());
	}
}



