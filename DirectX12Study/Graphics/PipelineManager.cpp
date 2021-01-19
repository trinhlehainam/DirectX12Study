#include "PipelineManager.h"

#include <vector>
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
	bool HasPSO(const std::string& name);
	bool HasRootSignature(const std::string& name);
	bool HasInputLayout(const std::string& name);
private:
	std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> m_rootSigs;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_psos;
	std::unordered_map<std::string, D3D12_INPUT_LAYOUT_DESC> m_inputLayouts;
	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> m_inputs;
};

bool PipelineManager::Impl::HasPSO(const std::string& name)
{
	return m_psos.count(name);
}

bool PipelineManager::Impl::HasRootSignature(const std::string& name)
{
	return m_rootSigs.count(name);
}

bool PipelineManager::Impl::HasInputLayout(const std::string& name)
{
	return m_inputLayouts.count(name);
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

bool PipelineManager::CreatePSO(const std::string& name, ID3D12PipelineState* pPSO)
{
	if (!pPSO) return false;
	if (IMPL.HasPSO(name)) return false;
	IMPL.m_psos[name] = pPSO;
	return true;
}

bool PipelineManager::CreateRootSignature(const std::string& name, ID3D12RootSignature* pRootSignature)
{
	if (IMPL.HasRootSignature(name)) return false;
	IMPL.m_rootSigs[name] = pRootSignature;
	return true;
}

bool PipelineManager::CreateInputLayout(const std::string& name, UINT numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDesc)
{
	if (IMPL.HasInputLayout(name)) return false;
	IMPL.m_inputs[name].reserve(numElements);
	IMPL.m_inputs[name] = std::vector<D3D12_INPUT_ELEMENT_DESC>(pInputElementDesc, pInputElementDesc + numElements);
	IMPL.m_inputLayouts[name] = { IMPL.m_inputs[name].data() , static_cast<UINT>(IMPL.m_inputs[name].size()) };
	return true;
}

ID3D12PipelineState* PipelineManager::GetPSO(const std::string& name) const
{
	return IMPL.HasPSO(name) ? IMPL.m_psos[name].Get() : nullptr;
}

ID3D12RootSignature* PipelineManager::GetRootSignature(const std::string& name) const
{
	return IMPL.HasRootSignature(name) ? IMPL.m_rootSigs[name].Get() : nullptr;
}

D3D12_INPUT_LAYOUT_DESC PipelineManager::GetInputLayout(const std::string& name) const
{
	return IMPL.HasInputLayout(name) ? IMPL.m_inputLayouts[name] : D3D12_INPUT_LAYOUT_DESC{};
}


