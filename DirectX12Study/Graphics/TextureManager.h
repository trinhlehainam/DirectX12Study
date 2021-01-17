#pragma once

#include <string>

#include <d3d12.h>

class TextureManager
{
public:
	TextureManager();
	TextureManager(ID3D12Device* pDevice);
	~TextureManager();

	void SetDevice(ID3D12Device* pDevice);
	bool Create(ID3D12GraphicsCommandList* pCmdList, const std::string& name, const std::wstring& path);
	ID3D12Resource* Get(const std::string& name);
private:
	// don't allow copy semantics
	TextureManager(const TextureManager&);
	void operator = (const TextureManager&);
private:
	class Impl;
	Impl* m_impl;
};

