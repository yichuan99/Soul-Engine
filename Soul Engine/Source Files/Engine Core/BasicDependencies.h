#pragma once

#include <Windows.h>
#include <IOstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include <stdlib.h>
#include <vector>
#include <list>
#include <queue> 
#include <map>
#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <math.h>
#include <cmath>
#include <ctime>
#include <thread>
#include <future>
#include <fstream>
#include <sstream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include "Metrics.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "Utility\OpenGL\GLError.h"
#include <cuda.h>
#include <cudaGL.h>
//#include <boost\fiber\all.hpp>  wait to do this until multithreading

namespace std {

	inline void *align(size_t alignment, size_t size, void *start, size_t bufferSize) {
		return (void *)((reinterpret_cast<uintptr_t>(start)+static_cast<uintptr_t>(alignment - 1)) & static_cast<uintptr_t>(~(alignment - 1)));
	}

}