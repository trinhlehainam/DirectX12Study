#include <cassert>
#include "PMDResouceBinder.h"
#include "../Debug/Dx12Debug.h"

void ResourceBinder::Initialize(const std::vector<SHADER_RESOURCE_TYPE>& types)
{
	
}

ComPtr<ID3D12Resource>& ResourceBinder::AddBuffer(SHADER_RESOURCE_TYPE type, const ComPtr<ID3D12Resource>& pResource)
{
    Resource rs;
    rs.type = type;
    rs.pResource = pResource;
	mResources.push_back(rs);
    return mResources.rbegin()->pResource;
}

void PMDResouceBinder::AddParameter(const std::string& group)
{
	mResourceBinders[group];
}

ResourceBinder& PMDResouceBinder::GetParameter(const std::string& group)
{
#ifdef _DEBUG
	assert(mResourceBinders.count(group));
#endif
	return mResourceBinders[group];
}

void PMDResouceBinder::Build(ComPtr<ID3D12Device>& device, const std::string& groups)
{

}

void PMDResouceBinder::CreateResourceView(ComPtr<ID3D12Device>& pDevice, const std::string& group)
{
#ifdef _DEBUG
	assert(mResourceBinders.count(group));
#endif
	auto& resourceBinder = mResourceBinders[group];
    
    //CreateDescriptorHeap(pDevice, resourceBinder.mDescHeap, resourceBinder.mResources.size());

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
    size_t strideBytes = 0;
    
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    for (auto& resource : resourceBinder.mResources)
        if (resource.type == SHADER_RESOURCE_TYPE::CBV)
        {
            auto& pResource = resource.pResource;
            gpuAddress = pResource->GetGPUVirtualAddress();
            strideBytes = pResource->GetDesc().Width;
            cbvDesc.SizeInBytes = strideBytes;
            cbvDesc.BufferLocation = gpuAddress;
            break;
        }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ※

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(resourceBinder.mpDescHeap->GetCPUDescriptorHandleForHeapStart());
    auto heapSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (auto& resource : resourceBinder.mResources)
    {
        switch (resource.type)
        {
        case SHADER_RESOURCE_TYPE::CBV:
            pDevice->CreateConstantBufferView(
                &cbvDesc,
                heapAddress);
            gpuAddress += strideBytes;
            break;
        case SHADER_RESOURCE_TYPE::SRV:
        {
            auto& pResource = resource.pResource;
            srvDesc.Format = pResource->GetDesc().Format;
            pDevice->CreateShaderResourceView(
                pResource.Get(),
                &srvDesc,
                heapAddress
            );
            break;
        } 
        case SHADER_RESOURCE_TYPE::UAV:
            break;
        default:
            break;
        }
        heapAddress.Offset(1, heapSize);
    }
}

ComPtr<ID3D12DescriptorHeap>& PMDResouceBinder::GetDescriptorHeap(const std::string& group)
{
	return mResourceBinders.at(group).mpDescHeap;
}

ComPtr<ID3D12RootSignature>& PMDResouceBinder::GetRootSignature()
{
	return mRootSignature;
}

void ResourceBinder::CreateDescriptorHeap(ComPtr<ID3D12Device>& pDevice, UINT numDescriptors,
    UINT nodeMask)
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
	descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeap.NumDescriptors = numDescriptors;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeap.NodeMask = 0;
	pDevice->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(mpDescHeap.ReleaseAndGetAddressOf()));
}

void PMDResouceBinder::CreateRootSignature(ComPtr<ID3D12Device>& pDevice)
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    
    std::vector<D3D12_ROOT_PARAMETER> rootParam(mResourceBinders.size());

    int cbvRegisterBase = 0;
    int srvRegisterBase = 0;
    int uvaRegisterBase = 0;
    int rootParamOffset = 0;
    for (auto& resourceBinder : mResourceBinders)
    {
        std::vector<D3D12_DESCRIPTOR_RANGE> range;
        int cbvRegisterCount = 0;
        int srvRegisterCount = 0;
        int uvaRegisterCount = 0;
        for (auto& resource : resourceBinder.second.mResources)
        {
            switch (resource.type)
            {
            case SHADER_RESOURCE_TYPE::CBV:
                cbvRegisterCount++;
                break;
            case SHADER_RESOURCE_TYPE::SRV:
                srvRegisterCount++;
                break;
            case SHADER_RESOURCE_TYPE::UAV:
                uvaRegisterCount++;
                break;
            default:
                break;
            }
        }

        if (cbvRegisterCount > 0)
        {
            range.emplace_back(CD3DX12_DESCRIPTOR_RANGE(
                D3D12_DESCRIPTOR_RANGE_TYPE_CBV,                            // range type
                cbvRegisterCount,                                           // number of descriptors
                cbvRegisterBase));                                          // base shader register)
            cbvRegisterBase += cbvRegisterCount;
        }

        if (srvRegisterCount > 0)
        {
            range.emplace_back(CD3DX12_DESCRIPTOR_RANGE(
                D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                srvRegisterCount,
                srvRegisterBase));

            srvRegisterBase += srvRegisterCount;
        }

        CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[rootParamOffset], range.size(), range.data());
        rootParamOffset++;
    }

    rtSigDesc.pParameters = rootParam.data();
    rtSigDesc.NumParameters = rootParam.size();

    //サンプラらの定義、サンプラとはUVが0未満とか1超えとかのときの
    //動きyや、UVをもとに色をとってくるときのルールを指定するもの
    D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
    //WRAPは繰り返しを表す。
    CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0],
        0,                                  // shader register location
        D3D12_FILTER_MIN_MAG_MIP_POINT);    // Filter     

    samplerDesc[1] = samplerDesc[0];
    samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[1].ShaderRegister = 1;

    rtSigDesc.pStaticSamplers = samplerDesc;
    rtSigDesc.NumStaticSamplers = _countof(samplerDesc);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errBlob;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = pDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(mRootSignature.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(result));
}
