#pragma once
//#include "NvPerfReportGeneratorD3D12.h"
#include "NvPerfReportDefinition.h"

#define NUM_CAPTURES 1

namespace nv {
	namespace perf {
		namespace profiler {
			class ReportGeneratorD3D12;
		}
	}
}

namespace ProfilerGlobal {
	// range name -> range values
	extern std::map<std::string, std::vector<double>> test_results_;
	extern int remaining_captures_;
}

class Profiler
{
public:
	~Profiler();

	void Init(ID3D12Device* device);

	void Update(float dt);

	void FrameStart(ID3D12CommandQueue* queue);
	void FrameEnd();

	void StartCapture();
	bool IsCapturing();

	void PushRange(ID3D12GraphicsCommandList* command_list, const char* range_name);
	void PopRange(ID3D12GraphicsCommandList* command_list);

	static nv::perf::ReportDefinition GetCustomReportDefinition();
	static double GetTotalCaptures() { return NUM_CAPTURES; }



private:
	nv::perf::profiler::ReportGeneratorD3D12* nvperf_;
	float time_since_last_capture_ = 0.0f;

};

