/******************************************************************************
 * GPUPrefixSums
 *
 * SPDX-License-Identifier: MIT
 * Copyright Thomas Smith 12/2/2024
 * https://github.com/b0nes164/GPUPrefixSums
 *
 ******************************************************************************/
#pragma once
//#include "pch.h"
#include "GPUPrefixSumBase.h"
#include "CSDLKernels.h"
#include <filesystem>

class ChainedScanDecoupledLookback : public GPUPrefixSumBase {
    winrt::com_ptr<ID3D12Resource> m_scanBumpBuffer;

    CSDLKernels::InitCSDL* m_initCSDL;
    CSDLKernels::CSDLExclusive* m_csdlExclusive;

   public:
    ChainedScanDecoupledLookback(DX::DeviceResources* device_resources);

    ~ChainedScanDecoupledLookback();

   protected:
    void InitComputeShaders() override;

    void DisposeBuffers() override;

    void InitStaticBuffers() override;

    void PrepareScanCmdListExclusive() override;
};