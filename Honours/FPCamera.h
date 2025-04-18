#pragma once

#include "Camera.h"
#include "input.h"


using namespace DirectX;

class FPCamera : public Camera
{
public:
	FPCamera(Input* in, int width, int height, HWND hnd);	///< Initialised default camera object

	virtual void Update(float dt) override { move(dt); };

private:
	bool move(float dt);	///< Move camera, handles basic camera movement

	Input* input;
	int winWidth, winHeight;///< stores window width and height
	int deltax, deltay;		///< for mouse movement
	POINT cursor;			///< Used for converting mouse coordinates for client to screen space
	HWND wnd;				///< handle to the window
};