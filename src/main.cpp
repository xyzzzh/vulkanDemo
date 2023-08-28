#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};
struct SwapChainSupportDetails {
	// 基本表面能力（交换链中图像的最小/最大数量，图像的最小/最大宽度和高度）
	VkSurfaceCapabilitiesKHR capabilities;
	// 表面格式（像素格式，颜色空间）
	std::vector<VkSurfaceFormatKHR> formats;
	// 可用的显示模式
	std::vector<VkPresentModeKHR> presentModes;
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
	// m_Window 是一个指向 GLFWwindow 对象的指针，它代表了程序创建的窗口。
	GLFWwindow *m_Window;
	// m_Instance 代表了程序的 Vulkan 实例。Vulkan 实例是 Vulkan
	// 程序的基础，大部分 Vulkan 功能都需要它。
	VkInstance m_Instance;
	// m_ExtensionCount 用于存储程序所需的 Vulkan 扩展数量。
	uint32_t m_ExtensionCount = 0;
	// debugMessenger 是一个 Vulkan Debug Utils Messenger 对象，用于处理 Vulkan
	// 的调试和错误消息。
	VkDebugUtilsMessengerEXT debugMessenger;
	// physicalDevice 是一个表示物理设备的对象，在这里我们初始化为
	// VK_NULL_HANDLE。物理设备表示GPU设备，在 Vulkan
	// 中，我们需要从所有可用的物理设备中选择一个进行操作。
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	// m_Device 是一个 Vulkan
	// 设备（Device）对象，它代表了一个物理设备上的逻辑设备， 程序中所有的
	// Vulkan 操作，如创建图像缓冲、提交命令等，都需要通过这个逻辑设备进行。
	VkDevice m_Device;

	// 这是一个handle，用来存储图形队列。
	VkQueue graphicsQueue;

	// 这是一个Vulkan表面对象，用于呈现渲染结果到窗口。
	VkSurfaceKHR m_Surface;

	// 这是一个handle，用来存储呈现队列。
	VkQueue presentQueue;

	// 用于Swap Chain
	VkSwapchainKHR m_SwapChain; // 交换链句柄，用于管理应用程序的图像呈现操作
	std::vector<VkImage> swapChainImages; // 存储交换链中的VkImage对象的数组
	VkFormat swapChainImageFormat; // 交换链图像的格式
	VkExtent2D swapChainExtent;    // 交换链图像的尺寸

	// 存储Image View
	std::vector<VkImageView> swapChainImageViews;

	// Pipeline Layout
	VkPipelineLayout m_PipelineLayout;

	// Render pass
	VkRenderPass m_RenderPass;

	// Graphics pipeline
	VkPipeline m_GraphicsPipeline;

	// Framebuffer
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	void initWindow() {
		// 初始化GLFW库
		glfwInit();

		// 因为GLFW最初是设计来创建OpenGL上下文的，所以我们需要告诉它不要创建OpenGL上下文
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// 禁用窗口调整大小的功能
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		// 创建一个新的GLFW窗口
		m_Window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_Window)) {
			glfwPollEvents();
		}
	}

	void cleanUp() {
		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(m_Device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		// Logical devices don't interact directly with instances, which is why
		// it's not included as a parameter.
		vkDestroyDevice(m_Device, nullptr);
		if (enableValidationLayers) {
			DestoryDebugUtilsMessengerEXT(m_Instance, debugMessenger, nullptr);
		}
		// Make sure that the surface is destroyed before the instance.
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void createInstance() {
		// 检查是否启用了验证层，如果启用了但没有可用的验证层，则抛出运行时错误
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error(
			    "validation layer requested, but not available!");
		}

		// 创建一个VkApplicationInfo结构体，填充我们的应用信息
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// 创建一个VkInstanceCreateInfo结构体，该结构体将包含创建Vulkan实例所需的所有信息
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// 获取并设置要启用的扩展列表
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount =
		    static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// 创建一个VkDebugUtilsMessengerCreateInfoEXT，用于存储调试信使的创建信息
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

		// 通过vkCreateInstance函数创建Vulkan实例，如果创建失败则抛出运行时错误
		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	void checkExtensionSupport() {
		// 检索支持的扩展列表的数量
		vkEnumerateInstanceExtensionProperties(nullptr, &m_ExtensionCount,
		                                       nullptr);

		// 创建一个足够容纳所有扩展的向量
		std::vector<VkExtensionProperties> extensions(m_ExtensionCount);

		// 获取所有支持的扩展列表
		vkEnumerateInstanceExtensionProperties(nullptr, &m_ExtensionCount,
		                                       extensions.data());

		// 打印可用的扩展
		std::cout << "Available extensions:\n";
		for (const auto &extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}
	}

	void pickPhysicalDevice() {
		// 查询系统中支持Vulkan的物理设备数量
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		// 如果没有任何支持Vulkan的物理设备，则抛出运行时错误
		if (deviceCount == 0) {
			throw std::runtime_error(
			    "Failed to find GPUs with Vulkan support!");
		}

		// 创建一个足以容纳所有设备的向量，并获取设备列表
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		// 遍历所有设备，找到第一个符合我们要求的设备
		for (const auto &device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		// 如果没有找到符合要求的设备，则抛出运行时错误
		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU!");
		}
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
		// 获取设备的队列家族信息
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		if (extensionSupported) {
			SwapChainSupportDetails swapChainSupportDetails =
			    querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupportDetails.formats.empty() &&
			                    !swapChainSupportDetails.presentModes.empty();
		}
		// 如果这个设备的所有队列家族都完整，那么我们认为这个设备是适合的
		return indices.isComplete() && extensionSupported && swapChainAdequate;
	}

	void setupDebugMessenger() {
		// 如果没有启用验证层，则直接返回
		if (!enableValidationLayers)
			return;

		// 创建一个VkDebugUtilsMessengerCreateInfoEXT结构体，并填充创建调试信使所需的信息
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		// 使用CreateDebugUtilsMessengerEXT函数创建调试信使，如果创建失败则抛出运行时错误
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

	// 检查设备拓展支持
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;

		// 获取设备扩展数量
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
		                                     nullptr);

		// 创建一个向量用于存储可用的扩展属性
		std::vector<VkExtensionProperties> avaliableExtensions(extensionCount);

		// 获取所有可用的设备扩展
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
		                                     avaliableExtensions.data());

		// 创建一个集合，包含所需要的所有扩展。
		std::set<std::string> requiredExtensions(deviceExtensions.begin(),
		                                         deviceExtensions.end());

		// 遍历所有可用扩展，如果此扩展在所需扩展中，则从所需扩展集合中移除该扩展
		for (const auto &extension : avaliableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		// 如果所有所需的扩展都被找到并从集合中移除，那么集合应为空，返回值为true
		return requiredExtensions.empty();
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
	// Queue Family
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		// 创建一个QueueFamilyIndices结构体，用于存储找到的队列家族索引
		QueueFamilyIndices indices;

		// 获取设备支持的队列家族数量
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
		                                         nullptr);

		// 创建一个向量来存储所有队列家族，并获取队列家族列表
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
		                                         queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			// 检查当前队列家族是否支持图形命令
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			// 检查当前队列家族是否支持呈现操作
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface,
			                                     &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			// 如果已经找到了支持图形命令和呈现操作的队列家族，则退出循环
			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	// Logical device and queues
	void createLogicalDevice() {
		// 查找支持所需队列家族的物理设备
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// 初始化用于存储设备队列创建信息的向量，并创建一个包含所有独特队列家族的集合
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
		    indices.graphicsFamily.value(), indices.presentFamily.value()};

		// 设置队列优先级并为每个独特队列家族创建VkDeviceQueueCreateInfo结构体
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// 初始化物理设备特性结构体
		VkPhysicalDeviceFeatures deviceFeatures{};

		// 配置设备创建信息
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount =
		    static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount =
		    static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// 当启用验证层时，设定启用的验证层数量和名称
		if (enableValidationLayers) {
			createInfo.enabledLayerCount =
			    static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			// 若未启用验证层，则设定启用的验证层数量为0
			createInfo.enabledLayerCount = 0;
		}

		// 创建逻辑设备，若创建失败则抛出运行时错误
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_Device) !=
		    VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device!");
		}

		// 获取设备队列的引用，这将用于图形和计算命令的提交
		vkGetDeviceQueue(m_Device, indices.graphicsFamily.value(), 0,
		                 &graphicsQueue);
		vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0,
		                 &presentQueue);
	}

	// 1.2.1 Window Surface
	// Surface在Vulkan中扮演着一个关键角色，它实际上是Vulkan与窗口系统之间的接口。
	// 表面对象（surface）代表了实际显示设备的窗口或帧缓冲器，并且可以将其视为Vulkan渲染操作的目标。
	// 这样一来，Vulkan就能够向此Surface提交图像，最终将其显示在屏幕上。
	void createSurface() {
		// 使用glfwCreateWindowSurface函数创建窗口表面，如果创建失败则抛出运行时错误
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr,
		                            &m_Surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	// 1.2.2 Swap Chain
	// Vulkan没有"默认帧缓冲"的概念，因此它需要一个会拥有我们要渲染的缓冲区的基础设施，然后我们再将其可视化到屏幕上。
	// 这个基础设施被称为交换链，必须在Vulkan中显式创建。交换链本质上是一个等待呈现到屏幕的图像队列。
	// 我们的应用程序会获取此类图像以进行绘制，然后将其返回到队列中。
	// 队列的工作方式以及从队列呈现图像的条件取决于交换链的设置，但交换链的主要目的是同步图像的呈现与屏幕的刷新率。

	// [1]. 查询是否支持Swap Chain
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface,
		                                          &details.capabilities);
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount,
		                                     nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(
			    device, m_Surface, &formatCount, details.formats.data());
		}
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface,
		                                          &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(
			    device, m_Surface, &presentModeCount,
			    details.presentModes.data());
		}
		return details;
	}

	// [2]. 选择交换链的正确设置
	// 如果满足了swapChainAdequate条件，那么支持肯定是足够的，但可能仍然存在许多不同模式的优化程度不一。
	// 我们现在将编写几个函数来找到最好的可能的交换链的正确设置。有三种类型的设置需要确定：
	// - 表面格式（颜色深度）
	// - 呈现模式（将图像"切换"到屏幕的条件）
	// - 交换范围（交换链中图像的分辨率）
	// 对于这些设置，我们都会有一个理想的值，如果它可用，我们就会选择它，否则我们将创建一些逻辑来寻找次优选择。

	// [2].1 Surface format
	// 一个物理设备（如GPU）可能支持多种不同的表面格式，每种格式都由颜色通道和颜色空间类型的组合定义。这些表面格式定义了你可以在图像和渲染中使用的颜色。
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
	    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
		// 每个VkSurfaceFormatKHR条目都包含一个格式和一个颜色空间成员。
		// - 格式成员指定颜色通道和类型。
		// - 颜色空间成员使用VK_COLOR_SPACE_SRGB_NONLINEAR_KHR标志,
		//   指示其是否支持SRGB颜色空间。
		for (const auto &availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			    availableFormat.colorSpace ==
			        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return availableFormats[0];
	}
	// [2].2 Present Mode
	// 交换链的呈现模式可以说是最重要的设置，因为它代表了将图像显示到屏幕的实际条件。在Vulkan中有四种可能的模式:
	// -VK_PRESENT_MODE_IMMEDIATE_KHR：你的应用程序提交的图像会立即被传输到屏幕上，这可能会导致撕裂。
	// -VK_PRESENT_MODE_FIFO_KHR：交换链是一个队列，当显示刷新时，显示器从队列前面取出一个图像，程序在队列后面插入渲染的图像。如果队列已满，那么程序必须等待。这与现代游戏中找到的垂直同步最相似。显示刷新的那一刻被称为“垂直空白”。
	// -VK_PRESENT_MODE_FIFO_RELAXED_KHR：如果应用程序延迟，并且在最后一个垂直空白时队列为空，那么此模式只与前一个模式有所不同。而不是等待下一个垂直空白，图像在最终到达时立即传输。这可能会导致可见的撕裂。
	// -VK_PRESENT_MODE_MAILBOX_KHR：这是第二种模式的另一种变体。当队列已满时，而不是阻塞应用程序，已经排队的图像简单地被新的图像替换。此模式可用于尽可能快地渲染帧，同时仍然避免撕裂，导致比标准垂直同步更少的延迟问题。这通常被称为“三重缓冲”，尽管存在三个缓冲区并不一定意味着帧率是解锁的。
	VkPresentModeKHR chooseSwapPresentMode(
	    const std::vector<VkPresentModeKHR> &avaliablePresentModes) {
		for (const auto &avaliablePresentMode : avaliablePresentModes) {
			if (avaliablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return avaliablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// [2].3 Swap extent
	// 交换范围SwapExtent通常指的就是交换链图像的宽度和高度，也就是图像分辨率。
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
		if (capabilities.currentExtent.width !=
		    std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(m_Window, &width, &height);

			VkExtent2D actualExtent = {static_cast<uint32_t>(width),
			                           static_cast<uint32_t>(height)};

			actualExtent.width = std::clamp(actualExtent.width,
			                                capabilities.minImageExtent.width,
			                                capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(
			    actualExtent.height, capabilities.minImageExtent.height,
			    capabilities.maxImageExtent.height);
			return actualExtent;
		}
	}

	// Create Swap Chain
	void createSwapChain() {
		// 查询物理设备对交换链的支持情况
		SwapChainSupportDetails swapChainSupport =
		    querySwapChainSupport(physicalDevice);

		// 选择表面格式
		VkSurfaceFormatKHR surfaceFormat =
		    chooseSwapSurfaceFormat(swapChainSupport.formats);

		// 选择呈现模式
		VkPresentModeKHR presentMode =
		    chooseSwapPresentMode(swapChainSupport.presentModes);

		// 选择交换范围
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// 根据设备能力确定图像数量
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
		    imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// 设置交换链创建信息
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// 查找队列家族
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
		                                 indices.presentFamily.value()};

		// 如果图形家族与显示家族不同，则使用并发共享模式；如果相同，则使用独占共享模式。
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		// 设置其他创建信息
		createInfo.preTransform =
		    swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_Device, &createInfo, nullptr,
		                         &m_SwapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain!");
		}

		// 保存交换链中VkImages的handle
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount,
		                        swapChainImages.data());

		// 保存交换链图像的格式及范围
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// Image View
	// 为每个VkImage创建对应的VkImageView
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			// VK_COMPONENT_SWIZZLE_IDENTITY
			// 表示我们不会改变原始通道的顺序，也就是默认的映射
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// MIP 映射级别
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			// 要访问的图像层数
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			if (vkCreateImageView(m_Device, &createInfo, nullptr,
			                      &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image views!");
			}
		}
	}

	// Graphics Pipeline
	// Loading a shader
	static std::vector<char> readFile(const std::string &filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open " + filename);
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	// Create Shader Module
	VkShaderModule createShaderModule(const std::vector<char> &code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_Device, &createInfo, nullptr,
		                         &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module!");
		}
		return shaderModule;
	}

	void createGraphicsPipeline() {
		auto vertShaderCode = readFile("../shaders/bin/triangle.vert.spv");
		auto fragShaderCode = readFile("../shaders/bin/triangle.frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
		                                                  fragShaderStageInfo};

		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
		                                             VK_DYNAMIC_STATE_SCISSOR};

		// dynamic state
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = dynamicStates.size();
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		// input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		// Color blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr,
		                           &m_PipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1,
		                              &pipelineInfo, nullptr,
		                              &m_GraphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(m_Device, fragShaderModule, nullptr);
		vkDestroyShaderModule(m_Device, vertShaderModule, nullptr);
	}

	// Render Pass
	void createRenderPass() {
		// 定义颜色附件描述符
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat; // 设置图像的格式
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 设置多重采样数量
		colorAttachment.loadOp =
		    VK_ATTACHMENT_LOAD_OP_CLEAR; // 加载操作为清除操作
		colorAttachment.storeOp =
		    VK_ATTACHMENT_STORE_OP_STORE; // 存储操作为保存渲染后的图像
		colorAttachment.stencilLoadOp =
		    VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 模板加载操作为不关心
		colorAttachment.stencilStoreOp =
		    VK_ATTACHMENT_STORE_OP_DONT_CARE; // 模板存储操作为不关心
		colorAttachment.initialLayout =
		    VK_IMAGE_LAYOUT_UNDEFINED; // 初始布局为未定义
		colorAttachment.finalLayout =
		    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // 最终布局为可直接用于显示的布局

		// 定义颜色附件引用
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment =
		    0; // 指定要引用哪个附件（在本例中只有一个，即colorAttachment）
		colorAttachmentRef.layout =
		    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 指定附件布局为优化的颜色附件布局

		// 定义子通道描述符
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint =
		    VK_PIPELINE_BIND_POINT_GRAPHICS; // 将管线绑定点设置为图形
		subpass.colorAttachmentCount = 1;    // 设置颜色附件计数为1
		subpass.pColorAttachments =
		    &colorAttachmentRef; // 设置颜色附件数组的指针为colorAttachmentRef

		// 创建渲染通道的信息
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType =
		    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; // 设置结构类型为渲染通道创建信息
		renderPassInfo.attachmentCount = 1; // 设置附件计数为1
		renderPassInfo.pAttachments =
		    &colorAttachment; // 设置附件描述符的指针为colorAttachment
		renderPassInfo.subpassCount = 1; // 设置子通道计数为1
		renderPassInfo.pSubpasses = &subpass; // 设置子通道描述符的指针为subpass

		// 如果不能成功创建渲染通道，则抛出运行时错误
		if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr,
		                       &m_RenderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	// Framebuffer
	void createFramebuffers() {
		m_SwapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = {swapChainImageViews[i]};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr,
			                        &m_SwapChainFramebuffers[i]) !=
			    VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
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