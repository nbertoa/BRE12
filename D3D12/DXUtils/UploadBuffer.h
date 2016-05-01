#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

#include <DxUtils/d3dx12.h>
#include <Utils/DebugUtils.h>

class UploadBuffer{
public:
	UploadBuffer(ID3D12Device& device, const size_t elemSize, const uint32_t elemCount);

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer();

	ID3D12Resource* Resource() const { return mBuffer.Get(); }

	void CopyData(const uint32_t elemIndex, uint8_t* srcData, const size_t srcDataSize);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
	uint8_t* mMappedData = nullptr;
	size_t mElemSize = 0U;
};