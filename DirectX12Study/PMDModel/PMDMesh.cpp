#include "PMDMesh.h"

bool PMDMesh::CreateBuffers(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList)
{

	size_t sizeOfVertices = sizeof(PMDVertex) * vertices.size();
	vertexBuffer.CreateBuffer(pDevice, sizeOfVertices);
	vertexBuffer.SetUpSubresource(vertices.data(), sizeOfVertices);
	vertexBuffer.UpdateSubresource(pDevice, pCmdList);

	size_t sizeOfIndices = sizeof(uint16_t) * indices.size();
	indexBuffer.CreateBuffer(pDevice, sizeOfIndices);
	indexBuffer.SetUpSubresource(indices.data(), sizeOfIndices);
	indexBuffer.UpdateSubresource(pDevice, pCmdList);

	return true;
}

bool PMDMesh::CreateViews()
{
	size_t sizeOfVertices = sizeof(PMDVertex) * vertices.size();
	vbview.BufferLocation = vertexBuffer.GetGPUVirtualAddress();
	vbview.SizeInBytes = sizeOfVertices;
	vbview.StrideInBytes = sizeof(PMDVertex);

	size_t sizeOfIndices = sizeof(uint16_t) * indices.size();
	ibview.BufferLocation = indexBuffer.GetGPUVirtualAddress();
	ibview.Format = DXGI_FORMAT_R16_UINT;
	ibview.SizeInBytes = sizeOfIndices;

	return true;
}

bool PMDMesh::ClearSubresource()
{
	vertexBuffer.ClearSubresource();
	indexBuffer.ClearSubresource();
	vertices.clear();
	indices.clear();
	return true;
}