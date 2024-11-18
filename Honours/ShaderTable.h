#pragma once
#include "Utilities.h"

// Adapted from Microsoft samples
class ShaderRecord
{
public:
    ShaderRecord(void* pShaderIdentifier) : m_ShaderIdentifier(pShaderIdentifier) {}

    ShaderRecord(void* pShaderIdentifier, void* pLocalRootArguments, UINT localRootArgumentsSize) :
        m_ShaderIdentifier(pShaderIdentifier), m_LocalRootArguments(pLocalRootArguments, localRootArgumentsSize) {}

    void CopyTo(void* dest) const;

private:
    struct PointerWithSize
    {
        void* ptr;
        UINT size;

        PointerWithSize()
            : ptr(nullptr), size(0) {}
        PointerWithSize(void* _ptr, UINT _size)
            : ptr(_ptr), size(_size) {}
    };

    void* m_ShaderIdentifier;
    PointerWithSize m_LocalRootArguments;
};

class ShaderTable
{
public:
    ShaderTable(ID3D12Device* device, UINT record_count, UINT root_params_size)
    {
        mElementByteSize = Utilities::Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + root_params_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * record_count),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&shader_table_)));

        ThrowIfFailed(shader_table_->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

        // We do not need to unmap until we are done with the resource.  However, we must not write to
        // the resource while it is in use by the GPU (so we must use synchronization techniques).
    }

    ShaderTable(const ShaderTable& rhs) = delete;
    ShaderTable& operator=(const ShaderTable& rhs) = delete;
    ~ShaderTable()
    {
        if (shader_table_ != nullptr)
            shader_table_->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* Resource()const
    {
        return shader_table_.Get();
    }

    void CopyRecord(int elementIndex, const ShaderRecord& record)
    {
        record.CopyTo(&mMappedData[elementIndex * mElementByteSize]);
        //memcpy(, &data, sizeof(T));
    }

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> shader_table_;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};