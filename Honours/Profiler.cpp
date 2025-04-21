#include "stdafx.h"
#include "Profiler.h"
#include "NvPerfReportGeneratorD3D12.h"
#include "nvperf_host_impl.h"

//namespace ProfilerGlobal {
//    // range name -> range values
//    std::map<std::string, std::vector<double>> test_results_;
//
//    // buffer name -> size in bytes
//    std::map<std::string, UINT64> memory_usage_results_;
//    UINT64 current_brickpool_size_;
//    UINT64 current_blas_size_;
//
//    int remaining_captures_;
//}

Profiler::~Profiler()
{
    if (cpu_test_vars_.test_mode_) {
        nvperf_->Reset();
    }
}

void Profiler::Init(ID3D12Device* device)
{
    if (cpu_test_vars_.test_mode_) {
        ProfilerGlobal::remaining_captures_ = NUM_CAPTURES + 1; // Extra capture as 1st reports abnormally large values so is discarded

        nvperf_ = new nv::perf::profiler::ReportGeneratorD3D12;

        nvperf_->InitializeReportGenerator(device);
        nvperf_->SetNumNestingLevels(1);
        nvperf_->SetMaxNumRanges(1);
        nvperf_->outputOptions.directoryName = "Profiler";
        nvperf_->outputOptions.enableHtmlReport = false;
        nvperf_->outputOptions.appendDateTimeToDirName = nv::perf::AppendDateTime::no;
    }
}

void Profiler::Update(float dt)
{
    if (cpu_test_vars_.test_mode_) {
        if (ProfilerGlobal::remaining_captures_ > 0 && !nvperf_->IsCollectingReport()) {
            time_since_last_capture_ += dt;

            if (time_since_last_capture_ >= 0.5f) {
                time_since_last_capture_ = 0.0f;
                ProfilerGlobal::remaining_captures_--;
                nvperf_->StartCollectionOnNextFrame();
            }
        }
        else {
            PostQuitMessage(0);
        }
    }
}

void Profiler::FrameStart(ID3D12CommandQueue* queue)
{
    if (cpu_test_vars_.test_mode_) {
        nvperf_->OnFrameStart(queue);
    }
}

void Profiler::FrameEnd()
{
    if (cpu_test_vars_.test_mode_) {
        nvperf_->OnFrameEnd();
    }
}

void Profiler::StartCapture()
{
    if (cpu_test_vars_.test_mode_) {
        nvperf_->StartCollectionOnNextFrame();
    }
}

bool Profiler::IsCapturing()
{
    if (cpu_test_vars_.test_mode_) {
        return nvperf_->IsCollectingReport();
    }
    else return false;
}

void Profiler::PushRange(ID3D12GraphicsCommandList* command_list, const char* range_name)
{
    if (cpu_test_vars_.test_mode_) {
        nvperf_->rangeCommands.PushRange(command_list, range_name);
    }
}

void Profiler::PopRange(ID3D12GraphicsCommandList* command_list)
{
    if (cpu_test_vars_.test_mode_) {
        nvperf_->rangeCommands.PopRange(command_list);
    }
}

void Profiler::RegisterResource(std::string name, UINT64 size)
{
    ProfilerGlobal::memory_usage_results_[name] = size;
}

nv::perf::ReportDefinition Profiler::GetCustomReportDefinition()
{
    static const char* const RequiredCounters[] = {
        "gpu__time_duration", // Time duration
        "tpc__warps_active_shader_cs_realtime", // Warp activity (occupancy)
        "sm__inst_executed_pipe_alu_realtime", // ALU activity
        "sm__inst_executed_pipe_lsu", // LSU activity
        "tpc__warp_launch_cycles_stalled_shader_cs_reason_register_allocation", // Warp stall - register allocation
        "tpc__warp_launch_cycles_stalled_shader_cs_reason_barrier_allocation", // Warp stall - barrier allocation
        "tpc__warp_launch_cycles_stalled_shader_cs_reason_shmem_allocation", // Warp stall - GSM allocation
        "tpc__warp_launch_cycles_stalled_shader_cs_reason_warp_allocation", // Warp stall - warp allocation
        "tpc__warp_launch_cycles_stalled_shader_cs_reason_cta_allocation", // Warp stall - CTA allocation
    };

    static const char* const RequiredRatios[] = {
        "lts__average_t_sector_hit_rate_realtime", // L2 cache hit rate
    };

    static const char* const RequiredThroughputs[] = {
        "rtcore__throughput", // RT core throughput
        "sm__throughput", // SM throughput     
    };

    nv::perf::ReportDefinition reportDefinition = {
                RequiredCounters,
                sizeof(RequiredCounters) / sizeof(RequiredCounters[0]),
                RequiredRatios,
                sizeof(RequiredRatios) / sizeof(RequiredRatios[0]),
                RequiredThroughputs,
                sizeof(RequiredThroughputs) / sizeof(RequiredThroughputs[0]),
                nullptr
    };
    return reportDefinition;
}

