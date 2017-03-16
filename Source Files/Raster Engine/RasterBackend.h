#pragma once

#include "Display\Window\Window.h"
#include "Raster Engine\Shader.h"
#include "Raster Engine\RasterJob.h"
#include "Raster Engine\Buffer.h"

#include "RasterBase.h"
#include "OpenGL\OpenGLBackend.h"
#include "Vulkan\VulkanBackend.h"

enum BackendName { OpenGL, Vulkan };

namespace RasterBackend {

	namespace detail {
		extern std::unique_ptr<RasterBase> raster;
	}


	/////////////////////////////////////////
	/*         Public Definitions         */
	/////////////////////////////////////////

	extern BackendName backend;


	void Init();

	//needs to be called from the main thread
	void SetWindowHints(GLFWwindow*&);

	Shader* CreateShader(const std::string&, shader_t);

	Buffer* CreateBuffer(uint size);

	RasterJob* CreateJob();

	void MakeContextCurrent();

	void ResizeWindow(GLFWwindow*, int, int);

	void BuildWindow(GLFWwindow*);

	void Terminate();

	void Draw(GLFWwindow*, RasterJob*);
}
