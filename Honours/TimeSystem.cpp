#include "stdafx.h"
#include "TimeSystem.h"


void TimeSystem::Start() {
	previous_time_ = Clock::now();
}

// Called every frame.
// Returns the delta time, in seconds, since the last frame.
double TimeSystem::Update()
{
	// Get current time, get dt since previously recorded time, set current as previous
	Clock::time_point current_time = Clock::now();
	delta_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - previous_time_).count() / 1000000000.f; // converts from nanoseconds to seconds
	previous_time_ = current_time;

	if (track_avg_) {
		// For calculating average FPS. 
		// Sums the FPS value over 500ms and devides by number of frames 
		fps_aggregate_time_ += delta_time_; // How long its been since last averaging
		fps_aggregate_ += 1.f / delta_time_; // Sum of FPS values
		update_counter_++; // Tracks number of frames
		if (fps_aggregate_time_ >= 0.5f) { // Been at least 500ms
			// Take average
			avg_fps_ = fps_aggregate_ / update_counter_;
			fps_aggregate_time_ = 0.f;
			fps_aggregate_ = 0.f;
			update_counter_ = 0.f;
		}
	}

	// Return the dt since last frame
	return delta_time_;
}
