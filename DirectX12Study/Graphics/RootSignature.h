#pragma once
#include <d3d12.h>

/*
* Object encapsulates methods that using to create Root Signature Interface
*/
class RootSignature
{
public:
	enum ROOT_DESCRIPTOR_TYPE
	{
		CONSTANT_BUFFER_VIEW,
		SHADER_RESOURCE_VIEW,
		UNORDERED_ACCESS_VIEW
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
	bool Init(ID3D12Device* pDevice);
	ID3D12RootSignature* Get();
	void SetEmpty();
private:
	RootSignature(const RootSignature&);
	void operator = (const RootSignature&);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

