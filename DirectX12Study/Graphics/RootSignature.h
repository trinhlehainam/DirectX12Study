#pragma once
#include <d3d12.h>

/*
* Object encapsulates methods that using to create Root Signature
*/

class RootSignature
{
public:
	enum ROOT_DESCRIPTOR_TYPE
	{
		CBV,
		SRV,
		UAV
	};
public:
	RootSignature();
	~RootSignature();
public:
	void AddRootParameterAsDescriptorTable(UINT16 numCBV, UINT16 numSRV, UINT16 numUAV, 
		D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
	bool AddRootParameterAsRootDescriptor(ROOT_DESCRIPTOR_TYPE rootDescriptor,
		D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL);
	void AddStaticSampler();
	bool Create(ID3D12Device* pDevice);
	ID3D12RootSignature* Get() const;
	void Reset();
private:
	// Don't allow copy semantics
	RootSignature(const RootSignature&);
	void operator = (const RootSignature&);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

