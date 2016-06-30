#include "D3dData.h"

Microsoft::WRL::ComPtr<IDXGIFactory4> D3dData::mDxgiFactory{ nullptr };
Microsoft::WRL::ComPtr<IDXGISwapChain3> D3dData::mSwapChain{ nullptr };
Microsoft::WRL::ComPtr<ID3D12Device> D3dData::mDevice{ nullptr };