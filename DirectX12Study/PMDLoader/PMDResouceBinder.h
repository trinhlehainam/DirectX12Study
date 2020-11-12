#pragma once
#include <d3dx12.h>
#include <vector>
#include <string>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

enum class SHADER_RESOURCE_TYPE
{
	CBV,
	SRV,
	UAV
};

struct Resource
{
	ComPtr<ID3D12Resource> pResource;
	SHADER_RESOURCE_TYPE type;
};

struct ResourceBinder
{
	std::vector<Resource> mResources;
	ComPtr<ID3D12DescriptorHeap> mpDescHeap;
	void Initialize(const std::vector<SHADER_RESOURCE_TYPE>&);
	ComPtr<ID3D12Resource>& AddBuffer(SHADER_RESOURCE_TYPE, const ComPtr<ID3D12Resource>&);
	void CreateDescriptorHeap(ComPtr<ID3D12Device>&, UINT, UINT = 0);
};

class PMDResouceBinder
{
public:
	void AddParameter(const std::string& group);
	ResourceBinder& GetParameter(const std::string& group);
	void Build(ComPtr<ID3D12Device>& device, const std::string& group);
	void CreateResourceView(ComPtr<ID3D12Device>& device, const std::string& group);

	ComPtr<ID3D12DescriptorHeap>& GetDescriptorHeap(const std::string& group);
	ComPtr<ID3D12RootSignature>& GetRootSignature();

	void CreateRootSignature(ComPtr<ID3D12Device>&);
private:
	std::unordered_map<std::string, ResourceBinder> mResourceBinders;
	ComPtr<ID3D12RootSignature> mRootSignature;
	
};

