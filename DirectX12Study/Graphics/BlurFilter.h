#pragma once
#include <d3d12.h>

class BlurFilter
{
public:
	BlurFilter();
	~BlurFilter();
	bool Create(ID3D12Device* pDevice, UINT16 textureWidth, UINT16 textureHeight, DXGI_FORMAT textureFormat);
	
	// Blur destination texture
	// *** destination texture needed set to COMMON STATE before ***
	void Blur(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* destinationTexture, UINT16 blurCount);
private:
	// Disable copy semantics
	BlurFilter(const BlurFilter&);
	void operator = (const BlurFilter&);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

