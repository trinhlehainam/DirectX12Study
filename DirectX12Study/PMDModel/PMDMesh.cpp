#include "PMDMesh.h"

bool PMDMesh::CreateVertexBufferView(ID3D12Device* pDevice)
{
	vertexBuffer.Create(pDevice, vertices.size());
	vertexBuffer.CopyData(vertices);

	vbview.BufferLocation = vertexBuffer.GetGPUVirtualAddress();
	vbview.SizeInBytes = vertexBuffer.SizeInBytes();
	vbview.StrideInBytes = vertexBuffer.ElementSize();

	return true;
}

bool PMDMesh::CreateIndexBufferView(ID3D12Device* pDevice)
{
	indexBuffer.Create(pDevice, indices.size());
	indexBuffer.CopyData(indices);

	ibview.BufferLocation = indexBuffer.GetGPUVirtualAddress();
	ibview.Format = DXGI_FORMAT_R16_UINT;
	ibview.SizeInBytes = indexBuffer.SizeInBytes();

	return true;
}
