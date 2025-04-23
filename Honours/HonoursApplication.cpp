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

#include "stdafx.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "HonoursApplication.h"

HonoursApplication::HonoursApplication(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name)
{
    // Initialize the world matrix to the identity matrix.
    world_matrix_ = XMMatrixIdentity();

    // Create the projection matrix for 3D rendering.
    projection_matrix_ = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0f, m_aspectRatio, 0.01f, 100.f);

    // Create an orthographic projection matrix for 2D rendering.
    ortho_projection_matrix_ = XMMatrixOrthographicLH((float)m_width, (float)m_height, 0.01f, 100.f);
}

void HonoursApplication::OnInit()
{
    // Initialise device resources
    device_resources_ = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_UNKNOWN,
        FrameCount,
        D3D_FEATURE_LEVEL_11_0,
        DX::DeviceResources::c_RequireTearingSupport,
        DXSample::m_adapterIDoverride
    );
    device_resources_->RegisterDeviceNotify(this);
    device_resources_->SetWindow(Win32Application::GetHwnd(), m_width, m_height);
    device_resources_->InitializeDXGIAdapter();
    device_resources_->CreateDeviceResources();
    RayTracer::CheckRayTracingSupport(device_resources_->GetD3DDevice());
    device_resources_->CreateWindowSizeDependentResources();

    // Initialise cameras
    cameras_array_[0] = std::make_unique<OrbitalCamera>();
    cameras_array_[1] = std::make_unique<FPCamera>(&input_, m_width, m_height, Win32Application::GetHwnd());
    if (SCENE == SceneNormals) {
        cameras_array_[1]->setPosition(0.59f, 0.51f, 0.22f);
        cameras_array_[1]->setRotation(6.62f, -38.75f, 0.f);
        camera_ = 1;
    }
    else {
        cameras_array_[1]->setPosition(0.5, 0.5, -3.f);
        camera_ = 0;
    }

    // Initialise values used by shaders
    if (cpu_test_vars_.test_mode_) {
        debug_.render_normals_ = true;
    }
    switch (cpu_test_vars_.implementation_) {
    case Naive:
        SetNaiveImplementation();
        break;
    case Simple:
        SetSimpleImplementation();
        break;
    case Complex:
    default:
        SetComplexImplementation();
        break;
    }

    LoadPipeline();

    profiler_ = std::make_unique<Profiler>();
    profiler_->Init(device_resources_->GetD3DDevice());

    InitGUI();

    timer_.Start();
}

// Load the rendering pipeline dependencies and systems.
void HonoursApplication::LoadPipeline()
{
    // Describe and create a descriptor heap to hold constant buffer, shader resource, and unordered access views (CBV/SRV/UAV)
    D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
    srv_heap_desc.NumDescriptors = 4;
    srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(device_resources_->GetD3DDevice()->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&descriptor_heap_)));

    // Create computer
    computer_ = std::make_unique<Computer>(device_resources_.get(), this);

    // Create ray tracer
    ray_tracer_ = std::make_unique<RayTracer>(device_resources_.get(), this, computer_.get());
    computer_->SetRayTracer(ray_tracer_.get());
}

void HonoursApplication::InitGUI()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Win32Application::GetHwnd());
    ImGui_ImplDX12_Init(device_resources_->GetD3DDevice(), FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap_.Get(),
        descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
        descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
}

// Update frame-based values and compute grid etc
void HonoursApplication::OnUpdate()
{
    device_resources_->ResetCommandList();

    timer_.Update(!profiler_->IsCapturing());

    if (!profiler_->IsCapturing()) { // Don't update while profiler is capturing

        profiler_->Update(timer_.GetDeltaTime());
       
        cameras_array_[camera_]->Update(timer_.GetDeltaTime());

        // Update rt constant buffer
        RayTracingCB& buffer = ray_tracer_->GetRaytracingCB()->Values();
        buffer.camera_pos_ = cameras_array_[camera_]->getPosition();
        buffer.camera_lookat_ = cameras_array_[camera_]->getLookAt();
        buffer.view_proj_ = XMMatrixMultiply(cameras_array_[camera_]->getViewMatrix(), projection_matrix_);
        buffer.inv_view_proj_ = XMMatrixTranspose(XMMatrixInverse(nullptr, buffer.view_proj_));
        buffer.view_proj_ = XMMatrixTranspose(buffer.view_proj_);

        buffer.rendering_flags_ = RENDERING_FLAG_NONE;
        if (debug_.visualize_particles_) buffer.rendering_flags_ |= RENDERING_FLAG_VISUALIZE_PARTICLES;
        if (debug_.render_analytical_) buffer.rendering_flags_ |= RENDERING_FLAG_ANALYTICAL;
        if (debug_.render_normals_) buffer.rendering_flags_ |= RENDERING_FLAG_NORMALS;
        if (debug_.visualize_aabbs_) buffer.rendering_flags_ |= RENDERING_FLAG_VISUALIZE_AABBS;
        if (debug_.use_simple_aabb_) buffer.rendering_flags_ |= RENDERING_FLAG_SIMPLE_AABB;

        ray_tracer_->GetRaytracingCB()->CopyData(0);

        // Update particle simulation unless paused or the scene is Grid or Normals
        if (!debug_.pause_positions_ && SCENE != SceneGrid && SCENE != SceneNormals) {
            computer_->GetConstantBuffer()->Values().time_ = timer_.GetElapsedTime();
            computer_->GetConstantBuffer()->CopyData(0);
            computer_->ComputePostitions();
        }
    }
    profiler_->FrameStart(device_resources_->GetCommandQueue());

    // Grid construction, AABB creation and BVH update - ranges to profile specified in the functions
    if (!(debug_.use_simple_aabb_)) {    
        computer_->ComputeGrid(profiler_.get()); 

        computer_->ComputeAABBs(profiler_.get());
        ray_tracer_->GetAccelerationStructure()->UpdateStructure(profiler_.get());
    }
    else {
        // Execute and wait for work to finish need this when not calling ComputeAABBs and UpdateStructure
        device_resources_->ExecuteCommandList();
        device_resources_->WaitForGpu();
    }

    // Create 3D texture/ brick pool
    if (!(debug_.render_analytical_ || debug_.visualize_particles_)) {
        device_resources_->ResetCommandList();

        if (debug_.use_simple_aabb_) { // Simple method
            profiler_->PushRange(device_resources_->GetCommandList(), "Simple Texture");
            computer_->ComputeSimpleSDFTexture();
            profiler_->PopRange(device_resources_->GetCommandList());
        }
        else { // Complex method
            profiler_->PushRange(device_resources_->GetCommandList(), "Particle Reordering");
            computer_->SortParticleData(); // Particle sorting 
            profiler_->PopRange(device_resources_->GetCommandList());

            profiler_->PushRange(device_resources_->GetCommandList(), "Brick Pool");
            computer_->ComputeBrickPoolTexture(); // Create brick pool
            profiler_->PopRange(device_resources_->GetCommandList());
        }

        device_resources_->ExecuteCommandList();
        device_resources_->WaitForGpu();
    }

    // Perform ray tracing 
    if (debug_.use_simple_aabb_ || ray_tracer_->GetAccelerationStructure()->IsStructureBuilt()) {
        device_resources_->ResetCommandList();
        ray_tracer_->RayTracing(profiler_.get());
        device_resources_->ExecuteCommandList();
        device_resources_->WaitForGpu();
    }

}

// Render the scene.
void HonoursApplication::OnRender()
{
    if (!device_resources_->IsWindowVisible())
    {
        return;
    }

    // Prepare GUI for new frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Prepare the command list and render target for rendering.
    device_resources_->Prepare(m_pipelineState.Get());

    // Copy the output from ray tracing to the back buffer
    if (debug_.use_simple_aabb_ || ray_tracer_->GetAccelerationStructure()->IsStructureBuilt()) {
        CopyRaytracingOutputToBackbuffer();
    }

    ID3D12DescriptorHeap* srv_heaps[1] = { descriptor_heap_.Get() };
    device_resources_->GetCommandList()->SetDescriptorHeaps(1, srv_heaps);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = device_resources_->GetRenderTargetView();
    device_resources_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands for drawing GUI
    DrawGUI();

    profiler_->FrameEnd();

    // Present the back buffer
    device_resources_->Present();
}

// Copy the raytracing output to the backbuffer.
void HonoursApplication::CopyRaytracingOutputToBackbuffer()
{
    auto commandList = device_resources_->GetCommandList();
    auto renderTarget = device_resources_->GetRenderTarget();
    ID3D12Resource* ray_tracing_output = ray_tracer_->GetRaytracingOutput();

    D3D12_RESOURCE_BARRIER preCopyBarriers[2];
    preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
    preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(ray_tracing_output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

    commandList->CopyResource(renderTarget, ray_tracing_output);

    D3D12_RESOURCE_BARRIER postCopyBarriers[2];
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
    postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(ray_tracing_output, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
}

// Records commands for drawing GUI
void HonoursApplication::DrawGUI()
{
   // ImGui::ShowDemoWindow();

    ImGui::Text("FPS: %.2f", timer_.GetCurrentFPS());

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Implementation")) {

        if (ImGui::RadioButton("Naive", (int*)&cpu_test_vars_.implementation_, Naive)) {
            SetNaiveImplementation();
        }
        if (ImGui::RadioButton("Simple", (int*)&cpu_test_vars_.implementation_, Simple)) {
            SetSimpleImplementation();
        }
        if (ImGui::RadioButton("Complex", (int*)&cpu_test_vars_.implementation_, Complex)) {
            SetComplexImplementation();
        }
    }
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Visualisation")) {
        ImGui::Checkbox("Visualize bounding volumes", &debug_.visualize_aabbs_);
        ImGui::Checkbox("Visualise particles", &debug_.visualize_particles_);
        ImGui::Checkbox("Pause particles", &debug_.pause_positions_);
    }
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Text("Select scene:");
        if (ImGui::Button("Grid")) {
            SCENE = SceneGrid;
            computer_->GetTestValsBuffer()->Values().scene_ = SceneGrid;
            computer_->GetTestValsBuffer()->CopyData(0);
            computer_->GenerateParticles();
        } ImGui::SameLine();
        if (ImGui::Button("Random")) {
            SCENE = SceneRandom;
            computer_->GetTestValsBuffer()->Values().scene_ = SceneRandom;
            computer_->GetTestValsBuffer()->CopyData(0);
            computer_->GenerateParticles();
        } ImGui::SameLine();
        if (ImGui::Button("Wave")) {
            SCENE = SceneWave;
            computer_->GetTestValsBuffer()->Values().scene_ = SceneWave;
            computer_->GetTestValsBuffer()->CopyData(0);
            computer_->GenerateParticles();
        }
        if (ImGui::Button("Normals Test")) {
            SCENE = SceneNormals;
            computer_->GetTestValsBuffer()->Values().scene_ = SceneNormals;
            computer_->GetTestValsBuffer()->CopyData(0);
            computer_->GenerateParticles();
            cameras_array_[1]->setPosition(0.59f, 0.51f, 0.22f);
            cameras_array_[1]->setRotation(6.62f, -38.75f, 0.f);
        }

        ImGui::Text("Select camera:");
        if (ImGui::RadioButton("Orbital", &camera_, 0)) {
            SwitchCamera();
        }ImGui::SameLine();
        if (ImGui::RadioButton("First person", &camera_, 1)) {
            SwitchCamera();
        }

    }
    
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Debug")) {
        ImGui::Checkbox("Debug normals", &debug_.render_normals_);
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), device_resources_->GetCommandList());
}

void HonoursApplication::SwitchCamera()
{
    int prev_cam = 1 - camera_;

    cameras_array_[camera_]->setPosition(cameras_array_[prev_cam]->getPosition().x, cameras_array_[prev_cam]->getPosition().y, cameras_array_[prev_cam]->getPosition().z);
    cameras_array_[camera_]->setRotation(cameras_array_[prev_cam]->getRotation().x, cameras_array_[prev_cam]->getRotation().y, cameras_array_[prev_cam]->getRotation().z);
}

void HonoursApplication::SetNaiveImplementation()
{
    debug_.use_simple_aabb_ = true;
    debug_.render_analytical_ = true;
}

void HonoursApplication::SetSimpleImplementation()
{
    debug_.use_simple_aabb_ = true;
    debug_.render_analytical_ = false;
}

void HonoursApplication::SetComplexImplementation()
{
    debug_.use_simple_aabb_ = false;
    debug_.render_analytical_ = false;
}

void HonoursApplication::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    device_resources_->WaitForGpu();

    // Destroy GUI
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void HonoursApplication::OnSizeChanged(UINT width, UINT height, bool minimized)
{
    if (!device_resources_->WindowSizeChanged(width, height, minimized))
    {
        return;
    }
    DXSample::UpdateForSizeChange(width, height);

    projection_matrix_ = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0f, m_aspectRatio, 0.01f, 100.f);

    ray_tracer_->CreateRaytracingOutputResource();
}

void HonoursApplication::OnDeviceLost()
{
}

void HonoursApplication::OnDeviceRestored()
{
}

// Allocate a descriptor and return its index. 
// If the passed descriptorIndexToUse is valid, it will be used instead of allocating a new one.
UINT HonoursApplication::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse)
{
    UINT descriptor_size = device_resources_->GetD3DDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto descriptorHeapCpuBase = descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
    if (descriptorIndexToUse >= descriptor_heap_->GetDesc().NumDescriptors)
    {
        descriptorIndexToUse = descriptors_allocated_++;
    }
    *cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, descriptor_size);
    return descriptorIndexToUse;
}
