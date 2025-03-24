/******************************************************************************
 * GPUPrefixSums
 *
 * SPDX-License-Identifier: MIT
 * Copyright Thomas Smith 12/2/2024
 * https://github.com/b0nes164/GPUPrefixSums
 *
 ******************************************************************************/
#pragma once
#include "../stdafx.h"

static winrt::com_ptr<ID3D12Resource> CreateBuffer(ID3D12Device* device,
                                                   uint32_t size, D3D12_HEAP_TYPE heapType,
                                                   D3D12_RESOURCE_STATES initialState,
                                                   D3D12_RESOURCE_FLAGS flags) {
    winrt::com_ptr<ID3D12Resource> buffer;
    auto bufferHeapProps = CD3DX12_HEAP_PROPERTIES(heapType);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
    winrt::check_hresult(device->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE,
                                                         &bufferDesc, initialState, nullptr,
                                                         IID_PPV_ARGS(buffer.put())));
    return buffer;
}

static void UAVBarrierSingle(ID3D12GraphicsCommandList* commandList,
                             winrt::com_ptr<ID3D12Resource> resource) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource.get();
    commandList->ResourceBarrier(1, &barrier);
}