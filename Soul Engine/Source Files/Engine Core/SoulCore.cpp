
/////////////////////////Includes/////////////////////////////////

#include "SoulCore.h"
#include "Utility\CUDA\CUDAHelper.cuh"
#include "Utility\Vulkan\VulkanBackend.h"
#include "Engine Core/BasicDependencies.h"

#include "Settings.h"
#include "Multithreading\Scheduler.h"
#include "Engine Core/Frame/Frame.h"
#include "Engine Core/Camera/CUDA/Camera.cuh"
#include "Input/InputState.h"
#include "Ray Engine/RayEngine.h"
#include "Physics Engine\PhysicsEngine.h"
#include "Renderer\Renderer.h"
#include "Bounding Volume Heirarchy/BVH.h"
#include "Resources\Objects\Hand.h"
#include "Utility\CUDA\CUDADevices.cuh"




namespace Soul {


	/////////////////////////Variables and Declarations//////////////////

	uint seed;

	typedef struct window{
		GLFWwindow* windowHandle;
		WindowType type;
	}window;

	typedef struct renderer{
		Renderer* rendererHandle;
		RenderType type;
		float timeModifier;
	}renderer;

	std::vector<window> windows;
	GLFWwindow* masterWindow = nullptr;

	int monitorCount;
	GLFWmonitor** monitors;

	std::vector<renderer> renderObjects;

	std::list<Scene*> scenes;

	Settings* settings;

	bool usingDefaultCamera;
	std::vector<Camera*> cameras;
	Camera* mouseCamera;

	int engineRefreshRate;

	float CurrentDelta();
	void ClearColor(float, float, float, float);

	/////////////////////////Synchronization///////////////////////////

	void SynchCPU(){

	}
	void SynchGPU(){
		CudaCheck(cudaDeviceSynchronize());
	}
	void Synch(){
		SynchCPU();
		SynchGPU();
	}

	/////////////////////////Hints and Toggles///////////////////////////





	bool RequestRenderSwitch(RenderType newR){
		return true;
	}
	bool RequestWindowSwitch(WindowType newW){
		return true;
	}
	bool RequestScreenSize(glm::uvec2 newScreen){
		return true;
	}

	/////////////////////////Engine Core/////////////////////////////////


	//Call to deconstuct both the engine and its dependencies
	void SoulShutDown(){
		Soul::Synch();
		RayEngine::Clean();
		CudaCheck(cudaDeviceReset());

		for (auto const& win : windows){
			glfwDestroyWindow(win.windowHandle);
		}

		delete Soul::settings;
		glfwTerminate();
	}


	void InputToCamera(GLFWwindow* window, Camera* camera){

		if (camera != nullptr){

			int width, height;
			glfwGetWindowSize(window, &width, &height);

			mouseCamera->OffsetOrientation(
				(float)(InputState::GetInstance().xPos / width * camera->FieldOfView().x),
				(float)(InputState::GetInstance().yPos / height * camera->FieldOfView().y));
		}

	}

	void UpdateDefaultCamera(GLFWwindow* window, double deltaTime){
		double moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
			moveSpeed = 9 * METER * deltaTime;
		}
		else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS){
			moveSpeed = 1 * METER * deltaTime;
		}
		else{
			moveSpeed = 4.5 * METER * deltaTime;
		}

		//fill with freecam variable
		if (true){
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
				mouseCamera->OffsetPosition(float(moveSpeed) * -mouseCamera->Forward());
			}
			else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
				mouseCamera->OffsetPosition(float(moveSpeed) * mouseCamera->Forward());
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
				mouseCamera->OffsetPosition(float(moveSpeed) * -mouseCamera->Right());
			}
			else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
				mouseCamera->OffsetPosition(float(moveSpeed) * mouseCamera->Right());
			}
			if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS){
				mouseCamera->OffsetPosition(float(moveSpeed) * -glm::vec3(0, 1, 0));
			}
			else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS){
				mouseCamera->OffsetPosition(float(moveSpeed) * glm::vec3(0, 1, 0));
			}
		}

	}

	void Run()
	{

		SynchGPU();

		int width, height;
		glfwGetWindowSize(masterWindow, &width, &height);
		glfwSetCursorPos(masterWindow, width / 2.0f, height / 2.0f);

		glfwPollEvents();

		double deltaTime = 1.0 / engineRefreshRate;

		for (auto const& scene : scenes){
			scene->Build(deltaTime);
		}

		//timer info for loop
		double t = 0.0f;
		double currentTime = glfwGetTime();
		double accumulator = 0.0f;



		//stop loop when glfw exit is called
		while (!glfwWindowShouldClose(masterWindow)){
			double newTime = glfwGetTime();
			double frameTime = newTime - currentTime;
			//std::cout << "FPS:: " <<1.0f / frameTime << std::endl;

			//setting up timers
			if (frameTime > 0.25){
				frameTime = 0.25;
			}
			currentTime = newTime;
			accumulator += frameTime;

			//# of updates based on accumulated time

			while (accumulator >= deltaTime){
				SynchGPU();

				deltaTime = 1.0 / engineRefreshRate;

				//loading and updates for multithreading

				glfwPollEvents();

				if (usingDefaultCamera){
					UpdateDefaultCamera(masterWindow, deltaTime);
					InputToCamera(masterWindow, mouseCamera);
				}

				SynchGPU();


				//apply camera changes to their matrices
				for (auto const& cam : cameras){
					cam->UpdateVariables();
				}
				SynchGPU();
				glfwGetWindowSize(masterWindow, &width, &height);

				//UpdateVulkan
				//VulkanBackend::GetInstance().UpdateVulkanProjection();

				//Update();

				cudaEvent_t start, stop;
				float time;
				cudaEventCreate(&start);
				cudaEventCreate(&stop);
				cudaEventRecord(start, 0);

				for (auto const& scene : scenes){
					scene->Build(deltaTime);
				}

				cudaEventRecord(stop, 0);
				cudaEventSynchronize(stop);
				cudaEventElapsedTime(&time, start, stop);
				cudaEventDestroy(start);
				cudaEventDestroy(stop);

				std::cout << "Building Execution: " << time << "ms" << std::endl;
				SynchGPU();

				for (auto const& scene : scenes){
					PhysicsEngine::Process(scene);
				}


				InputState::GetInstance().ResetOffsets();

				t += deltaTime;
				accumulator -= deltaTime;
			}
			SynchGPU();

			for (auto const& rend : renderObjects){
				rend.rendererHandle->RenderSetup({ width, height }, mouseCamera, deltaTime);
			}

			for (auto const& scene : scenes){
				RayEngine::Clear();
				RayEngine::Process(scene);
			}

			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			SynchGPU();
			for (auto const& rend : renderObjects){
				//integration bool
				rend.rendererHandle->Render(false);
			}
			SynchGPU();

			///////////////////////////////////////////////////////////////////////until vulkan


			glfwSwapBuffers(masterWindow);
			////////////////////////////////////////////////////////////////////////////////////
		//	VulkanBackend::GetInstance().DrawFrame(masterWindow, width, height);

		}

		//Put Vulkan into idle
		//VulkanBackend::GetInstance().IdleDevice();

		SoulShutDown();

	}
}

/////////////////////////User Interface///////////////////////////

void SoulSignalClose(){

	glfwSetWindowShouldClose(Soul::masterWindow, GLFW_TRUE);

}

void SoulRun(){
	Soul::Run();
}

void SetKey(int key, void(*func)(void)){
	InputState::GetInstance().SetKey(key, std::bind(func));
}

void AddObject(Scene* scene, glm::vec3& globalPos, const char* file, Material* mat){
	Object* obj = new Object(globalPos, file, mat);
	scene->AddObject(obj);
}
void RemoveObject(void* object){

}

int GetSetting(std::string request){
	return Soul::settings->Retrieve(request);
}

int GetSetting(std::string request, int defaultSet){
	return Soul::settings->Retrieve(request, defaultSet);
}

void SetSetting(std::string rName, int rValue){
	Soul::settings->Set(rName, rValue);
}

//Initializes Soul. This must be called before using variables or 
//any other functions relating to the engine.
void SoulInit(){

	Soul::seed = uint(time(NULL));
	srand(Soul::seed);

	Soul::settings = new Settings("Settings.ini");

	Devices::ExtractDevices();

	Soul::Synch();

	if (!glfwInit()){
		Soul::SoulShutDown();
	}

	


	RenderType  win = static_cast<RenderType>(GetSetting("Renderer", 2));

	Soul::engineRefreshRate = GetSetting("Engine Refresh Rate", 60);

	Soul::monitors = glfwGetMonitors(&Soul::monitorCount);

	Soul::usingDefaultCamera = true;
	Soul::mouseCamera = new Camera();
	Soul::cameras.push_back(Soul::mouseCamera);

	Soul::mouseCamera->SetPosition(glm::vec3(-(METER * 2), METER * 2 * 2, -(METER * 2)));
	Soul::mouseCamera->OffsetOrientation(45, 45);
}

//the moniter number, and a float from 0-1 of the screen size for each dimension,
//if its the fisrt, it becomes the master window
GLFWwindow* SoulCreateWindow(int monitor, float xSize, float ySize){

	GLFWmonitor* monitorIn = Soul::monitors[monitor];

	WindowType  win = static_cast<WindowType>(GetSetting("Window", 2));


	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_VISIBLE, true);

	//////////////////////////////////////////////////for vulkan
	//glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	///////////////////////////////////////////////////
	const GLFWvidmode* mode = glfwGetVideoMode(monitorIn);

	GLFWwindow* windowOut;

	if (win == FULLSCREEN){

		glfwWindowHint(GLFW_RESIZABLE, false);
		glfwWindowHint(GLFW_DECORATED, false);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		windowOut = glfwCreateWindow(mode->width, mode->height, "Soul Engine", monitorIn, NULL);

	}
	else if (win == WINDOWED){

		glfwWindowHint(GLFW_RESIZABLE, true);
		windowOut = glfwCreateWindow(int(xSize*mode->width), int(ySize*mode->height), "Soul Engine", NULL, NULL);

	}

	else if (win == BORDERLESS){

		glfwWindowHint(GLFW_RESIZABLE, false);
		glfwWindowHint(GLFW_DECORATED, false);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		windowOut = glfwCreateWindow(int(xSize*mode->width), int(ySize*mode->height), "Soul Engine", NULL, NULL);

	}
	else{
		throw std::runtime_error("NO Window setting found");
	}



	if (!windowOut){
		throw std::runtime_error("GLFW window failed");
	}


	Soul::windows.push_back({ windowOut, win });

	if (Soul::masterWindow == nullptr){

		Soul::masterWindow = windowOut;

		glfwMakeContextCurrent(Soul::masterWindow);

		glfwSetInputMode(windowOut, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		glfwSetKeyCallback(windowOut, Input::KeyCallback);
		glfwSetScrollCallback(windowOut, Input::ScrollCallback);
		glfwSetCursorPosCallback(windowOut, Input::MouseCallback);
	}


	////////////////////////////////////////////////////remove for vulkan
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK){

		throw std::runtime_error("glewInit failed");

	}

	glEnable(GL_DEPTH_TEST); // enable depth-testing

	glDepthMask(GL_TRUE);  // turn on

	glDepthFunc(GL_LEQUAL);

	glDepthRange(0.0f, 1.0f);

	glEnable(GL_TEXTURE_2D);
	/////////////////////////////////////////////////////////////////////

	//glfwSetWindowUserPointer(windowOut, &VulkanBackend::GetInstance());
	//glfwSetWindowSizeCallback(windowOut, VulkanBackend::OnWindowResized);

	//////////////////////////

	Soul::renderer rend = {
		new Renderer(*Soul::mouseCamera, glm::uvec2(int(xSize*mode->width), int(ySize*mode->height))),
		SPECTRAL,
		1.0f
	};

	Soul::renderObjects.push_back(rend);

	//////////////////////////

	//VulkanBackend::GetInstance().AddWindow(windowOut, int(xSize*mode->width), int(ySize*mode->height), Soul::renderObjects[0].rendererHandle->targetData);

	return windowOut;
}

void SubmitScene(Scene* scene){
	Soul::scenes.push_back(scene);
}

void RemoveScene(Scene* scene){
	Soul::scenes.remove(scene);
}

int main()
{
	SoulInit();

	//create a Window
	GLFWwindow* win=SoulCreateWindow(0, 0.95f, 0.95f);

	SetKey(GLFW_KEY_ESCAPE, SoulSignalClose);

	Material* whiteGray = new Material();
	whiteGray->diffuse = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
	whiteGray->emit = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	Scene* scene = new Scene();
	AddObject(scene, glm::vec3(0, 0, 0), "Rebellion.obj", whiteGray);

	SubmitScene(scene);

	SoulRun();

	return 0;
}