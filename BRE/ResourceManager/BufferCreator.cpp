#include "BufferCreator.h"

#include <ResourceManager/ResourceManager.h>
#include <Utils/DebugUtils.h>

namespace BufferCreator {
	BufferData::BufferData(const void* data, const std::uint32_t elementCount, const std::size_t elementSize)
		: mData(data)
		, mElemCount(elementCount)
		, mElementSize(elementSize)
	{
	}

	bool BufferData::IsDataValid() const noexcept {
		return mData != nullptr && mElemCount != 0U && mElementSize != 0UL;
	}

	const VertexBufferData& VertexBufferData::operator=(const VertexBufferData& instance) {
		if (this == &instance) {
			return *this;
		}

		mBuffer = instance.mBuffer;
		mBufferView = instance.mBufferView;
		mElementCount = instance.mElementCount;

		return *this;
	}

	bool VertexBufferData::IsDataValid() const noexcept {
		D3D12_VERTEX_BUFFER_VIEW invalidView{};

		return 
			mBuffer != nullptr && 
			mElementCount != 0U && 
			mBufferView.BufferLocation != invalidView.BufferLocation &&
			mBufferView.SizeInBytes != invalidView.SizeInBytes &&
			mBufferView.StrideInBytes != invalidView.StrideInBytes;
	}

	void CreateVertexBuffer(
		ID3D12GraphicsCommandList& commandList,
		const BufferData& bufferData,
		VertexBufferData& vertexBufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
	{
		ASSERT(bufferData.IsDataValid());

		// Create buffer
		const std::uint32_t bufferSize{ bufferData.mElemCount * static_cast<std::uint32_t>(bufferData.mElementSize) };
		ResourceManager::Get().CreateDefaultBuffer(commandList, bufferData.mData, bufferSize, vertexBufferData.mBuffer, uploadBuffer);
		vertexBufferData.mElementCount = bufferData.mElemCount;

		// Fill view
		vertexBufferData.mBufferView.BufferLocation = vertexBufferData.mBuffer->GetGPUVirtualAddress();
		vertexBufferData.mBufferView.SizeInBytes = bufferSize;
		vertexBufferData.mBufferView.StrideInBytes = static_cast<std::uint32_t>(bufferData.mElementSize);

		ASSERT(vertexBufferData.IsDataValid());
	}

	const IndexBufferData& IndexBufferData::operator=(const IndexBufferData& instance) {
		if (this == &instance) {
			return *this;
		}

		mBuffer = instance.mBuffer;
		mBufferView = instance.mBufferView;
		mElementCount = instance.mElementCount;

		return *this;
	}

	bool IndexBufferData::IsDataValid() const noexcept {
		D3D12_INDEX_BUFFER_VIEW invalidView{};

		return
			mBuffer != nullptr && 
			mElementCount != 0U && 
			mBufferView.BufferLocation != invalidView.BufferLocation &&
			mBufferView.Format != invalidView.Format &&
			mBufferView.SizeInBytes != invalidView.SizeInBytes;
	}

	void CreateIndexBuffer(
		ID3D12GraphicsCommandList& commandList,
		const BufferData& bufferData,
		IndexBufferData& indexBufferData,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
	{
		ASSERT(bufferData.IsDataValid());

		// Create buffer
		const std::uint32_t elementSize{ static_cast<std::uint32_t>(bufferData.mElementSize) };
		const std::uint32_t bufferSize{ bufferData.mElemCount * elementSize };
		ResourceManager::Get().CreateDefaultBuffer(commandList, bufferData.mData, bufferSize, indexBufferData.mBuffer, uploadBuffer);
		indexBufferData.mElementCount = bufferData.mElemCount;

		// Set index format
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
		switch (elementSize)
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
		indexBufferData.mBufferView.BufferLocation = indexBufferData.mBuffer->GetGPUVirtualAddress();
		indexBufferData.mBufferView.Format = format;
		indexBufferData.mBufferView.SizeInBytes = bufferSize;

		ASSERT(indexBufferData.IsDataValid());
	}
}



