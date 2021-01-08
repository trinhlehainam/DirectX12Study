#include "TextureManager.h"

#include <vector>
#include <unordered_map>

#include "../Utility/D12Helper.h"

#define IMPL (*m_impl)

using Microsoft::WRL::ComPtr;

class TextureManager::Impl
{
	friend TextureManager;
private:
	Impl();
	Impl(ID3D12Device* pDevice);
	~Impl();

	Impl(const Impl&) = delete;
	void operator = (const Impl&) = delete;
private:
	bool Has(const std::string& name);
private:
	using Index_t = uint32_t;
	ID3D12Device* m_device = nullptr;
	Index_t m_maxIndex = -1;
	std::vector<ComPtr<ID3D12Resource>> m_textures;
	std::vector<ComPtr<ID3D12Resource>> m_uploadBuffers;
	std::unordered_map<std::string, Index_t> m_indices;
};

TextureManager::Impl::Impl()
{

}

TextureManager::Impl::Impl(ID3D12Device* pDevice):m_device(pDevice)
{

}

TextureManager::Impl::~Impl()
{
	m_device = nullptr;
}

bool TextureManager::Impl::Has(const std::string& name)
{
	return m_indices.count(name);
}

//
/* PUBLIC INTERFACE METHOD */
//

TextureManager::TextureManager():m_impl(new Impl())
{

}

TextureManager::TextureManager(ID3D12Device* pDevice):m_impl(new Impl(pDevice))
{
}

TextureManager::~TextureManager()
{
	SAFE_DELETE(m_impl);
}

TextureManager::TextureManager(const TextureManager&)
{
}

void TextureManager::operator=(const TextureManager&)
{
}

void TextureManager::SetDevice(ID3D12Device* pDevice)
{
	IMPL.m_device = pDevice;
}

bool TextureManager::Create(ID3D12GraphicsCommandList* pCmdList, const std::string& name, 
	const std::wstring& path)
{
	if (IMPL.Has(name)) return false;
	if (!IMPL.m_device) return false;

	IMPL.m_indices[name] = ++IMPL.m_maxIndex;
	IMPL.m_textures.push_back(nullptr);
	IMPL.m_uploadBuffers.push_back(nullptr);

	auto& texture = IMPL.m_textures[IMPL.m_indices[name]];
	auto& uploadBuffer = IMPL.m_uploadBuffers[IMPL.m_indices[name]];
	texture = D12Helper::CreateTextureFromFilePath(IMPL.m_device, pCmdList, uploadBuffer, path);

	return true;
}

ID3D12Resource* TextureManager::Get(const std::string& name)
{
	return IMPL.m_textures[IMPL.m_indices[name]].Get();
}

