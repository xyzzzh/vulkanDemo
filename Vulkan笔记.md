Part 1 Vulkan学习

1.1 Drawing a triangle

```C++
// 基本框架
class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanUp();
    }

private:
    void initVulkan() {

    }

    void mainLoop() {

    }

    void cleanUp() {

    }

};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

Vulkan objects are either created directly with functions like `vkCreateXXX`, or allocated through another object with
functions like `vkAllocateXXX`. After making sure that an object is no longer used anywhere, you need to destroy it with
the counterparts `vkDestroyXXX` and `vkFreeXXX`. The parameters for these functions generally vary for different types
of objects, but there is one parameter that they all share: `pAllocator`. This is an optional parameter that allows you
to specify callbacks for a custom memory allocator. We will ignore this parameter in the tutorial and always
pass `nullptr` as argument.

**Instance**

Creating an instance

The very first thing you need to do is initialize the Vulkan library by creating an instance. The instance is the
connection between your application and the Vulkan library and creating it involves specifying some details about your
application to the driver.

**队列家族**

物理设备提供队列家族，在 Vulkan 中，物理设备（通常是指图形处理器或 GPU）提供了一个或多个队列家族。每个队列家族都具有一定的特性和能力，例如支持图形命令、计算命令、传输命令等。

这些队列家族和它们的能力是由物理设备（以及其驱动程序）决定的。例如，一块高性能的 GPU
可能会提供多个队列家族，包括专门执行图形命令的队列家族，专门执行计算命令的队列家族，以及专门执行数据传输任务的队列家族等。

Vulkan API 允许开发者查询物理设备支持的所有队列家族及其能力，这样开发者就可以根据自己的需求选择最适合的队列家族进行操作。



### **1.2.1 Window Surface**

Surface在Vulkan中扮演着一个关键角色，它实际上是**Vulkan**与窗口系统之间的接口。
表面对象（surface）代表了实际显示设备的窗口或帧缓冲器，并且可以将其视为**Vulkan**渲染操作的目标。
这样一来，Vulkan就能够向此Surface提交图像，最终将其显示在屏幕上。

```C++
void createSurface() {
		// 使用glfwCreateWindowSurface函数创建窗口表面，如果创建失败则抛出运行时错误
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr,
		                            &m_Surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface!");
		}
	}
```

### **1.2.2 Swap Chain**

**Vulkan**没有**"**默认帧缓冲**"**的概念，因此它需要一个会拥有我们要渲染的缓冲区的基础设施，然后我们再将其可视化到屏幕上。
这个基础设施被称为交换链，必须在**Vulkan**
中显式创建。交换链本质上是一个等待呈现到屏幕的图像队列。我们的应用程序会获取此类图像以进行绘制，然后将其返回到队列中。队列的工作方式以及从队列呈现图像的条件取决于交换链的设置，但交换链的主要目的是同步图像的呈现与屏幕的刷新率。

- 表面格式（颜色深度）
- 呈现模式（将图像**"**切换**"**到屏幕的条件）
- 交换范围（交换链中图像的分辨率）

对于这些设置，我们都会有一个理想的值，如果它可用，我们就会选择它，否则我们将创建一些逻辑来寻找次优选择。

#### [2].1 Surface format

一个物理设备（如**GPU**）可能支持多种不同的表面格式，每种格式都由颜色通道和颜色空间类型的组合定义。这些表面格式定义了你可以在图像和渲染中使用的颜色。

```C++
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
	    const std::vector<VkSurfaceFormatKHR> &availableFormats) {

		for (const auto &availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			    availableFormat.colorSpace ==
			        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return availableFormats[0];
	}
```

#### [2].2 Present Mode

交换链的呈现模式可以说是最重要的设置，因为它代表了将图像显示到屏幕的实际条件。在**Vulkan**中有四种可能的模式:

- **VK_PRESENT_MODE_IMMEDIATE_KHR**：你的应用程序提交的图像会立即被传输到屏幕上，这可能会导致撕裂。

- **VK_PRESENT_MODE_FIFO_KHR**
  ：交换链是一个队列，当显示刷新时，显示器从队列前面取出一个图像，程序在队列后面插入渲染的图像。如果队列已满，那么程序必须等待。这与现代游戏中找到的垂直同步最相似。显示刷新的那一刻被称为**“
  **垂直空白**”**。**
- **VK_PRESENT_MODE_FIFO_RELAXED_KHR**：如果应用程序延迟，并且在最后一个垂直空白时队列为空，那么此模式只与前一个模式有所不同。而不是等待下一个垂直空白，图像在最终到达时立即传输。这可能会导致可见的撕裂。

- **VK_PRESENT_MODE_MAILBOX_KHR**
  ：这是第二种模式的另一种变体。当队列已满时，而不是阻塞应用程序，已经排队的图像简单地被新的图像替换。此模式可用于尽可能快地渲染帧，同时仍然避免撕裂，导致比标准垂直同步更少的延迟问题。这通常被称为**“
  **三重缓冲**”**，尽管存在三个缓冲区并不一定意味着帧率是解锁的。

```C++
	VkPresentModeKHR chooseSwapPresentMode(
	    const std::vector<VkPresentModeKHR> &avaliablePresentModes) {
		for (const auto &avaliablePresentMode : avaliablePresentModes) {
			if (avaliablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return avaliablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}
```

#### [2].3 Swap Extent

交换范围**SwapExtent**通常指的就是交换链图像的宽度和高度，也就是图像分辨率。

```C++
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
```

#### [2].4 创造交换链

除了上述选择的三个参数外，imageArrayLayers指定每个图像由多少层组成。除非你正在开发立体3D应用程序，否则这总是1。imageUsage位字段指定我们将如何使用交换链中的图像进行操作。在本教程中，我们将直接渲染到它们，这意味着它们被用作颜色附件。也有可能你会首先渲染图像到一个独立的图像中，以执行像后处理这样的操作。在那种情况下，你可能会使用类似VK_IMAGE_USAGE_TRANSFER_DST_BIT的值，并使用内存操作将渲染的图像转移到交换链图像。

接下来，我们需要指定如何处理在多个队列家族间使用的交换链图像。如果我们的应用中图形队列家族与显示队列不同，那么就会出现这种情况。我们将在图形队列上绘制交换链图像，然后在显示队列上提交它们。处理从多个队列访问的图像有两种方式：

- `VK_SHARING_MODE_EXCLUSIVE`：一个图像在一次只被一个队列家族拥有，必须显式转移所有权才能在另一个队列家族中使用。此选项提供了最佳性能。
- `VK_SHARING_MODE_CONCURRENT`：图像可在多个队列家族之间使用，无需显式所有权转移。

如果队列家族有差异，那么在本教程中我们将使用并发模式，以避免做所有权章节，因为这涉及到一些概念，更好在后面的时间解释。并发模式要求你提前指定哪些队列家族之间将共享所有权，使用参数`queueFamilyIndexCount`和`pQueueFamilyIndices`进行指定。如果图形队列家族和显示队列家族是相同的（这在大多数硬件上都会是这种情况），那么我们应该坚持使用独占模式，因为并发模式要求你指定至少两个不同的队列家族。

> **队列家族与交换链**
>
> 在Vulkan中，队列家族(queue family)和交换链(swap chain)之间的关系可以理解为操作和资源的关系。
>
> 队列家族是一组可以执行特定类型操作（例如图形渲染、计算任务或提交到显示设备）的队列。每个队列家族都支持特定的操作，并且所有在同一个队列家族中的队列都拥有相同的能力。
>
> 交换链则是一种特殊的图像队列，它用于存储应用程序生成的图像，并将它们按顺序发送给显示设备进行显示。这意味着交换链实质上是由一系列图像（通常称为帧）组成的缓冲区，这些图像会被逐一送入显示设备。
>
> 队列家族和交换链之间的关系就是：队列家族负责对交换链中的图像进行处理。具体来说，图形队列家族可能会负责绘制图像，然后将图像数据提交给交换链；然后，显示队列家族会从交换链中取出图像并将其送入显示设备。
>
> 同时，在Vulkan中，图像的所有权需要在不同的队列家族之间转移。这就引出了你之前提到的两种模式：`VK_SHARING_MODE_EXCLUSIVE`和`VK_SHARING_MODE_CONCURRENT`。使用独占模式时，一个图像在一次只被一个队列家族拥有，如果想要在另一个队列家族中使用，必须显式转移所有权；而使用并发模式时，图像可以在多个队列家族之间使用，无需显式转移所有权。所以，选择何种模式取决于你的应用是否需要在多个队列家族之间共享交换链图像。

> **"图形队列家族和显示队列家族是相同或者不同"，是什么意思？**
>
> 在Vulkan渲染管线中，队列家族(queue families)是负责处理不同工作的单元。例如，一些队列家族可能特化于处理图形渲染任务，而另一些则专注于在显示设备上呈现图像。
>
> - **图形队列家族(Graphics Queue Family)**：这种队列家族主要负责图形渲染操作，例如绘制三维对象、应用纹理等。
> - **显示队列家族(Presentation Queue Family)**：这种队列家族负责将图像从交换链传递到显示设备上。
>
> 通常，一个物理设备(如GPU)可以有多个队列家族，每个都支持不同类型的操作。不过，并非所有设备上的所有队列家族都必须是不同的。在某些硬件上，图形和显示操作可能由同一个队列家族处理，这意味着图形队列家族和显示队列家族是相同的。然而，在其他硬件上，图形和显示操作可能由不同的队列家族处理，即图形队列家族和显示队列家族是不同的。
>
> 所以，"图形队列家族和显示队列家族是相同或者不同"，实际上是指示了你的应用程序在处理图形渲染和图像显示时是否需要切换队列家族。如果它们是相同的，那么你的应用程序可以简化逻辑，使得所有的图形和显示任务都在同一个队列家族中进行。如果它们不同，则需要适当地处理队列家族之间的切换和资源所有权转移。
