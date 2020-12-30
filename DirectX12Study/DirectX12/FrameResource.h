#pragma once
#include <d3dx12.h>

class FrameResource
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAlloc = nullptr;
	uint16_t FenceValue = 0;
};

