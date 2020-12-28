#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "../DirectX12/DefaultBuffer.h"

struct SubMesh
{
	uint32_t IndexCount = 0;
	// + Index of Index Buffer
	// start index when GPU accesses Index Buffer
	uint32_t StartIndexLocation = 0;
	// + Index of Vertex Buffer
	// GPU takes this as base index and adds it to value took from Index Buffer
	// => then use calculated value as index to access Vertex Buffer
	// - PS : with this the value of indices from other Meshes don't need to change
	// when concatenated to One Mesh
	uint32_t BaseVertexLocation = 0;
};

template<class Vertex>
struct Mesh
{
	std::vector<Vertex> Vertices;
	std::vector<uint16_t> Indices16;

	std::unordered_map<std::string, SubMesh> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;

	DefaultBuffer VertexBuffer;
	DefaultBuffer IndexBuffer;

	bool CreateBuffers(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList);
	bool CreateViews();
	bool ClearSubresource();
};

template<class Vertex>
inline bool Mesh<Vertex>::CreateBuffers(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList)
{
	size_t sizeOfVertices = sizeof(Vertex) * Vertices.size();
	VertexBuffer.CreateBuffer(pDevice, sizeOfVertices);
	VertexBuffer.SetUpSubresource(Vertices.data(), sizeOfVertices);
	VertexBuffer.UpdateSubresource(pDevice, pCmdList);

	size_t sizeOfIndices = sizeof(uint16_t) * Indices16.size();
	IndexBuffer.CreateBuffer(pDevice, sizeOfIndices);
	IndexBuffer.SetUpSubresource(Indices16.data(), sizeOfIndices);
	IndexBuffer.UpdateSubresource(pDevice, pCmdList);

	return true;
}

template<class Vertex>
inline bool Mesh<Vertex>::CreateViews()
{
	uint32_t sizeOfVertices = sizeof(Vertex) * Vertices.size();
	VertexBufferView.BufferLocation = VertexBuffer.GetGPUVirtualAddress();
	VertexBufferView.SizeInBytes = sizeOfVertices;
	VertexBufferView.StrideInBytes = sizeof(Vertex);

	uint32_t sizeOfIndices = sizeof(uint16_t) * Indices16.size();
	IndexBufferView.BufferLocation = IndexBuffer.GetGPUVirtualAddress();
	IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	IndexBufferView.SizeInBytes = sizeOfIndices;

	return true;
}

template<class Vertex>
inline bool Mesh<Vertex>::ClearSubresource()
{
	VertexBuffer.ClearSubresource();
	IndexBuffer.ClearSubresource();
	Vertices.clear();
	Indices16.clear();
	return true;
}
