//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include <memory>
#include "TimeSystem.h"
#include "DXSample.h"
#include "Resources.h"
#include "Camera.h"
#include "TriangleMesh.h"
#include "CubeMesh.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class HonoursApplication : public DXSample
{
public:
    HonoursApplication(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    void LoadPipeline();
    void InitGUI();
    void LoadAssets();
    void PopulateCommandList();
    void MoveToNextFrame();
    void WaitForGpu();
    void DrawGUI();

    static const UINT FrameCount = 2;

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
    ComPtr<ID3D12CommandQueue> m_commandQueue;
   // ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> srv_heap_;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ID3D12CommandList* ppCommandLists_[1];
    UINT m_rtvDescriptorSize;

    std::unique_ptr<Resources> resources_ = nullptr;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FrameCount];

    // Scene Resources
    //std::unique_ptr<TriangleMesh> triangle_ = nullptr;
    std::unique_ptr<CubeMesh> cube_ = nullptr;

    Camera camera_;

    XMMATRIX projection_matrix_;					///< Identity projection matrix
    XMMATRIX world_matrix_;						///< Identity world matrix
    XMMATRIX ortho_projection_matrix_;						///< Identity orthographic matrix

    /*inline XMMATRIX GetProjectionMatrix() { return projection_matrix_; }	///< Returns default projection matrix
    inline XMMATRIX GetOrthoProjectionMatrix() { return ortho_projection_matrix_; }	///< Returns default orthographic matrix
    inline XMMATRIX GetWorldMatrix() { return world_matrix_; }		///< Returns identity world matrix*/

    TimeSystem timer_;
};
