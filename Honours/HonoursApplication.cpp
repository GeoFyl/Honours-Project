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
    DXSample(width, height, name)/*,
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_fenceValues{},
    m_rtvDescriptorSize(0)*/
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

    // Initialise camera
    camera_ = std::make_unique<FPCamera>(&input_, m_width, m_height, Win32Application::GetHwnd());
    camera_->setPosition(0.5, 0.5, -3.f);
    //camera_.setRotation(0, -30, 0);
    LoadPipeline();
   // LoadAssets();

    InitGUI();
    timer_.Start();
}

// Load the rendering pipeline dependencies.
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

    // Create constant buffers
    ray_tracing_cb_ = std::make_unique<UploadBuffer<RayTracingCB>>(device_resources_->GetD3DDevice(), 1, true);

    compute_cb_ = std::make_unique<UploadBuffer<ComputeCB>>(device_resources_->GetD3DDevice(), 1, true);
}

void HonoursApplication::InitGUI()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Win32Application::GetHwnd());
    ImGui_ImplDX12_Init(device_resources_->GetD3DDevice(), FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap_.Get(),
        descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
        descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
}

// Load the sample assets.
void HonoursApplication::LoadAssets()
{
    // Create buffers and root signatures
    resources_ = std::make_unique<Resources>(device_resources_->GetD3DDevice());

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = resources_->GetRootSignature();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(device_resources_->GetD3DDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    //// Create the command list.
    //ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
    //ppCommandLists_[0] = m_commandList.Get();

    

    // Create meshes
    //triangle_ = std::make_unique<TriangleMesh>(m_device.Get(), m_commandList.Get());
    device_resources_->GetCommandList()->Reset(device_resources_->GetCommandAllocator(), m_pipelineState.Get());
    //cube_ = std::make_unique<CubeMesh>(device_resources_->GetD3DDevice(), device_resources_->GetCommandList());

    // Execute command list and wait until assets have been uploaded to the GPU.
    device_resources_->ExecuteCommandList();
    device_resources_->WaitForGpu();

    //// Close and execute command list.
    //ThrowIfFailed(m_commandList->Close());
    //m_commandQueue->ExecuteCommandLists(1, ppCommandLists_);

    //// Create synchronization objects and wait until assets have been uploaded to the GPU.
    //{
    //    ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    //    m_fenceValues[m_frameIndex]++;

    //    // Create an event handle to use for frame synchronization.
    //    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    //    if (m_fenceEvent == nullptr)
    //    {
    //        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    //    }

    //    // Wait for the command list to execute; we are reusing the same command 
    //    // list in our main loop but for now, we just want to wait for setup to 
    //    // complete before continuing.
    //    WaitForGpu();

        // Release mesh upload heaps
        //triangle_->ReleaseUploaders();
        //cube_->ReleaseUploaders();
    //}
}

// Update frame-based values.
void HonoursApplication::OnUpdate()
{
    timer_.Update();

    camera_->move(timer_.GetDeltaTime());

    // Update constant buffers
    RayTracingCB buff;
    buff.camera_pos_ = camera_->getPosition();
    buff.view_proj_ = XMMatrixMultiply(camera_->getViewMatrix(), projection_matrix_);
    buff.inv_view_proj_ = XMMatrixTranspose(XMMatrixInverse(nullptr, buff.view_proj_));
    buff.view_proj_ = XMMatrixTranspose(buff.view_proj_);
    buff.uvw_step_ = uvw_normals_step_;

    if (debug_.visualize_particles_) buff.rendering_flags_ |= RENDERING_FLAG_VISUALIZE_PARTICLES;
    if (debug_.render_analytical_) buff.rendering_flags_ |= RENDERING_FLAG_ANALYTICAL;
    if (render_normals_) buff.rendering_flags_ |= RENDERING_FLAG_NORMALS;

    ray_tracing_cb_->CopyData(0, buff);

    if (!pause_positions_) {
        ComputeCB compute_buffer_data;
        compute_buffer_data.time_ = timer_.GetElapsedTime();
        compute_cb_->CopyData(0, compute_buffer_data);
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

    /*ID3D12DescriptorHeap* srv_heaps[1] = { descriptor_heap_.Get() };
    device_resources_->GetCommandList()->SetDescriptorHeaps(1, srv_heaps);*/

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = device_resources_->GetRenderTargetView();
    device_resources_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record all the commands we need to render the scene into the command list.
    //PopulateCommandList();

    if (!pause_positions_) computer_->ComputePostitions();
    if (!(debug_.render_analytical_ || debug_.visualize_particles_)) computer_->ComputeSDFTexture();

    ray_tracer_->RayTracing();
    CopyRaytracingOutputToBackbuffer();

    // Record commands for drawing GUI
    DrawGUI();

    // Present the back buffer
    device_resources_->Present();
}

void HonoursApplication::PopulateCommandList()
{
    //// Command list allocators can only be reset when the associated 
    //// command lists have finished execution on the GPU; apps should use 
    //// fences to determine GPU execution progress.
    //ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

    //// However, when ExecuteCommandList() is called on a particular command 
    //// list, that command list can then be reset at any time and must be before 
    //// re-recording.
    //ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

    // Set necessary state.
    ID3D12GraphicsCommandList4* command_list = device_resources_->GetCommandList();
    command_list->SetGraphicsRootSignature(resources_->GetRootSignature());
    command_list->RSSetViewports(1, &device_resources_->GetScreenViewport());
    command_list->RSSetScissorRects(1, &device_resources_->GetScissorRect());
    ID3D12DescriptorHeap* srv_heaps[1] = { descriptor_heap_.Get() };
    command_list->SetDescriptorHeaps(1, srv_heaps);

    // Indicate that the back buffer will be used as a render target.
    //command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    /*CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    command_list->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);*/

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = device_resources_->GetRenderTargetView();
    command_list->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    XMMATRIX view_projection = XMMatrixMultiply(camera_->getViewMatrix(), projection_matrix_);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    command_list->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // Set world-view-projection matrix for current object
    XMMATRIX world_view_proj = XMMatrixMultiplyTranspose(world_matrix_, view_projection);
    resources_->SetWorldViewProj(world_view_proj);

    // Set root views to the ones for current object and draw
    resources_->SetRootViews(command_list);
    //cube_->SendData(command_list);
    //cube_->Draw(command_list);

    // Record commands for drawing GUI
    DrawGUI();

    // Indicate that the back buffer will now be used to present.
    //m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    //ThrowIfFailed(m_commandList->Close());
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

// Wait for pending GPU work to complete.
//void HonoursApplication::WaitForGpu()
//{
//    // Schedule a Signal command in the queue.
//    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));
//
//    // Wait until the fence has been processed.
//    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
//    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
//
//    // Increment the fence value for the current frame.
//    m_fenceValues[m_frameIndex]++;
//}

// Records commands for drawing GUI
void HonoursApplication::DrawGUI()
{
    //ImGui::ShowDemoWindow();

    ImGui::Text("FPS: %.2f", timer_.GetCurrentFPS());

    ImGui::Checkbox("Analytical distances", &debug_.render_analytical_);
    ImGui::Checkbox("Visualize particles", &debug_.visualize_particles_);
    ImGui::Checkbox("Freeze particles", &pause_positions_);
    ImGui::Checkbox("Debug normals", &render_normals_);
    ImGui::Text("Normals uvw step (texture):");
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.9f);
    ImGui::SliderFloat("##", &uvw_normals_step_, 0.0001f, 0.1f, "%.5f");
    ImGui::PopItemWidth();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), device_resources_->GetCommandList());
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

    // ------------------------===================================== TODO: NEED TO CLEANLY DESTROY EVERYTHING =====================================-------------------------------------------
}

void HonoursApplication::OnSizeChanged(UINT width, UINT height, bool minimized)
{
    if (!device_resources_->WindowSizeChanged(width, height, minimized))
    {
        return;
    }
    DXSample::UpdateForSizeChange(width, height);

    projection_matrix_ = XMMatrixPerspectiveFovLH((float)XM_PI / 4.0f, m_aspectRatio, 0.01f, 100.f);

    // todo: resize raytracing output  
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

// Prepare to render the next frame.
//void HonoursApplication::MoveToNextFrame()
//{
//    // Schedule a Signal command in the queue.
//    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
//    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));
//
//    // Update the frame index.
//    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
//
//    // If the next frame is not ready to be rendered yet, wait until it is ready.
//    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
//    {
//        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
//        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
//    }
//
//    // Set the fence value for the next frame.
//    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
//}
