#pragma once

#include <DirectXMath.h>
#include <memory>

#include <App/D3dApp.h>
#include <DXUtils\UploadBuffer.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class ShapesApp : public D3DApp {
public:
	ShapesApp(HINSTANCE hInstance);
	ShapesApp(const ShapesApp& rhs) = delete;
	ShapesApp& operator=(const ShapesApp& rhs) = delete;

	void Initialize() noexcept override;
	
protected:
	struct Vertex {
		Vertex() {}
		Vertex(const DirectX::XMFLOAT4& p) 
			: mPosition(p)
		{}

		DirectX::XMFLOAT4 mPosition{ 0.0f, 0.0f, 0.0f, 0.0f };
	};

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
	std::uint32_t mNumIndices{ 0U };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCBVHeap;	
	std::unique_ptr<UploadBuffer> mCBVsUploadBuffer;	

	void Update(const Timer& timer) noexcept override;
	void Draw(const Timer& timer) noexcept override;

	void BuildPSO() noexcept;
	void BuildVertexAndIndexBuffers() noexcept;
	void BuildConstantBuffers() noexcept;
	void BuildRootSignature() noexcept;
};

