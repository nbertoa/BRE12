#include "VertexAndIndexBufferCreator.h"

#include <ResourceManager/ResourceManager.h>
#include <Utils/DebugUtils.h>

VertexAndIndexBufferCreator::BufferCreationData::BufferCreationData(
	const void* data, 
	const std::uint32_t elementCount, 
	const std::size_t elementSize)
	: mData(data)
	, mElementCount(elementCount)
	, mElementSize(elementSize)
{
}

bool VertexAndIndexBufferCreator::BufferCreationData::IsDataValid() const noexcept {
	return mData != nullptr && mElementCount != 0U && mElementSize != 0UL;
}

const VertexAndIndexBufferCreator::VertexBufferData&
VertexAndIndexBufferCreator::VertexBufferData::operator=(
	const VertexBufferData& instance) 
{
	if (this == &instance) {
		return *this;
	}

	mBuffer = instance.mBuffer;
	mBufferView = instance.mBufferView;
	mElementCount = instance.mElementCount;

	return *this;
}

bool VertexAndIndexBufferCreator::VertexBufferData::IsDataValid() const noexcept {
	D3D12_VERTEX_BUFFER_VIEW invalidView{};

	return
		mBuffer != nullptr &&
		mElementCount != 0U &&
		mBufferView.BufferLocation != invalidView.BufferLocation &&
		mBufferView.SizeInBytes != invalidView.SizeInBytes &&
		mBufferView.StrideInBytes != invalidView.StrideInBytes;
}

void VertexAndIndexBufferCreator::CreateVertexBuffer(
	ID3D12GraphicsCommandList& commandList,
	const BufferCreationData& bufferCreationData,
	VertexBufferData& vertexBufferData,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
{
	ASSERT(bufferCreationData.IsDataValid());

	// Create buffer
	const std::uint32_t bufferSize { 
		bufferCreationData.mElementCount * static_cast<std::uint32_t>(bufferCreationData.mElementSize) 
	};
	ResourceManager::Get().CreateDefaultBuffer(
		commandList, 
		bufferCreationData.mData, 
		bufferSize, 
		vertexBufferData.mBuffer, 
		uploadBuffer);
	vertexBufferData.mElementCount = bufferCreationData.mElementCount;

	// Fill view
	vertexBufferData.mBufferView.BufferLocation = vertexBufferData.mBuffer->GetGPUVirtualAddress();
	vertexBufferData.mBufferView.SizeInBytes = bufferSize;
	vertexBufferData.mBufferView.StrideInBytes = static_cast<std::uint32_t>(bufferCreationData.mElementSize);

	ASSERT(vertexBufferData.IsDataValid());
}

const VertexAndIndexBufferCreator::IndexBufferData&
VertexAndIndexBufferCreator::IndexBufferData::operator=(
	const IndexBufferData& instance) 
{
	if (this == &instance) {
		return *this;
	}

	mBuffer = instance.mBuffer;
	mBufferView = instance.mBufferView;
	mElementCount = instance.mElementCount;

	return *this;
}

bool VertexAndIndexBufferCreator::IndexBufferData::IsDataValid() const noexcept {
	D3D12_INDEX_BUFFER_VIEW invalidView{};

	return
		mBuffer != nullptr &&
		mElementCount != 0U &&
		mBufferView.BufferLocation != invalidView.BufferLocation &&
		mBufferView.Format != invalidView.Format &&
		mBufferView.SizeInBytes != invalidView.SizeInBytes;
}

void VertexAndIndexBufferCreator::CreateIndexBuffer(
	ID3D12GraphicsCommandList& commandList,
	const BufferCreationData& bufferCreationData,
	IndexBufferData& indexBufferData,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer) noexcept
{
	ASSERT(bufferCreationData.IsDataValid());

	// Create buffer
	const std::uint32_t elementSize{ static_cast<std::uint32_t>(bufferCreationData.mElementSize) };
	const std::uint32_t bufferSize{ bufferCreationData.mElementCount * elementSize };
	ResourceManager::Get().CreateDefaultBuffer(
		commandList, 
		bufferCreationData.mData, 
		bufferSize, 
		indexBufferData.mBuffer, 
		uploadBuffer);
	indexBufferData.mElementCount = bufferCreationData.mElementCount;

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



