#pragma once

#include <wrl.h>

struct ID3D12Resource;

struct Texture {
	Texture() = default;

	bool ValidateData() const noexcept { return mBuffer != nullptr && mUploadBuffer.Get() != nullptr; }

	ID3D12Resource* mBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer{ nullptr };
};
