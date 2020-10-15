#include <cassert>
#include "Dx12Wrapper.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

bool Dx12Wrapper::Initialize(HWND hwnd)
{
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
    HRESULT result = S_OK;
    for (auto& fLevel : featureLevels)
    {
        result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(&dev_));
        if (SUCCEEDED(result)) {
            featureLevel = fLevel;
            break;
        }
    }
    if (featureLevel == D3D_FEATURE_LEVEL_1_0_CORE)
    {
        OutputDebugString(L"Feature level not found");
        return false;
    }
#if _DEBUG
    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_));
#else
    result = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_));
#endif
    assert(SUCCEEDED(result));

    return true;
}

void Dx12Wrapper::Terminate()
{
    dev_->Release();
}
