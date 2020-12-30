#include "FrameResource.h"
#include "../Utility/D12Helper.h"

FrameResource::FrameResource(ID3D12Device* pDevice)
{
	D12Helper::ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdAlloc.GetAddressOf())));
}
