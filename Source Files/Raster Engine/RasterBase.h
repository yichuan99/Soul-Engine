#pragma once

#include "Display\Window\Window.h"
#include "Raster Engine\Shader.h"
#include "Raster Engine\RasterJob.h"
#include "Raster Engine\Buffer.h"


class RasterBase {
public:
	RasterBase();
	~RasterBase();

	virtual void SetWindowHints(GLFWwindow*&) = 0;
	virtual void BuildWindow(GLFWwindow*) = 0;
	virtual void Draw(GLFWwindow*, RasterJob*) = 0;
	virtual void ResizeWindow(GLFWwindow*, int, int) = 0;

	virtual void MakeContextCurrent() = 0;

private:
};

