#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

// Given geometry data (vertices and indices), we can call Execute() to generate D3D vertex and index buffers
namespace  GeomBuffersCreator {
	struct Input {
		Input() = default;
		explicit Input(const void* verts, const std::uint32_t numVerts, const std::size_t vertexSize, const void* indices, const std::uint32_t numIndices);

		bool ValidateData() const noexcept;

		const void* mVertsData{ nullptr };
		std::uint32_t mNumVerts{ 0U };
		std::size_t mVertexSize{ 0UL };
		const void* mIndexData{ nullptr };
		std::uint32_t mNumIndices{ 0U };
	};

	struct Output {
		ID3D12Resource* mVertexBuffer{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Resource> mUploadVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};

		ID3D12Resource* mIndexBuffer{ nullptr };
		Microsoft::WRL::ComPtr<ID3D12Resource> mUploadIndexBuffer;
		D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
		std::uint32_t mIndexCount{ 0U };
	};

	void Execute(ID3D12GraphicsCommandList& cmdList, const Input& input, Output& output) noexcept;
}