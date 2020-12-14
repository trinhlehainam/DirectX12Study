#pragma once
#include <vector>
#include <unordered_map>
#include <string>

#include "PMDCommon.h"
#include "../Utility/DefaultBuffer.h"

struct SubMesh
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

	std::unordered_map<std::string, SubMesh> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW vbview;
	D3D12_INDEX_BUFFER_VIEW ibview;

	DefaultBuffer vertexBuffer;
	DefaultBuffer indexBuffer;

	bool CreateBuffers(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList);
	bool CreateViews();
	bool ClearSubresource();
};

