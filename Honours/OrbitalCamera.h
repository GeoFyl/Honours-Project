#pragma once
#include "Camera.h"
class OrbitalCamera : public Camera
{
public:
	OrbitalCamera();
	virtual void Update(float dt) override;

private:
	float time_elapsed_ = 0;
	XMVECTOR z_axis_;
};

