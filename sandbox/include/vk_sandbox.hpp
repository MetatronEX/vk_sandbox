#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>

namespace sandbox
{
	class sandbox_app
	{
	public:

		static constexpr char* app_name = "sandbox";

		void run();

	private:

		static constexpr unsigned RES_WIDTH = 1280;
		static constexpr unsigned RES_HEIGHT = 720;

		void initialize();
		void app_loop();
		void cleanup();
	};
}