/******************************************************************************
 * GPUPrefixSums
 *
 * SPDX-License-Identifier: MIT
 * Copyright Thomas Smith 12/2/2024
 * https://github.com/b0nes164/GPUPrefixSums
 *
 ******************************************************************************/

#include "ChainedScanDecoupledLookback.h"

ChainedScanDecoupledLookback::ChainedScanDecoupledLookback(DX::DeviceResources* device_resources)
    : GPUPrefixSumBase("ChainedScanDecoupledLookback ", 3072, 1 << 13, device_resources) {

    Initialize();
}

ChainedScanDecoupledLookback::~ChainedScanDecoupledLookback() {}

void ChainedScanDecoupledLookback::InitComputeShaders() {
    const std::filesystem::path path = "Shaders/ChainedScanDecoupledLookback.hlsl";
    m_initCSDL = new CSDLKernels::InitCSDL(device_resources_->GetD3DDevice(), m_compileArguments, path);
    m_csdlExclusive = new CSDLKernels::CSDLExclusive(device_resources_->GetD3DDevice(), m_compileArguments, path);
}

void ChainedScanDecoupledLookback::DisposeBuffers() {
    m_scanInBuffer = nullptr;
    m_scanOutBuffer = nullptr;
    m_threadBlockReductionBuffer = nullptr;
}

void ChainedScanDecoupledLookback::InitStaticBuffers() {
    m_scanBumpBuffer =
        CreateBuffer(device_resources_->GetD3DDevice(), sizeof(uint32_t), D3D12_HEAP_TYPE_DEFAULT,
                     D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void ChainedScanDecoupledLookback::PrepareScanCmdListExclusive() {
    m_initCSDL->Dispatch(device_resources_->GetCommandList(), m_scanBumpBuffer->GetGPUVirtualAddress(),
                         m_threadBlockReductionBuffer->GetGPUVirtualAddress(), m_partitions);
    UAVBarrierSingle(device_resources_->GetCommandList(), m_scanBumpBuffer);
    UAVBarrierSingle(device_resources_->GetCommandList(), m_threadBlockReductionBuffer);

    m_csdlExclusive->Dispatch(device_resources_->GetCommandList(), m_scanInBuffer->GetGPUVirtualAddress(),
                              m_scanOutBuffer->GetGPUVirtualAddress(),
                              m_scanBumpBuffer->GetGPUVirtualAddress(),
                              m_threadBlockReductionBuffer, m_vectorizedSize, m_partitions);

    UAVBarrierSingle(device_resources_->GetCommandList(), m_scanOutBuffer);
}