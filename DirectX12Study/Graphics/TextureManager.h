#pragma once

#include <d3d12.h>

class TextureManager
{
public:
	TextureManager();
	TextureManager(ID3D12Device* pDevice);
	~TextureManager();

	void SetDevice(ID3D12Device* pDevice);
	bool Create(ID3D12GraphicsCommandList* pCmdList, const char* name, const wchar_t* path);
	ID3D12Resource* Get(const char* name);
private:
	// don't allow copy semantics
	TextureManager(const TextureManager&);
	void operator = (const TextureManager&);
private:
	class Impl;
	Impl* m_impl;
};

