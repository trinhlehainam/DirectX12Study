#pragma once
#include <d3dx12.h>

using Microsoft::WRL::ComPtr;


void OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob);
