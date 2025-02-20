#ifndef COMPUTE_DISPATCH
#define COMPUTE_DISPATCH

#include "ComputeGridCommon.hlsli"

StructuredBuffer<GridSurfaceCounts> surface_counts_ : register(t0);
RWStructuredBuffer<uint3> surface_cells_dispatch_args_ : register(u0);

// Shader for setting number of suface cell detection shader thread groups to dispatch.
// Executes once all suface blocks detected.
[numthreads(1, 1, 1)]
void CSDispatchSurfaceCellDetection(uint3 dispatch_ID : SV_DispatchThreadID)
{
    // Determine dispatch size - one thread group per surface block
    surface_cells_dispatch_args_[0] = uint3(surface_counts_[0].surface_blocks, 1, 1);
}

#endif