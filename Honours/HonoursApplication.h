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
#include "DeviceResources.h"
#include "Computer.h"
#include "RayTracer.h"
#include "Resources.h"
#include "Input.h"
#include "FPCamera.h"
#include "OrbitalCamera.h"
#include "TriangleMesh.h"
#include "CubeMesh.h"
#include "Profiler.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

struct DebugValues {
    bool visualize_particles_ = false;
    bool render_analytical_ = false; // If the SDF should be calculated analytically for each ray (without the texture)
    bool pause_positions_ = false;
    float uvw_normals_step_ = 1.f / TEXTURE_RESOLUTION;
    bool render_normals_ = false;
    bool visualize_aabbs_ = false;
    bool use_simple_aabb_ = false;
};

class HonoursApplication : public DXSample
{
public:
    HonoursApplication(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    virtual void OnSizeChanged(UINT width, UINT height, bool minimized);
    virtual IDXGISwapChain* GetSwapchain() { return device_resources_->GetSwapChain(); }

    //inline ID3D12Resource* GetComputeCB() { return compute_cb_->Resource(); }
    inline ID3D12Resource* GetRaytracingCB() { return ray_tracing_cb_->Resource(); }

    inline DebugValues& GetDebugValues() { return debug_; }

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    virtual inline void OnKeyDown(WPARAM key) override { input_.SetKeyDown(key); }
    virtual inline void OnKeyUp(WPARAM key) override { input_.SetKeyUp(key); }
    virtual inline void OnMouseMove(WPARAM x, WPARAM y) override { input_.setMouseX(x); input_.setMouseY(y); }
    virtual inline void OnLeftButtonDown(WPARAM x, WPARAM y) override { input_.setLeftMouse(true); }
    virtual inline void OnLeftButtonUp(WPARAM x, WPARAM y) override { input_.setLeftMouse(false); }
    virtual inline void OnRightButtonDown(WPARAM x, WPARAM y) override { input_.setRightMouse(true); }
    virtual inline void OnRightButtonUp(WPARAM x, WPARAM y) override { input_.setRightMouse(false); }

    UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse = UINT_MAX);

    inline ID3D12DescriptorHeap* GetDescriptorHeap() { return descriptor_heap_.Get(); }

private:
    void LoadPipeline();
    void InitGUI();
    void CopyRaytracingOutputToBackbuffer();
    void DrawGUI();

    void SwitchCamera();

    static const UINT FrameCount = 2;

    // Pipeline objects.
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> descriptor_heap_;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    UINT descriptors_allocated_ = 1; // 0th descriptor for ImGui

    // Application systems
    std::unique_ptr<Resources> resources_ = nullptr;
    std::unique_ptr<RayTracer> ray_tracer_ = nullptr;
    std::unique_ptr<Computer> computer_ = nullptr;
    std::unique_ptr<Profiler> profiler_ = nullptr;

    // Upload buffers
   // std::unique_ptr<UploadBuffer<ComputeCB>> compute_cb_ = nullptr;
    std::unique_ptr<UploadBuffer<RayTracingCB>> ray_tracing_cb_ = nullptr;

    // Debug
    DebugValues debug_;


    // Scene Resources
    Input input_;
    int camera_;
    std::unique_ptr<Camera> cameras_array_[2];
    //std::unique_ptr<FPCamera> camera_; 
    //std::unique_ptr<OrbitalCamera> camera_;

    XMMATRIX projection_matrix_;					///< Identity projection matrix
    XMMATRIX world_matrix_;						///< Identity world matrix
    XMMATRIX ortho_projection_matrix_;						///< Identity orthographic matrix

    TimeSystem timer_;
};
