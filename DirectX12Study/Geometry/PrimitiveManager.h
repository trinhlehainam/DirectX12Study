#pragma once

#include <string>
#include <d3d12.h>

#include "GeometryCommon.h"

class PrimitiveManager
{
public:
	enum GEOMETRY
	{
		CYLINDER,
		GEOSPHERE,
		GRID
	};
public:
	PrimitiveManager();
	PrimitiveManager(ID3D12Device* pDevice);
	~PrimitiveManager();

	bool SetDevice(ID3D12Device* pDevice);
	bool SetWorldPassConstantGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS worldPassConstantGpuAddress);
	bool SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer);
	bool SetViewDepth(ID3D12Resource* pViewDepthBuffer);

	bool Create(const std::string& name, Geometry::Mesh primitive);
	// Need to set up all resource for PMD Manager BEFORE initialize it
	bool Init(ID3D12GraphicsCommandList* cmdList);

	bool ClearSubresources();
public:
	bool Move(const std::string& name, float x, float y, float z);
public:
	void Update(const float& deltaTime);
	void Render(ID3D12GraphicsCommandList* pCmdList);
	void RenderDepth(ID3D12GraphicsCommandList* pCmdList);
private:
	PrimitiveManager(const PrimitiveManager&) = delete;
	PrimitiveManager& operator = (const PrimitiveManager&) = delete;
private:
	class Impl;
	Impl* m_impl = nullptr;
};

