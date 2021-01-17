#pragma once

#include <string>

#include <d3d12.h>

class PipelineManager
{
public:
	PipelineManager();
	~PipelineManager();
public:
	bool CreatePSO(const std::string& name, ID3D12PipelineState* pPSO);
	bool CreateRootSignature(const std::string& name, ID3D12RootSignature* pRootSignature);
	bool CreateInputLayout(const std::string& name, UINT numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDesc);
	ID3D12PipelineState* GetPSO(const std::string& name) const;
	ID3D12RootSignature* GetRootSignature(const std::string& name) const;
	D3D12_INPUT_LAYOUT_DESC GetInputLayout(const std::string& name) const;
private:
	PipelineManager(const PipelineManager&);
	void operator = (const PipelineManager&);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

