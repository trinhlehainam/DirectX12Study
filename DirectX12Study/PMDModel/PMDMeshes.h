#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "PMDCommon.h"
#include "../DirectX12/UploadBuffer.h"

struct PMDSubMesh
{
	uint16_t baseIndex;
};

struct PMDMeshes
{
	using Indices_t = uint16_t;

	std::vector<PMDVertex> vertices;
	std::vector<Indices_t> indices;

	std::unordered_map<std::string, PMDSubMesh> meshIndex;

	D3D12_VERTEX_BUFFER_VIEW vbview;
	D3D12_INDEX_BUFFER_VIEW ibview;

	UploadBuffer<PMDVertex> vertexBuffer;
	UploadBuffer<Indices_t> indexBuffer;

	bool CreateVertexBufferView(ID3D12Device* pDevice);
	bool CreateIndexBufferView(ID3D12Device* pDevice);

};

