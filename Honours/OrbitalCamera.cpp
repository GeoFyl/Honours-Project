#include "stdafx.h"
#include "OrbitalCamera.h"
#include "TestValues.h"

OrbitalCamera::OrbitalCamera() : Camera()
{
	z_axis_ = XMVectorSet(0.0, 0.0, -1.0f, 1.0f);
}

void OrbitalCamera::Update(float dt)
{
	time_elapsed_ += dt;

	float x = 0.5 + VIEW_DIST * cos(time_elapsed_);
	float y = SCENE == SceneWave ? 0.3 : 0.5;
	float z = 0.5 + VIEW_DIST * sin(time_elapsed_);

	setPosition(x, y, z);

	setRotation(0, atan2f(0.5 - x, 0.5 - z) * 57.2957795, 0);

	update();
}
