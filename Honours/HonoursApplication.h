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
#include "Input.h"
#include "FPCamera.h"
#include "OrbitalCamera.h"
#include "TriangleMesh.h"
#include "CubeMesh.h"
#include "Profiler.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

struct DebugValues {
    bool visualize_particles_ = false;
    bool render_analytical_ = false; // If the SDF should be calculated analytically for each ray (without the texture)
    bool pause_positions_ = false;
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
    void SetNaiveImplementation();
    void SetSimpleImplementation();
    void SetComplexImplementation();

    static const UINT FrameCount = 2;

    // Pipeline objects.
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> descriptor_heap_;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    UINT descriptors_allocated_ = 1; // 0th descriptor is for ImGui

    // Application systems
    std::unique_ptr<RayTracer> ray_tracer_ = nullptr;
    std::unique_ptr<Computer> computer_ = nullptr;
    std::unique_ptr<Profiler> profiler_ = nullptr;

    // Debug
    DebugValues debug_;


    // Scene Resources
    Input input_;
    int camera_;
    std::unique_ptr<Camera> cameras_array_[2];

    XMMATRIX projection_matrix_;					///< Identity projection matrix
    XMMATRIX world_matrix_;						///< Identity world matrix
    XMMATRIX ortho_projection_matrix_;						///< Identity orthographic matrix

    TimeSystem timer_;
};
