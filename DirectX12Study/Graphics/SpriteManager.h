#pragma once
#include <d3d12.h>
#include <string>

class SpriteManager
{
public:
	SpriteManager();
	~SpriteManager();

	bool SetDevice(ID3D12Device* pDevice);
	bool SetWorldPassConstantGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS worldPassConstantGpuAddress);
	bool SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer);
	bool SetViewDepth(ID3D12Resource* pViewDepthBuffer);

	bool Create(const std::string& name, float width, float height, ID3D12Resource* pTexture = nullptr);
	bool Init(ID3D12GraphicsCommandList* pCmdList);
	bool ClearSubresources();

	void Render(ID3D12GraphicsCommandList* pCmdList);

	bool Move(const std::string& name, float x, float y, float z);
	bool Scale(const std::string& name, float x, float y, float z);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

