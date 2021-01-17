#include "PipelineManager.h"

#include <unordered_map>

#include "../Utility/D12Helper.h"

#define IMPL (*m_impl)

using Microsoft::WRL::ComPtr;

class PipelineManager::Impl
{
	friend PipelineManager;
private:
	Impl();
	~Impl();
private:
	bool Has(const std::string& name);
private:
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_psos;
};

bool PipelineManager::Impl::Has(const std::string& name)
{
	return m_psos.count(name);
}

PipelineManager::Impl::Impl()
{

}

PipelineManager::Impl::~Impl()
{

}

/*
* Public method
*/

PipelineManager::PipelineManager():m_impl(new Impl())
{
}

PipelineManager::~PipelineManager()
{
	SAFE_DELETE(m_impl);
}

PipelineManager::PipelineManager(const PipelineManager&) {}

void PipelineManager::operator=(const PipelineManager&) {}

bool PipelineManager::Create(const std::string& name, ID3D12PipelineState* pPSO)
{
	if (IMPL.Has(name)) return false;
	IMPL.m_psos[name] = pPSO;
	return true;
}

ID3D12PipelineState* PipelineManager::Get(const std::string& name) const
{
	return IMPL.Has(name) ? IMPL.m_psos[name].Get() : nullptr;
}


