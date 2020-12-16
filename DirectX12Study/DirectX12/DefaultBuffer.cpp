#include "DefaultBuffer.h"
#include "../Utility/D12Helper.h"
#include <cassert>

DefaultBuffer::~DefaultBuffer()
{
    SAFE_DELETE(m_subresource);
}

ID3D12Resource* DefaultBuffer::Resource()
{
    return m_buffer.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS DefaultBuffer::GetGPUVirtualAddress()
{
    return m_buffer.Get()->GetGPUVirtualAddress();
}

bool DefaultBuffer::CreateBuffer(ID3D12Device* pDevice, size_t SyteInBytes)
{
    if (m_buffer != nullptr) return false;
    m_buffer = D12Helper::CreateBuffer(pDevice, SyteInBytes, D3D12_HEAP_TYPE_DEFAULT);
    m_isShaderResourceBuffer = false;
    return true;
}

bool DefaultBuffer::CreateTexture2D(ID3D12Device* pDevice, UINT64 Width, UINT Height, DXGI_FORMAT TextureFormat,
    D3D12_RESOURCE_FLAGS ResourceFlag, D3D12_RESOURCE_STATES ResourceState, const D3D12_CLEAR_VALUE* ClearValue)
{
    if (m_buffer != nullptr) return false;
    m_buffer = D12Helper::CreateTex2DBuffer(pDevice, Width, Height, TextureFormat, ResourceFlag, D3D12_HEAP_TYPE_DEFAULT,
        ResourceState, ClearValue);
    m_isShaderResourceBuffer = true;
    return true;
}

void DefaultBuffer::SetUpSubresource(const void* pData, LONG_PTR RowPitch, LONG_PTR SlicePitch)
{
    if (!m_subresource)
        m_subresource = new D3D12_SUBRESOURCE_DATA();

    m_subresource->pData = pData;
    m_subresource->RowPitch = RowPitch;
    m_subresource->SlicePitch = SlicePitch;
}

bool DefaultBuffer::SetUpSubresource(const void* pData, size_t SizeInBytes)
{
    if (m_isShaderResourceBuffer) return false;

    if(!m_subresource)
        m_subresource = new D3D12_SUBRESOURCE_DATA();

    m_subresource->pData = pData;
    m_subresource->RowPitch = SizeInBytes;
    m_subresource->SlicePitch = SizeInBytes;

    return true;

}

bool DefaultBuffer::UpdateSubresource(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList)
{
    assert(m_subresource);
    assert(m_buffer);
    if (!m_subresource) return false;
    if (!m_buffer) return false;

    size_t sizeInBytes = static_cast<size_t>(m_subresource->SlicePitch);

    m_intermedinateBuffer = D12Helper::CreateBuffer(pDevice, sizeInBytes);

    UpdateSubresources(pCmdList, m_buffer.Get(), m_intermedinateBuffer.Get(), 0, 0, 1, m_subresource);

    if(m_isShaderResourceBuffer)
        D12Helper::ChangeResourceState(pCmdList, m_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    else
        D12Helper::ChangeResourceState(pCmdList, m_buffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ);
        
    return true;
}

bool DefaultBuffer::ClearSubresource()
{
    SAFE_DELETE(m_subresource);
        
    if (!m_intermedinateBuffer) return false;
    m_intermedinateBuffer.Reset();

    return true;
}
