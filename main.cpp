#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	bool isComplete() { return graphicsFamily.has_value(); }
};

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
	    instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestoryDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
	    instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

class HelloTriangleApplication {
  public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}

  private:
	GLFWwindow *m_Window;
	VkInstance m_Instance;
	uint32_t m_ExtensionCount = 0;
	VkDebugUtilsMessengerEXT debugMessenger;

	void initWindow() {
		glfwInit();
		// Because GLFW was originally designed to create an OpenGL context, we
		// need to tell it to not create an OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// disable resized window for now
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		pickPhysicalDevice();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_Window)) {
			glfwPollEvents();
		}
	}

	void cleanUp() {
		if (enableValidationLayers) {
			DestoryDebugUtilsMessengerEXT(m_Instance, debugMessenger, nullptr);
		}
		vkDestroyInstance(m_Instance, nullptr);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error(
			    "validation layere requested, but not available!");
		}

		// A lot of information in Vulkan is passed through structs instead of
		// function parameters and we’ll have to fill in one more struct to
		// provide sufficient information for creating an instance.
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// tells the Vulkan driver which global extensions and validation layers
		// we want to use.
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount =
		    static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount =
			    static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext =
			    (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void checkExtensionSupport() {
		// check for optional functionality
		// To retrieve a list of supported extensions before creating an
		// instance
		vkEnumerateInstanceExtensionProperties(nullptr, &m_ExtensionCount,
		                                       nullptr);
		std::vector<VkExtensionProperties> extensions(m_ExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &m_ExtensionCount,
		                                       extensions.data());
		std::cout << "Available extensions:\n";
		for (const auto &extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}
	}

	void pickPhysicalDevice() {
		// This object will be implicitly destroyed when the VkInstance is
		// destroyed, so we won't need to do anything new in the cleanup
		// function.
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error(
			    "Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());
		for (const auto &device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

		//    // Use an ordered map to automatically sort candidates by
		//    increasing score std::multimap<int, VkPhysicalDevice> candidates;
		//
		//    for (const auto &device : devices) {
		//      int score = rateDeviceSuitability(device);
		//      candidates.insert(std::make_pair(score, device));
		//    }
		//
		//    if (candidates.rbegin()->first > 0) {
		//      physicalDevice = candidates.rbegin()->second;
		//    } else {
		//      throw std::runtime_error("Failed to find a suitable GPU!");
		//    }
	}

	int rateDeviceSuitability(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		int score = 0;

		if (deviceProperties.deviceType ==
		    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}

		score += deviceProperties.limits.maxImageDimension2D;

		if (!deviceFeatures.geometryShader) {
			return 0;
		}
		return score;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {
		//    VkPhysicalDeviceProperties deviceProperties;
		//    VkPhysicalDeviceFeatures deviceFeatures;
		//    vkGetPhysicalDeviceProperties(device, &deviceProperties);
		//    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		//
		//    return deviceProperties.deviceType ==
		//               VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		//           deviceFeatures.geometryShader;
		QueueFamilyIndices indices = findQueueFamilies(device);
		return indices.isComplete();
	}

	void setupDebugMessenger() {
		if (!enableValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr,
		                                 &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	bool checkValidationLayerSupport() {
		// 查询系统支持的验证层数量。
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		// 获取系统支持的所有验证层的属性，存储在一个 vector 中。
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// 对于我们应用程序所请求的每一个验证层，都去检查它是否在可用的验证层列表中。
		for (const char *layerName :
		     validationLayers) { // validationLayers
			                     // 是你之前定义的需要用到的验证层列表
			bool layerFound = false;

			// 遍历系统可用的所有验证层
			for (const auto &layerProperties : availableLayers) {
				// 如果找到了与请求的验证层名称相同的，则设置 layerFound 为 true
				// 并跳出循环。
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			// 如果某个请求的验证层没有在可用的列表中找到，则返回 false。
			// 这表示系统不支持这个验证层，我们不能创建一个使用这个验证层的
			// Vulkan 实例。
			if (!layerFound)
				return false;
		}

		// 如果所有请求的验证层都在可用的列表中找到了，则返回
		// true，表明可以使用这些验证层创建 Vulkan 实例。
		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL
	debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	              VkDebugUtilsMessageTypeFlagsEXT messageType,
	              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	              void *pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage
		          << std::endl;
		return VK_FALSE;
	}

	std::vector<const char *> getRequiredExtensions() {
		// 查询GLFW需要的Vulkan扩展数量。
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;

		// 获取GLFW需要的Vulkan扩展名列表。
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		// 将GLFW需要的扩展添加到extensions向量中。这里，构造函数使用两个指针参数（表示数据的开始和结束位置）来初始化vector。
		std::vector<const char *> extensions(
		    glfwExtensions, glfwExtensions + glfwExtensionCount);

		// 如果启用了验证层，则还需要添加一个额外的扩展“VK_EXT_debug_utils”，用于调试支持。
		if (enableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		// 返回包含所有必要扩展名称的向量。
		return extensions;
	}

	// 用于调试和检查vkCreateInstance和vkDestroyInstance调用中的任何问题
	// First extract population of the messenger create info into a separate
	// function :
	// 该函数用来配置与创建Vulkan Debug Messenger相关的参数。
	void populateDebugMessengerCreateInfo(
	    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
		createInfo = {}; // 初始化所有成员为默认值

		// 指定struct的类型，让Vulkan知道这个struct是做什么用的。
		createInfo.sType =
		    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		// 定义触发调试回调的条件: VERBOSE（详细信息）, WARNING（警告）,
		// ERROR（错误）
		createInfo.messageSeverity =
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		// 通过messageType指明哪种类型的事件会出发回调：GENERAL（常规信息）,
		// VALIDATION（验证层的检测结果）, PERFORMANCE（性能相关问题）
		createInfo.messageType =
		    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback =
		    debugCallback; // 设置用户自定义的回调函数，一旦满足上述条件，该回调函数就会被触发。
	}

	// Queue families
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		// 该函数用于寻找支持图形命令的队列家族。
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
		                                         nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
		                                         queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily =
				    i; // 如果当前队列家族支持图形命令，则保存其索引值。
			}
			if (indices.isComplete()) {
				break; // 找到后即可退出。
			}
			i++;
		}
		return indices;
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}