/******************************************************************************
 * GPUPrefixSums
 *
 * SPDX-License-Identifier: MIT
 * Copyright Thomas Smith 3/5/2024
 * https://github.com/b0nes164/GPUPrefixSums
 *
 ******************************************************************************/
#pragma once
#include "../stdafx.h"
#include "Utils.h"
#include "../DeviceResources.h"
#include "../include/GFSDK_Aftermath.h"

class GPUPrefixSumBase {
   protected:
    const char* k_scanName;
    const uint32_t k_partitionSize;
    const uint32_t k_maxReadBack;
    const uint32_t k_maxRandSize = 1 << 20;

    uint32_t m_alignedSize;
    uint32_t m_vectorizedSize;
    uint32_t m_partitions;

    std::vector<std::wstring> m_compileArguments;

    winrt::com_ptr<ID3D12Resource> m_scanInBuffer;
    winrt::com_ptr<ID3D12Resource> m_scanOutBuffer;
    winrt::com_ptr<ID3D12Resource> m_threadBlockReductionBuffer;

    DX::DeviceResources* device_resources_;

    GPUPrefixSumBase(const char* scanName, uint32_t partitionSize, uint32_t maxReadBack, DX::DeviceResources* device_resources)
        : k_scanName(scanName), k_partitionSize(partitionSize), k_maxReadBack(maxReadBack), device_resources_(device_resources){};

    ~GPUPrefixSumBase(){};

   public:
    /*virtual void TestExclusiveScanInitRandom(uint32_t testSize, bool shouldReadBack,
                                             bool shouldValidate) {
        if (testSize <= k_maxRandSize) {
            UpdateSize(testSize);
            PrepareScanCmdListExclusive();
            ExecuteCommandList();
        } else {
            printf("Error, test size exceeds maximum test size. \n");
            printf("Due to numeric limits, the maximum test size for");
            printf("test values initialized to random integers is %u.\n", k_maxRandSize);
        }
    }*/

    void DispatchScan() {
        PrepareScanCmdListExclusive();
        ExecuteCommandList();
    }

    void UpdateSize(uint32_t size) {
        if (size <= k_maxRandSize) {
            //TODO : BAD
            const uint32_t alignedSize = align16(size);
            if (m_alignedSize != alignedSize) {
                m_alignedSize = alignedSize;
                m_vectorizedSize = vectorizeAlignedSize(m_alignedSize);
                m_partitions = divRoundUp(m_alignedSize, k_partitionSize);
                DisposeBuffers();
                InitBuffers(m_alignedSize, m_partitions);
            }
        }
        else {
            printf("Error, test size exceeds maximum test size. \n");
            printf("Due to numeric limits, the maximum test size for");
            printf("test values initialized to random integers is %u.\n", k_maxRandSize);
        }
    }

    ID3D12Resource* GetScanInBuffer() { return m_scanInBuffer.get(); }
    ID3D12Resource* GetScanOutBuffer() { return m_scanOutBuffer.get(); }

   protected:
    void Initialize() {
        InitComputeShaders();
        InitStaticBuffers();
    }

    virtual void InitComputeShaders() = 0;



    virtual void DisposeBuffers() = 0;

    virtual void InitStaticBuffers() = 0;

    void InitBuffers(const uint32_t alignedSize, const uint32_t partitions) {
        m_scanInBuffer =
            CreateBuffer(device_resources_->GetD3DDevice(), alignedSize * sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT,
                         D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        m_scanOutBuffer =
            CreateBuffer(device_resources_->GetD3DDevice(), alignedSize * sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT,
                         D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        m_threadBlockReductionBuffer =
            CreateBuffer(device_resources_->GetD3DDevice(), partitions * sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT,
                         D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        m_scanInBuffer->SetName(L"ScanIn");
        GFSDK_Aftermath_DX12_RegisterResource(m_scanInBuffer.get(), new GFSDK_Aftermath_ResourceHandle);
        m_scanOutBuffer->SetName(L"ScanOut");
        GFSDK_Aftermath_DX12_RegisterResource(m_scanOutBuffer.get(), new GFSDK_Aftermath_ResourceHandle);
        m_threadBlockReductionBuffer->SetName(L"ThreadReduction");
        GFSDK_Aftermath_DX12_RegisterResource(m_threadBlockReductionBuffer.get(), new GFSDK_Aftermath_ResourceHandle);
    }

    virtual void PrepareScanCmdListExclusive() = 0;

    void ExecuteCommandList() {
        device_resources_->ExecuteCommandList();
        device_resources_->WaitForGpu();
        device_resources_->ResetCommandList();
    }

    static inline uint32_t divRoundUp(uint32_t x, uint32_t y) { return (x + y - 1) / y; }

    static inline uint32_t align16(uint32_t toAlign) { return (toAlign + 3) / 4 * 4; }

    static inline uint32_t vectorizeAlignedSize(uint32_t alignedSize) { return alignedSize / 4; }
};