#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "PMDCommon.h"
#include "../DirectX12/UploadBuffer.h"

struct PMDSubMesh
{
	// + Index of Index Buffer
	// start index when GPU accesses Index Buffer
	uint32_t StartIndexLocation;
	// + Index of Vertex Buffer
	// GPU takes this as base index and adds it to value took from Index Buffer
	// => then use calculated value as index to access Vertex Buffer
	// - PS : with this the value of indices from other Meshes don't need to change
	// when concatenated to One Mesh
	uint32_t BaseVertexLocation;
};

struct PMDMesh
{
	std::vector<PMDVertex> vertices;
	std::vector<uint16_t> indices;

	std::unordered_map<std::string, PMDSubMesh> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW vbview;
	D3D12_INDEX_BUFFER_VIEW ibview;

	UploadBuffer<PMDVertex> vertexBuffer;
	UploadBuffer<uint16_t> indexBuffer;

	bool CreateVertexBufferView(ID3D12Device* pDevice);
	bool CreateIndexBufferView(ID3D12Device* pDevice);

};

