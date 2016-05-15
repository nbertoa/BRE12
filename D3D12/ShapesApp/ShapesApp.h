#pragma once

#include <DirectXMath.h>
#include <memory>

#include <App/App.h>
#include <ResourceManager\UploadBuffer.h>

#if defined(DEBUG) || defined(_DEBUG)                                                                                                                                                            
#define _CRTDBG_MAP_ALLOC          
#include <cstdlib>             
#include <crtdbg.h>               
#endif 

class ShapesApp : public App {
public:
	explicit ShapesApp(HINSTANCE hInstance);
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

	ID3D12PipelineState* mPSO{ nullptr };
	ID3D12RootSignature* mRootSignature{ nullptr };

	ID3D12Resource* mVertexBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};

	ID3D12Resource* mIndexBuffer{ nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
	std::uint32_t mNumIndices{ 0U };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCBVHeap;	
	UploadBuffer* mCBVsUploadBuffer{ nullptr };

	void Update(const float dt) noexcept override;
	void Draw(const float dt) noexcept override;

	void BuildPSO() noexcept;
	void BuildVertexAndIndexBuffers() noexcept;
	void BuildConstantBuffers() noexcept;
	void BuildRootSignature() noexcept;

	void UpdateConstantBuffers() noexcept;
};

