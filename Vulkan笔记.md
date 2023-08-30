[TOC]



# 1. Drawing a triangle

## 1.1 Setup

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

## 1.2 Presentation

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

### 1.2.3 Image View

要在渲染管线中使用任何 VkImage，包括交换链中的那些，我们必须创建一个 VkImageView 对象。图像视图Image View实质上就是图像的一个视窗。它描述了如何访问图像以及访问图像的哪个部分，例如，是否应将其视为没有任何 mipmap 层级的2D纹理深度纹理。

每个 VkImage 通常都需要一个对应的 VkImageView。VkImageView 本质上是 VkImage 的一个视图，在渲染过程中它描述了如何访问图像以及访问图像的哪个部分。这使得你可以选择图像的一部分进行操作，或者指定图像的特定用途（例如颜色附件、深度附件等）。这种灵活性使得你在不同的渲染阶段可以以不同的方式查看和使用同一 VkImage。

```C++
// Image View
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo;
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
```



>**VkImageView 与 Surface**
>
>`VkImageView`和`Surface`都是Vulkan中的重要概念，它们各自在图像渲染流程中扮演着重要角色。然而，这两者的功能和使用场景有明显的差异。
>
>### VkImageView
>
>`VkImageView`是对`VkImage`对象的一个视图或者子集。这个视图定义了如何访问图像以及如何将图像解释为特定的像素格式。例如，你可以创建一个视图来显示图像的一个子集、特定的层级、特定的色彩通道等等。`VkImageView`主要用于图像资源的读写操作，比如纹理采样、附件读写等。
>
>### Surface
>
>`Surface`在Vulkan中代表一个平台相关的窗口或者界面，用于显示渲染后的图像。无论你在哪种平台(Windows，Linux，Android等)上运行Vulkan程序，`Surface`都是与特定平台窗口系统交互的桥梁，用于将Vulkan图像内容展示到屏幕上。
>
>### 关系
>
>`VkImageView`和`Surface`之间的关系可以简单概括为：`VkImageView`常用于管理和使用图像数据，而`Surface`则用于将这些图像数据显示到用户的屏幕上。当我们完成一次Vulkan的渲染操作时，必须先通过`VkImageView`将图像资源呈现到一个 `VkFramebuffer`, 然后再通过`Surface`将它们呈现到应用程序窗口中。
>
>换句话说，`VkImageView`与`Surface`一起工作，以实现图像的创建、处理和最终的显示。
>
>希望这能帮助您更好地理解`VkImageView`和`Surface`在Vulkan中的作用和关系。

## 1.3 渲染管线基础

### 1.3.1 引论

![img](https://vulkan-tutorial.com/images/vulkan_simplified_pipeline.svg)

1. 输入装配器从你指定的缓冲区收集原始顶点数据，并可能使用索引缓冲区来重复某些元素，无需复制顶点数据本身。

2. 每个顶点都会运行顶点着色器，通常应用变换将顶点位置从模型空间转换到屏幕空间。它还会将每个顶点的数据传递到管道中。

3. 曲面细分着色器允许你根据某些规则对几何体进行细分，以提高网格质量。这通常用于使砖墙和楼梯等表面在靠近时看起来不那么平坦。

4. 几何着色器在每个图元（三角形，线，点）上运行，并可以丢弃它或输出比进入的图元更多的图元。这与细分着色器相似，但要灵活得多。然而，在现今的应用程序中并没有大量使用，因为除了 Intel 的集成 GPU 外，大多数显卡的性能并不好。

5. 光栅化阶段将图元离散化为片段。这些是他们在帧缓冲区上填充的像素元素。任何落在屏幕外的片段都会被丢弃，顶点着色器输出的属性会在片段中插值，如图所示。通常也会在此处丢弃位于其他图元片段后面的片段，因为有深度测试。

6. 片段着色器会为每个幸存的片段调用，并确定将片段写入哪个帧缓冲器，以及使用哪种颜色和深度值。它可以使用来自顶点着色器的插值数据来实现这一点，这可以包括纹理坐标和用于照明的法线等内容。

7. 颜色混合阶段应用操作以混合映射到帧缓冲器中同一像素的不同片段。片段可以简单地覆盖彼此，或者根据透明度进行加和或混合。

绿色阶段被称为固定功能阶段。这些阶段允许你使用参数调整它们的操作，但它们的工作方式是预先定义的。

另一方面，橙色阶段是可编程的，这意味着你可以向图形卡上传自己的代码，以精确地应用你想要的操作。这允许你使用片段着色器来实现从纹理和照明到光线追踪器等任何东西。这些程序同时在许多 GPU 核心上运行，以并行处理许多对象，例如顶点和片段。

如果你以前使用过 OpenGL 和 Direct3D 等旧API，那么你会习惯于随心所欲地更改任何管道设置，如使用 glBlendFunc 和 OMSetBlendState 等调用。Vulkan 中的图形管道几乎完全是不可变的，所以如果你想要更改着色器，绑定不同的帧缓冲器或更改混合函数，你必须从头开始重新创建管道。缺点是你将不得不创建一些管道，这些管道代表了你在渲染操作中想要使用的所有不同状态的组合。然而，由于你将要在管道中执行的所有操作都是预先知道的，驱动程序可以更好地为其优化。

根据你打算做什么，一些可编程阶段是可选的。例如，如果你只是绘制简单的几何体，可以禁用曲面细分和几何阶段。如果你只对深度值感兴趣，那么你可以禁用片段着色器阶段，这对生成阴影图非常有用。

### 1.3.2 Shader模块

与早期的API不同，Vulkan中的着色器代码必须以字节码格式指定，而不是像GLSL和HLSL那样的人类可读语法。这种字节码格式被称为SPIR-V，旨在与Vulkan和OpenCL（两者均为Khronos API）一起使用。它是一种可以用来编写图形和计算着色器的格式，但在这个教程中，我们将专注于在Vulkan的图形管道中使用的着色器。

使用字节码格式的优点在于，由GPU厂商编写的将着色器代码转换为本地代码的编译器复杂性明显减小。过去的经验表明，对于像GLSL这样的人类可读语法，一些GPU供应商对标准的解释相当灵活。如果你恰好用这些供应商的GPU编写了非平凡的着色器，那么你可能会冒着其他供应商的驱动程序因语法错误拒绝你的代码的风险，甚至更糟糕的情况是，由于编译器错误，你的着色器运行得不一样。有了像SPIR-V这样直接的字节码格式，这种情况希望能够避免。

然而，这并不意味着我们需要手工编写这个字节码。Khronos发布了他们自己的供应商独立的编译器，可以将GLSL编译成SPIR-V。这个编译器设计用来验证你的着色器代码是否完全符合标准，并生成一个你可以与你的程序一起发货的SPIR-V二进制文件。你也可以把这个编译器作为一个库，以在运行时生成SPIR-V，但在这个教程中我们不会这么做。尽管我们可以通过glslangValidator.exe直接使用这个编译器，但我们将使用Google的**glslc.exe**。glslc的优点在于，它使用与GCC和Clang等知名编译器相同的参数格式，并包含一些额外的功能，如includes。它们都已经包含在Vulkan SDK中，所以你不需要下载任何额外的东西。

GLSL是一种具有C风格语法的着色语言。用它编写的程序有一个主函数，每个对象都会调用该函数。GLSL使用全局变量处理输入和输出，而不是使用参数作为输入和返回值作为输出。该语言包含许多用于图形编程的特性，如内置向量和矩阵原语。函数用于操作如叉积、矩阵-向量积和围绕向量的反射等操作。向量类型被称为vec，后面带有表示元素数量的数字。例如，一个3D位置将存储在vec3中。可以通过成员如.x来访问单个组件，也可以同时从多个组件创建新的向量。例如，表达式vec3(1.0, 2.0, 3.0).xy会导致vec2。向量的构造函数也可以接收向量对象和标量值的组合。例如，vec3可以用vec3(vec2(1.0, 2.0), 3.0)构造。

正如前一章所提到的，我们需要编写一个顶点着色器和一个片段着色器才能在屏幕上显示出一个三角形。接下来的两节将介绍每一个着色器的GLSL代码，然后我将向你展示如何生成两个SPIR-V二进制文件并加载到程序中。

#### 1. Vertex Shader顶点着色器

顶点着色器处理每个传入的顶点。它将顶点的属性（如世界位置，颜色，法线和纹理坐标）作为输入。输出是裁剪坐标中的最终位置以及需要传递给片段着色器的属性，如颜色和纹理坐标。然后，这些值将通过光栅化程序在片段上进行插值，以产生平滑的渐变。

裁剪坐标是顶点着色器产生的四维向量，随后将整个向量通过其最后一个分量来转换为归一化设备坐标。这些归一化设备坐标是将帧缓冲区映射到类似于以下的[-1, 1]乘[-1, 1]坐标系统的齐次坐标：

![img](https://vulkan-tutorial.com/images/normalized_device_coordinates.svg)

> 裁剪坐标和归一化设备坐标 (NDC) 是3D图形渲染流程中的两个不同阶段。它们都是在顶点着色阶段和光栅化阶段之间进行的变换过程的一部分。
>
> 1. **裁剪坐标 (Clip Coordinates)**：当顶点位置从世界空间被转换到视图空间，再通过透视投影矩阵转换到裁剪空间时，就得到了裁剪坐标。这个四维向量用于定义视锥体（frustum），也就是我们在屏幕上实际看到的场景的范围。裁剪坐标的w分量可能不等于1，这是因为透视除法还没有发生。
> 2. **归一化设备坐标 (Normalized Device Coordinates, NDC)**：通过透视除法（每个裁剪坐标的x、y、z分量都除以其w分量）得到归一化设备坐标。这是一个齐次坐标系，将视锥体映射到一个立方体，范围通常是[-1, 1]乘[-1, 1]乘[-1, 1]。此时，任何位于这个范围外的顶点都会被剔除或裁剪掉。
>
> 简单来说，裁剪坐标是用来确定哪些部分应该显示在屏幕上，而归一化设备坐标则是用来决定这些部分如何在屏幕上显示。

Vulkan的NDC坐标系与OpenGL不同：

- x轴：**-1**~**1**，**右**为正方向
- y轴：**-1**~**1**，**下**为正方向
- z轴： **0**~**1**，**远**为正方向

#### 2. 使用glslc编译shader为spir-v字节码

```shell
@echo off
setlocal

:: 当前脚本所在目录
set "SCRIPT_DIR=%~dp0"

:: 设置Vulkan SDK路径为环境变量，或者你可以直接将其添加到系统环境变量中
set "VULKAN_SDK=C:/VulkanSDK/1.3.250.1"
:: 路径正规化，去除最后的斜杠（如果存在）
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
"%VULKAN_SDK%\Bin\glslc.exe" "%SCRIPT_DIR%/../shaders/triangle.vert" -o "%SCRIPT_DIR%/../shaders/bin/triangle.vert.spv"
"%VULKAN_SDK%\Bin\glslc.exe" "%SCRIPT_DIR%/../shaders/triangle.frag" -o "%SCRIPT_DIR%/../shaders/bin/triangle.frag.spv"
pause
```

#### 3. 在图形管线中对shader进行载入

载入shader编译生成的spv字节码，然后根据字节码生成对应的顶点ShaderModule和片段ShaderModule，为顶点和片段分别建立shaderStageInfo，填充对应字段，最后声明shaderStages

```C++
VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

**关于`Shader`、`ShaderModule` 和 `ShaderStage`**

在Vulkan中，`Shader`、`ShaderModule` 和 `ShaderStage` 是着色器的三个关键组成部分，他们之间的关系如下：

1. **Shader：** 着色器是一个程序，用于在图形管线的各种阶段执行操作。最常见的类型包括顶点着色器（对顶点数据进行操作）和片段着色器（计算像素颜色）。着色器通常用高级着色语言(HLSL, GLSL等)编写，并编译为字节码。
2. **ShaderModule：** 在Vulkan中，着色器代码（已经被编译为字节码）必须封装在一个`VkShaderModule`对象中。这个对象就像一个容器，存储了着色器的字节码。可以理解为，`ShaderModule` 是用于管理着色器字节码的Vulkan对象。
3. **ShaderStage：** 在Vulkan的渲染管线中，每个阶段都可以使用一个着色器。这些阶段包括顶点处理、几何处理、光栅化、片段处理等。`ShaderStage` 定义了着色器应该在管线的哪个阶段运行。例如，顶点着色器在顶点处理阶段运行，片段着色器在片段处理阶段运行。

因此，你首先要写一个着色器，然后将其编译为字节码并包装在一个`ShaderModule`中。最后，你需要创建一个`ShaderStage`来指定在渲染管线的哪个阶段使用这个`ShaderModule`。

### 1.3.3 固定阶段

早期的图形API为图形管线的大多数阶段提供了默认状态。而在Vulkan中，你需要对大部分的管线状态进行明确指定，因为它会被固化到一个不可变的管线状态对象中。在这一章中，我们将填充所有的结构来配置这些固定功能操作。

#### 1. Dynamic state

虽然大部分的管线状态需要固化到管线状态中，但实际上有限量的状态可以在绘制时间无需重建管线就可以改变。比如视口的大小、线宽和混合常数等。如果你想使用动态状态并保持这些属性不变，那么你需要填充一个VkPipelineDynamicStateCreateInfo结构。

```C++
std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = dynamicStates.size();
dynamicState.pDynamicStates = dynamicStates.data();
```

这将导致对这些值的配置被忽略，而你将有能力（并且需要）在绘制时指定数据。这将产生一个更加灵活的设置，并且对于像视口和剪辑状态这样的东西非常常见，如果它们被固化进管线状态中，会导致设置变得更为复杂。

#### 2. Vertex input

VkPipelineVertexInputStateCreateInfo结构描述了将被传递到顶点着色器的顶点数据的格式。它大致以两种方式描述这一点：

- 绑定：数据之间的间距，以及数据是每个顶点还是每个实例（参见实例化） 

- 属性描述：传递给顶点着色器的属性的类型，从哪个绑定加载它们，以及在哪个偏移位置 因为我们直接在顶点着色器中硬编码顶点数据，所以我们会填充这个结构来指定现在没有顶点数据需要加载。我们将在顶点缓冲区章节中回到这一点。

```C++
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType =
		    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
```



pVertexBindingDescriptions和pVertexAttributeDescriptions成员指向一个描述加载顶点数据的上述细节的结构数组。在shaderStages数组之后，将这个结构添加到createGraphicsPipeline函数中。

#### 3. Input assembly

VkPipelineInputAssemblyStateCreateInfo结构描述了两件事：将从顶点绘制出什么样的几何形状，以及是否应启用基元重启。前者在topology成员中指定，可以有如下值：

- VK_PRIMITIVE_TOPOLOGY_POINT_LIST：从顶点生成的点 
- VK_PRIMITIVE_TOPOLOGY_LINE_LIST：每2个顶点生成一条线，不重复使用 
- VK_PRIMITIVE_TOPOLOGY_LINE_STRIP：每条线的结束顶点用作下一条线的起始顶点 
- VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST：每3个顶点生成一个三角形，不重复使用 
- VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP：每个三角形的第二和第三个顶点用作下一个三角形的前两个顶点 

通常情况下，顶点按照顺序索引从顶点缓冲加载，但是有了元素缓冲器，你可以自己指定要使用的索引。这允许你执行优化操作，如重复使用顶点。如果你将primitiveRestartEnable成员设置为VK_TRUE，那么通过使用0xFFFF或0xFFFFFFFF 的特殊索引，就可以在_STRIP拓扑模式中打断线段和三角形。

#### 4. Viewports and scissors

视口基本上描述了帧缓冲区的输出将被渲染到哪个区域。这几乎总是从（0，0）到（宽度，高度），在本教程中也是如此。

```C++
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = (float) swapChainExtent.width;
viewport.height = (float) swapChainExtent.height;
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

请记住，交换链和其图像的大小可能与窗口的WIDTH和HEIGHT不同。交换链图像稍后将被用作帧缓冲器，所以我们应该坚持使用它们的大小。

minDepth和maxDepth值指定了帧缓冲器使用的深度值范围。这些值必须在[0.0f, 1.0f]范围内，但minDepth可能比maxDepth大。如果你没有做任何特殊的事情，那么你应该坚持使用0.0f和1.0f的标准值。

虽然视口定义了从图像到帧缓冲器的转换，剪刀矩形则定义了像素实际存储的区域。任何位于剪刀矩形外的像素都会被光栅化器丢弃。它们的功能类似于滤镜而非转换。下图说明了区别。注意，左侧的剪刀矩形只是产生那个图像的许多可能性之一，只要它大于视口就可以。

因此，如果我们想要绘制整个帧缓冲器，我们将指定一个完全覆盖它的剪刀矩形：

```CPP
VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapChainExtent;
```

视口（们）和剪刀矩形（们）可以作为管线的静态部分或者作为在命令缓冲中设置的动态状态来指定。虽然前者更符合其他状态，但通常将视口和剪刀状态设为动态更便利，因为这样可以给你更大的灵活性。这是非常常见的，所有的实现都可以处理这种动态状态而不会有性能损失。

实际的视口和剪刀矩形将在后续的绘制时被设置。

有了动态状态，甚至可以在一个命令缓冲中指定不同的视口和/或剪刀矩形。

没有动态状态，视口和剪刀矩形需要通过VkPipelineViewportStateCreateInfo结构在管线中设置。这使得该管线的视口和剪刀矩形变得不可变。对这些值所需的任何更改都将需要用新的值创建一个新的管线。

#### 5. Rasterizer

光栅器接收顶点着色器形成的几何体，将其转变为片段，这些片段由片段着色器进行着色。它还执行深度测试、面剔除和剪刀测试，并可以配置为输出填充整个多边形或仅输出边缘（线框渲染）的片段。所有这些都是通过VkPipelineRasterizationStateCreateInfo结构进行配置的。

```
VkPipelineRasterizationStateCreateInfo rasterizer{};
rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable = VK_FALSE;
```

如果depthClampEnable设置为VK_TRUE，那么超出近平面和远平面的片段将被限制在它们之间，而不是丢弃。这在一些特殊情况下，如阴影映射，是很有用的。使用这个需要启用GPU功能。

```
rasterizer.rasterizerDiscardEnable = VK_FALSE;
```

如果rasterizerDiscardEnable设置为VK_TRUE，那么几何体永远不会通过光栅化阶段。这基本上禁止了任何输出到帧缓冲器。

cpp复制代码rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

polygonMode决定了为几何体生成片段的方式。以下模式可用：

- VK_POLYGON_MODE_FILL：用片段填充多边形的区域
- VK_POLYGON_MODE_LINE：将多边形的边画成线
- VK_POLYGON_MODE_POINT：将多边形的顶点画成点

使用填充以外的任何模式都需要启用GPU功能。

```
rasterizer.lineWidth = 1.0f;
```

lineWidth成员很简单，它描述了线的厚度，以片段数计算。支持的最大线宽取决于硬件，任何大于1.0f的线宽都需要你启用wideLines GPU功能。

```
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
```

cullMode变量确定要使用的面剔除类型。你可以禁用剔除，剔除前面，剔除背面或者两者都剔除。frontFace变量指定被视为正面的面的顶点顺序，可以是顺时针或逆时针。

```
rasterizer.depthBiasEnable = VK_FALSE;
rasterizer.depthBiasConstantFactor = 0.0f; // 可选项
rasterizer.depthBiasClamp = 0.0f; // 可选项
rasterizer.depthBiasSlopeFactor = 0.0f; // 可选项
```

光栅化器可以通过添加一个常数值或根据片段的斜率偏移它们来改变深度值。这有时用于阴影映射，但我们不会使用它。只需将depthBiasEnable设置为VK_FALSE即可。

#### 6. Multisampling

VkPipelineMultisampleStateCreateInfo结构配置了多重采样，这是执行抗锯齿的一种方式。它通过合并多个多边形的片段着色器结果来工作，这些多边形栅格化到同一个像素。这主要发生在边缘，这也是最明显的锯齿状伪影出现的地方。因为如果只有一个多边形映射到一个像素，它不需要运行多次片段着色器，所以它的代价比简单地渲染到更高的解析度然后进行缩小显著得少。启用它需要启用GPU功能。

```C++
VkPipelineMultisampleStateCreateInfo multisampling{};
multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable = VK_FALSE;
multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
multisampling.minSampleShading = 1.0f; // 可选项
multisampling.pSampleMask = nullptr; // 可选项
multisampling.alphaToCoverageEnable = VK_FALSE; // 可选项
multisampling.alphaToOneEnable = VK_FALSE; // 可选项
```

我们将在后面的章节中回顾多重采样，现在让我们保持它处于禁用状态。

#### 7. Color blending

> 为什么需要Color blending?
>
> 在不使用颜色混合的情况下，新的像素颜色会直接替换已经存在的像素颜色，这在大多数场景下是可行的。然而，当我们需要呈现的对象具有**透明或半透明**效果时，就需要考虑旧的像素颜色和新的像素颜色如何组合在一起，这就需要使用颜色混合。

在片段着色器返回一个颜色之后，需要将它与已经在帧缓冲器中的颜色进行组合。这种转换被称为颜色混合，有两种方法可以实现：

- 混合旧的和新的值以产生最终颜色
- 使用位运算组合旧的和新的值

有两种结构可用于配置颜色混合。第一个结构，VkPipelineColorBlendAttachmentState包含每个附加的帧缓冲器的配置，第二个结构，VkPipelineColorBlendStateCreateInfo包含全局颜色混合设置。

#### 8. Pipeline layout

你可以在着色器中使用`uniform`值，它们类似于动态状态变量这样的全局变量，可以在绘制时改变以改变你的着色器行为，而不需要重新创建它们。它们通常用来将变换矩阵传递给顶点着色器，或者在片段着色器中创建纹理采样器。

这些一致性值需要在创建管道时通过创建一个 `VkPipelineLayout` 对象来指定。尽管我们直到未来的章节才会使用它们，但我们仍需要创建一个空的管道布局。

创建一个类成员来保存这个对象，因为我们会在之后的函数中引用它：

```
VkPipelineLayout pipelineLayout;
```

然后在 `createGraphicsPipeline` 函数中创建该对象：

```
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 0; // 可选的
pipelineLayoutInfo.pSetLayouts = nullptr; // 可选的
pipelineLayoutInfo.pushConstantRangeCount = 0; // 可选的
pipelineLayoutInfo.pPushConstantRanges = nullptr; // 可选的

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
}
```

这个结构也指定了推送常数（push constants），它们是将动态值传递给着色器的另一种方式，我们可能会在未来的章节中讨论。管道布局将在程序的整个生命周期中被引用，所以在最后应该销毁：

```
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```

#### 9. 结论 

以上就是所有固定函数状态的全部内容！从头开始设置所有这些确实需要大量工作，但优点是我们现在几乎完全了解图形管道中正在进行的一切！这降低了因为某些组件的默认状态不符合你的预期而遇到意外行为的机会。

然而，在我们最终创建图形管道之前，还有一个对象需要创建，那就是渲染通道。

### 1.3.4 Render Pass

#### 1. Setup

在我们完成管线创建之前，我们需要告诉Vulkan在渲染时会使用哪些帧缓冲附件。我们需要指定将有多少颜色和深度缓冲区，每个缓冲区要使用多少样本，以及如何处理它们在整个渲染操作中的内容。所有这些信息都包装在一个渲染通道对象中，我们将为此创建一个新的createRenderPass函数。在initVulkan中调用此函数，然后再调用createGraphicsPipeline方法。

#### 2. Attachment description

在我们的案例中，我们只有一个由交换链中的图像表示的颜色缓冲区附件。

loadOp 和 storeOp 决定了在渲染之前和渲染之后对附件中的数据进行何种操作。我们对于 loadOp 有以下选择：

- VK_ATTACHMENT_LOAD_OP_LOAD：保留附件中的现有内容 
- VK_ATTACHMENT_LOAD_OP_CLEAR：在开始时清除为常数的值 
- VK_ATTACHMENT_LOAD_OP_DONT_CARE：现有内容未定义；

在我们的案例中，我们将使用 clear 操作在绘制新帧之前将帧缓冲区清除为黑色。storeOp 只有两种可能：

- VK_ATTACHMENT_STORE_OP_STORE：渲染的内容将被存储在内存中，稍后可以读取 
- VK_ATTACHMENT_STORE_OP_DONT_CARE：渲染操作后，帧缓冲区的内容将是未定义的 

我们对在屏幕上看到渲染的三角形感兴趣，所以我们在这里选择了 store 操作。

loadOp 和 storeOp 适用于颜色和深度数据，而 stencilLoadOp / stencilStoreOp 则适用于模板数据。我们的应用程序不会对模板缓冲区做任何操作，所以加载和存储的结果无关紧要。

```C++
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; 
```

在 Vulkan 中，纹理和帧缓冲区由具有某种像素格式的 VkImage 对象表示，然而，基于你对图像进行的操作，内存中的像素布局可能会发生变化。

最常见的一些布局包括：

- VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: 作为颜色附件使用的图像 
- VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: 要在交换链中呈现的图像 
- VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: 作为内存复制操作目标的图像 

我们将在纹理章节中更深入地讨论这个主题，但现在需要知道的重要一点是，图像需要转换到适合它们将参与的下一步操作的特定布局。

initialLayout 指定了渲染通道开始前图像的布局。finalLayout 指定了渲染通道结束时自动过渡到的布局。对于 initialLayout 使用 VK_IMAGE_LAYOUT_UNDEFINED 表示我们不关心图像之前的布局。这个特殊值的注意事项是，不能保证图像的内容被保留，但那没关系，因为我们无论如何都要清除它。我们希望渲染后的图像准备好通过交换链进行呈现，这就是我们使用 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 作为 finalLayout 的原因。

#### 3. Subpasses and attachment references

一个单独的渲染通道可以包含多个子通道。子通道是依赖于前一次过程中帧缓冲区内容的后续渲染操作，例如，一个接一个应用的后处理效果序列。如果你将这些渲染操作分组成一个渲染通道，那么 Vulkan 能够重新排序这些操作，并节省内存带宽，可能提供更好的性能。然而，对于我们的第一个三角形，我们将坚持使用一个子通道。

每个子通道都引用了我们在前面章节中使用结构描述的一个或多个附件。这些引用本身就是 VkAttachmentReference 结构体。

> 在 Vulkan 中，一条渲染管线可以关联多个 render pass。一个 render pass 定义了一系列的渲染操作及其依赖关系。你可以将多个不同的渲染操作组合在一个 render pass 中以实现更有效的资源管理和可能的性能优化。但是，在每一时刻，只有一个 render pass 可以在渲染管线中激活并执行。
>
> 值得注意的是，虽然一个渲染管线可以关联多个 render pass，但**每次 draw call 只能在一个特定的 render pass 上下文中进行**。这意味着，你不能在一个 draw call 中同时使用两个或更多的 render pass。

> **`VkImageView`和`attachment`**
>
> 在Vulkan中，`VkImageView`和附件（attachment）之间的关系非常紧密。
>
> 在渲染过程中，附件是一个用于读取、写入和存储像素数据的抽象概念。每个附件都关联着一个图像资源，这个图像资源实际上就是通过`VkImageView`来表示的。
>
> 当我们创建一个`VkRenderPass`时，需要定义一组附件描述（attachment descriptions），每个附件描述都标识出在渲染管线的哪个阶段使用该附件，以及如何使用它（例如颜色缓冲、深度缓冲等）。然而，这些描述并没有指定具体的数据存储位置，也就是说，它们并没有直接关联到一个实际的图像资源。
>
> 这就是`VkImageView`的作用所在。当我们创建一个`VkFramebuffer`对象时，需要为每个附件提供一个对应的`VkImageView`对象。这样，每个附件就会被绑定到一个实际的图像资源，即`VkImageView`所引用的`VkImage`。
>
> 因此，可以说附件在逻辑上代表了渲染管线中的一个输入/输出位置，而`VkImageView`则提供了访问这个位置所需的具体图像资源的方式。

### 1.3.5 Conclusion

我们现在可以将前面章节的所有结构和对象组合起来，创建图形管线！以下是我们现在拥有的对象类型，做一个快速回顾：

- 着色器阶段：定义图形管线中可编程阶段功能的着色器模块
- 固定函数状态：定义管线中固定函数阶段的所有结构，如输入装配、光栅化器、视区和颜色混合
- 管线布局：由着色器引用并可在绘制时更新的均匀值和推送值
- 渲染通道：被管线阶段引用及其使用的附件

这些组合完全定义了图形管线的功能，因此我们现在可以开始填充位于createGraphicsPipeline函数末尾的VkGraphicsPipelineCreateInfo结构。但在调用vkDestroyShaderModule之前，因为这些在创建过程中仍然需要使用。

实际上还有两个参数：`basePipelineHandle`和`basePipelineIndex`。Vulkan允许你通过从现有的管线派生来创建一个新的图形管线。管线衍生物的理念是，当管线与现有的管线有很多功能相同时，设置管线的成本会更低，且从同一父管线切换也能更快地完成。你可以使用basePipelineHandle指定一个现有管线的句柄，或者使用basePipelineIndex通过索引引用即将创建的另一个管线。目前只有一个管线，所以我们只需指定一个空句柄和一个无效索引。只有在VkGraphicsPipelineCreateInfo的flags字段中也指定了VK_PIPELINE_CREATE_DERIVATIVE_BIT标志时，才会使用这些值。

vkCreateGraphicsPipelines函数实际上比Vulkan中的常用对象创建函数有更多的参数。它被设计为接收多个VkGraphicsPipelineCreateInfo对象，并在一次调用中创建多个VkPipeline对象。

我们传递了VK_NULL_HANDLE参数的第二个参数，引用了一个可选的VkPipelineCache对象。管线缓存可以被用来存储和重用跨多次调用vkCreateGraphicsPipelines，甚至跨程序执行（如果缓存被存储到文件）的管线创建相关数据。这使得在以后的时间大大加速管线创建成为可能。我们将在管线缓存章节中详细讲解这部分内容。

## 1.4 Drawing

### 1.4.1 Framebuffers

在创建渲染通道时指定的附件是通过将它们包装入VkFramebuffer对象来绑定的。一个帧缓存对象引用所有代表附件的VkImageView对象。在我们的情况下，那只会是一个：颜色附件。然而，我们必须使用哪个图像作为附件取决于当我们为展示获取一个图像时，交换链返回的是哪个图像。这意味着我们必须为交换链中的所有图像创建一个帧缓存，并在绘图时使用对应于检索到的图像的那个帧缓存。

如你所见，创建帧缓存相当直接了当。我们首先需要指定帧缓冲区需要与哪个渲染通道兼容。你只能使用与其兼容的渲染通道中的帧缓冲区，这大致意味着它们使用相同数量和类型的附件。

attachmentCount和pAttachments参数指定应绑定到渲染通道pAttachment数组中的各个附件描述的VkImageView对象。

width和height参数不言自明，layers则指的是图像数组中的层数。我们的交换链图像是单一图像，所以层数为1。

我们应该在它们基于的图像视图和渲染通道之前删除帧缓冲区，但只有在我们完成渲染后才能这样做。

> 在Vulkan中，`VkFramebuffer`和`VkImageView`之间有紧密的关系。
>
> `VkImageView`可以视为图像的一个视图或复制品，它描述了如何访问图像以及图像的哪一部分应该被访问。
>
> 而`VkFramebuffer`则是渲染操作的最终目标，它包含一个或多个`VkImageView`对象。每个`VkImageView`都代表了一个附件（attachment），这些附件在渲染管线的不同阶段用于读取、写入和存储像素数据。
>
> 当你创建`VkFramebuffer`时，需要指定与其兼容的`VkRenderPass`，并提供绑定到渲染通道中附件描述的`VkImageView`对象数组。这就意味着，渲染结果将会被写入由`VkFramebuffer`引用的`VkImageView`所表示的附件。因此，具体来说，`VkImageView`定义了怎么样以及在哪里存储像素数据，而`VkFramebuffer`则定义了在哪些`VkImageView`中存储渲染结果。
>
> 因此，可以说`VkFramebuffer`和`VkImageView`在Vulkan的渲染过程中共同扮演了关键角色。
>
> 你可以将`VkFramebuffer`视为指向一组`VkImageView`对象的引用或指针。实际上，`VkFramebuffer`并不直接包含图像数据，而是通过引用`VkImageView`对象来间接访问这些数据。
>
> 在创建`VkFramebuffer`时，你需要提供一个`VkImageView`对象数组，每个`VkImageView`都代表了一个附件（可能是颜色、深度、模板缓冲区等）。当渲染操作发生时，这些附件被用于存储输入和输出的像素数据。
>
> 所以说，`VkFramebuffer`可以看作是一个指针，它指向了执行渲染操作所需的所有附件。但请注意，这只是一个比喻，实际的Vulkan API比这个概念复杂得多。

### 1.4.2 Command buffers

在Vulkan中，诸如绘图操作和内存传输等命令不是直接通过函数调用执行的。你必须将要执行的所有操作记录在命令缓冲区对象中。这样做的好处是，当我们准备告诉 Vulkan 我们想要做什么时，所有的命令一起提交，因为所有的命令都一起可用，Vulkan 可以更有效地处理这些命令。此外，如果需要，这还允许在多个线程中进行命令记录。

#### 1. 命令池 Command Pool

在我们创建命令缓冲区之前，需要先创建一个命令池。命令池负责管理用于存储缓冲区的内存，而命令缓冲区则从其中分配。添加一个新的类成员来存储 VkCommandPool。

`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`和`VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`都是命令池创建标志，但它们的含义和用途有所不同：

1. `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`: 这个标志表示命令缓冲区可能会频繁地被新命令重新记录（rerecorded）。在Vulkan中，你可以把一系列操作（例如绘图或内存传输等）录制到一个命令缓冲区中，然后在需要时执行这些操作。如果你知道某个命令缓冲区会经常被重用（即清空并填入新的命令），那么就可以在创建命令池时使用这个标志，Vulkan可能会根据此标志优化其内部的内存分配策略。
2. `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`: 这个标志允许单独重置命令缓冲区。默认情况下，从同一个命令池分配的所有命令缓冲区必须一起被重置，也就是说，你不能只重置其中的一个，必须重置所有的。如果你设置了这个标志，那么就可以选择性地重置某个特定的命令缓冲区，而不是一口气重置所有的。

这两个标志在某些情况下是很有用的。例如，如果你的应用程序每帧都要录制新的命令（比如因为摄像机移动、物体动画等），那么`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`可能会提高效率。同样，如果你的应用程序需要管理大量的命令缓冲区，并且希望能够灵活地重置它们，那么`VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`就可能派上用场。

通过在设备队列（例如我们获取的图形和演示队列）上提交命令缓冲区来执行它们。每个命令池只能分配在单一类型的队列上提交的命令缓冲区。我们将记录用于绘制的命令，这就是我们选择图形队列族的原因。

#### 2. Command buffer

命令缓冲区在其命令池被销毁时会自动释放，所以我们不需要进行显式的清理。

现在我们将开始编写一个createCommandBuffer函数，从命令池中分配单个命令缓冲区。 命令缓冲区使用vkAllocateCommandBuffers函数进行分配，该函数接受一个VkCommandBufferAllocateInfo结构体作为参数，该结构体指定了命令池和要分配的缓冲区的数量：

level参数指定分配的命令缓冲区是主命令缓冲区还是次级命令缓冲区。

- VK_COMMAND_BUFFER_LEVEL_PRIMARY：可以提交到队列进行执行，但不能从其他命令缓冲区调用。 
- VK_COMMAND_BUFFER_LEVEL_SECONDARY：不能直接提交，但可以从主命令缓冲区调用。 

虽然我们在这里不会使用次级命令缓冲区的功能，但你可以想象它有助于从主命令缓冲区重用常见的操作。由于我们只分配一个命令缓冲区，所以commandBufferCount参数就是1。

#### 3. Command Buffer Recording

现在我们将开始编写recordCommandBuffer函数，该函数将我们想要执行的命令写入命令缓冲区中。使用的VkCommandBuffer将作为参数传入，同时也会传入我们想要写入的当前交换链图像的索引。

我们总是通过调用vkBeginCommandBuffer并使用一个小的VkCommandBufferBeginInfo结构体作为参数来开始记录一个命令缓冲区，该结构体指定了这个特定命令缓冲区使用的一些细节。

flags参数指定了我们将如何使用命令缓冲区。可用的值包括:

- VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT：命令缓冲区将在执行一次后立即重新记录。 
- VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT：这是一个完全在单个渲染通道内部的次级命令缓冲区。 
- VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT：命令缓冲区可以在已经等待执行的同时被重新提交。 

对于我们当前的情况，这些标志都不适用。

pInheritanceInfo参数只对次级命令缓冲区有关系。它指定了从调用的主命令缓冲区继承哪些状态。

如果命令缓冲区已经被记录过一次，那么调用vkBeginCommandBuffer会隐式地重置它。在以后的时间里，无法向缓冲区追加命令。

> 在Vulkan中，“record”是一个非常重要的概念。记录，或者编程，是指将一系列操作（命令）编入一个命令缓冲区中。
>
> 当你"记录"一个命令缓冲区时，你会使用各种Vulkan函数来填充（或记录）该命令缓冲区。这些函数对应的行为可能包括渲染、内存传输、计算等。这些命令不会立即被执行，而是存储在命令缓冲区中以供稍后执行。这样做的好处是可以预先准备多个操作，然后一次性提交给GPU进行处理，从而实现更有效的并行计算和资源管理。
>
> 在这种情况下，"记录"一个命令并不意味着它已经执行——只是说它已经被安排在一个序列中，等待被发送到GPU上。只有当命令缓冲区被提交到命令队列并且达到其在队列中的点时，其中的命令才会被执行。
>
> 因此，理解"记录"的概念就是理解Vulkan如何组织和调度GPU上的工作：你创建并"记录"命令缓冲区，然后在适当的时间将其提交给GPU执行。
>
> 这些命令可能包含各种渲染任务、内存拷贝操作等。这些录入的命令并不会立即执行，而是会被存储在命令缓冲区中，等待后续一次性提交给图形处理器（GPU）进行执行。
>
> 因此，"记录"一个命令缓冲区实际上就是填充或组织这个缓冲区，以便包含一系列将被GPU执行的命令。这种方式允许高效地批量处理操作，并可以优化GPU资源的使用。

**Command Pool, Command Buffer and Record**

> 在Vulkan中，命令池（Command Pool）是用于存储和管理命令缓冲区的对象。你可以将其视为一个命令缓冲区的存储池。
>
> 具体来说，命令缓冲区必须从某个命令池中分配出来，并且当命令池被销毁时，其中的所有命令缓冲区也会被自动释放。这意味着你无需单独管理每个命令缓冲区的生命周期，只需要管理它们所属的命令池即可。
>
> 这里的"record"概念与命令池和命令缓冲区紧密相关。当你“记录”命令到一个命令缓冲区时，你实际上是在使用命令池中分配的空间来存储你想要GPU执行的命令序列。然后，你可以将这个已经“记录”了命令的缓冲区提交给GPU进行处理。
>
> 总的来说，命令池、命令缓冲区和"record"都是Vulkan中命令组织和调度过程的重要组成部分。首先你通过命令池创建并管理命令缓冲区，然后你将一系列命令“记录”到这些命令缓冲区中，最后再将这些缓冲区提交给GPU执行。

#### 4. Starting a render pass

通过vkCmdBeginRenderPass开始渲染通道，开始绘制。渲染通道是使用VkRenderPassBeginInfo结构体中的一些参数来配置的。

```cpp
VkRenderPassBeginInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
renderPassInfo.renderPass = renderPass;
renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
```

首先定义的参数是渲染通道本身和需要绑定的附件。我们为每个交换链图像创建了一个帧缓冲区，并指定它为颜色附件。因此，我们需要绑定我们想要绘制到的交换链图像的帧缓冲区。通过传入的imageIndex参数，我们可以选择当前交换链图像对应的正确的帧缓冲区。

```cpp
renderPassInfo.renderArea.offset = {0, 0};
renderPassInfo.renderArea.extent = swapChainExtent;
```

接下来的两个参数定义了渲染区域的大小。渲染区域定义了着色器加载和存储将在哪里进行。这个区域外的像素将具有未定义的值。为了获得最佳性能，它应该匹配附件的大小。

```cpp
VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
renderPassInfo.clearValueCount = 1;
renderPassInfo.pClearValues = &clearColor;
```

最后两个参数定义了VK_ATTACHMENT_LOAD_OP_CLEAR用的清除值，我们将其作为颜色附件的加载操作。我将清除颜色简单地定义为黑色，不透明度为100%。

```cpp
vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
```

现在，渲染通道可以开始了。所有记录命令的函数都可以通过它们的vkCmd前缀来识别。他们都返回void，所以直到我们完成记录才会有错误处理。

每个命令的第一个参数始终是记录命令的命令缓冲区。第二个参数指定了我们刚刚提供的渲染通道的详细信息。最后一个参数控制渲染通道内的绘图命令如何提供。它可以有以下两个值之一：

- VK_SUBPASS_CONTENTS_INLINE：渲染通道命令将嵌入在主命令缓冲区本身中，不会执行任何次级命令缓冲区。
- VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS：渲染通道命令将从次级命令缓冲区执行。

我们不会使用次级命令缓冲区，所以我们选择第一个选项。

#### 5. Basic drawing commands

现在我们可以绑定图形管线了：

```cpp
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
```

第二个参数指定管线对象是图形管线还是计算管线。我们已经告诉Vulkan在图形管线中执行哪些操作以及在片段着色器中使用哪个附件。

正如在固定函数章节中所提到的，我们确实为这个管线指定了视口和剪裁状态为动态。因此，在发出我们的绘制命令之前，我们需要在命令缓冲区中设置它们：

```cpp
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = static_cast<float>(swapChainExtent.width);
viewport.height = static_cast<float>(swapChainExtent.height);
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapChainExtent;
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
```

现在我们准备好发出绘制三角形的命令了：

```cpp
vkCmdDraw(commandBuffer, 3, 1, 0, 0);
```

实际的vkCmdDraw函数有点anticlimactic（虎头蛇尾的），但由于我们事先指定了所有信息，所以它非常简单。除了命令缓冲区，它还有以下参数：

- vertexCount：尽管我们没有顶点缓冲区，但我们实际上仍然有3个顶点要绘制。
- instanceCount：用于实例渲染，如果你不做这个，使用1。
- firstVertex：用作顶点缓冲区的偏移量，定义gl_VertexIndex的最小值。
- firstInstance：用于实例渲染的偏移量，定义gl_InstanceIndex的最小值。

#### 6. Finishing Up

现在可以结束渲染通道了:

```cpp
vkCmdEndRenderPass(commandBuffer);
```

我们已经完成了命令缓冲区的记录:

```cpp
if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
}
```

在下一章节，我们将编写主循环的代码，该循环将从交换链中获取一个图像，记录并执行一个命令缓冲区，然后将完成的图像返回给交换链。

### 1.4.3 Rendering and presentation

#### 1. Outline of a frame

从高层视角来看，Vulkan中的一帧渲染包含了一套常见的步骤：

- 等待前一帧完成
- 从交换链获取一个图像
- 记录一个绘制场景到该图像上的命令缓冲区
- 提交已记录的命令缓冲区
- 呈现交换链图像

尽管我们会在以后的章节中扩展绘图函数，但这是我们渲染循环的核心。

#### 2. Synchronization

Vulkan的一个核心设计理念是GPU上执行的同步是显式的。操作的顺序由我们使用各种同步原语来定义，这些原语告诉驱动程序我们希望事物按照什么顺序运行。这意味着许多开始在GPU上执行工作的Vulkan API调用都是异步的，这些函数将在操作完成之前返回。

在本章中，我们需要明确排序的事件有很多，因为它们发生在GPU上，例如：

- 从交换链获取一个图像
- 执行将场景绘制到获得的图像上的命令
- 将图像呈现给屏幕，将其返回到交换链

这些事件都是通过单个函数调用启动的，但都是异步执行的。函数调用会在操作实际完成之前返回，执行顺序也是未定义的。这是不幸的，因为每个操作都依赖于前一个操作的完成。因此，我们需要探索可以使用哪些原语来实现所需的排序。

**Semaphores 信号量**

信号量用于添加队列操作之间的顺序。队列操作指的是我们提交给队列的工作，无论是在命令缓冲区中还是在稍后我们将看到的函数中。队列的例子包括图形队列和表示队列。信号量既用于在同一队列内部排序工作，也用于在不同队列之间排序。

Vulkan中恰好有两种类型的信号量，二元信号量和时间轴信号量。因为只有二元信号量将在本教程中使用，我们将不讨论时间轴信号量。进一步提及信号量这个术语专指二元信号量。

信号量要么处于未标记状态，要么处于标记状态。它开始时为未标记状态。我们使用信号量对队列操作进行排序的方式是，在一个队列操作中将相同的信号量作为“信号”信号量提供，在另一个队列操作中作为“等待”信号量提供。例如，假设我们有信号量S和我们想要按顺序执行的队列操作A和B。我们告诉Vulkan的是，操作A在完成执行时将“标记”信号量S，而操作B在开始执行之前将“等待”信号量S。当操作A完成时，信号量S将被标记，而操作B则不会在S被标记之前开始。在操作B开始执行之后，信号量S自动重置为未标记状态，允许它再次使用。

```C++
VkCommandBuffer A, B = ... // record command buffers
VkSemaphore S = ... // create a semaphore

// enqueue A, signal S when done - starts executing immediately
vkQueueSubmit(work: A, signal: S, wait: None)

// enqueue B, wait on S to start
vkQueueSubmit(work: B, signal: None, wait: S)
```

注意，在此代码片段中，对vkQueueSubmit()的两个调用都立即返回 - GPU上只发生等待。CPU继续运行，不会阻塞。要使CPU等待，我们需要一个不同的同步原语，我们将现在描述。

**Fence 围栏**

围栏（Fence）具有类似的目的，它也是用来同步执行，但它是用来排序CPU上的执行，也就是主机。简单地说，如果主机需要知道GPU何时完成了某些工作，我们使用围栏。

与信号量类似，围栏处于标记状态或未标记状态。每当我们提交需要执行的工作时，我们可以将一个围栏附加到该工作上。当工作完成时，围栏会被标记。然后，我们可以让主机等待围栏被标记，保证在主机继续之前工作已经完成。

一个具体的例子是截屏。假设我们已经在GPU上完成了必要的工作。现在需要将图像从GPU传输到主机，然后将内存保存到文件中。我们有命令缓冲区A，用于执行传输和围栏F。我们提交了带有围栏F的命令缓冲区A，然后立即告诉主机等待F标记。这使得主机阻塞，直到命令缓冲区A完成执行。因此，我们可以安全地让主机将文件保存到磁盘，因为内存传输已经完成。

刚刚描述的伪代码：

```cpp
VkCommandBuffer A = ... // 记录包含传输的命令缓冲区
VkFence F = ... // 创建围栏

// 入队A，立即开始工作，完成时标记F
vkQueueSubmit(work: A, fence: F)

vkWaitForFence(F) // 阻塞执行，直到A完成执行

save_screenshot_to_disk() // 不能运行，直到传输完成
```

与信号量示例不同，此示例确实阻塞了主机执行。这意味着除了等待执行结束外，主机不会做任何事情。对于这种情况，我们必须确保在我们能将截图保存到磁盘之前，传输已经完成。

一般来说，除非必要，最好不要阻塞主机。我们希望为GPU和主机提供有用的工作。等待围栏标记并不是有用的工作。因此，我们更喜欢使用信号量，或其他尚未涵盖的同步原语来同步我们的工作。

围栏必须手动重置以将其恢复到未标记状态。这是因为围栏被用来控制主机的执行，所以主机可以决定何时重置围栏。相比之下，信号量被用来在GPU上排序工作，而不涉及主机。

总的来说，信号量用于指定GPU上操作的执行顺序，而围栏用于使CPU和GPU保持同步。

> 信号量（Semaphore）和围栏（Fence）都是用于同步操作的原语，但它们的使用场景和目标有所不同。
>
> - 信号量：主要用于GPU与GPU之间的同步。例如，在两个不同的队列操作（可能是不同的GPU任务）之间添加顺序性。当一个操作完成时，它会发出（signal）一个信号量，然后另一个操作在开始之前等待（wait）这个信号量。这样可以保证操作的执行顺序。信号量主要关注GPU上的操作顺序，对于CPU来说，基本是透明的。
> - 围栏：主要用于CPU与GPU之间的同步。当我们提交一个任务到GPU执行，并且需要知道何时完成时，我们会使用一个围栏。任务完成后，围栏被标记为已信号状态，然后我们可以让CPU等待这个围栏变为已信号状态。这样可以确保在CPU继续之前，GPU的工作已经完成。因此，围栏主要关注主机（CPU）与设备（GPU）之间的同步。
>
> 总结一下，虽然这两种同步原语都可以使操作有序进行，但信号量主要用于控制在设备（GPU）内部的操作顺序，而围栏主要用于同步设备（GPU）和主机（CPU）的操作。

**Choosing which?**

我们有两种同步原语可用，并且方便地应用于两个需要同步的地方：交换链操作和等待前一帧完成。我们希望使用信号量进行交换链操作，因为它们在GPU上进行，因此，如果可能的话，我们不希望让主机等待。对于等待前一帧完成，我们希望使用围栏，原因正好相反，因为我们需要主机等待。这样我们就不会同时绘制超过一帧。因为我们每帧都重新记录命令缓冲区，所以在当前帧完成执行之前，我们不能记录下一帧的工作到命令缓冲区，因为我们不想在GPU使用命令缓冲区时覆盖它的当前内容。

#### 3. Creating the synchronization objects

我们需要一个信号量来表示已经从交换链获取了图像并准备好进行渲染，另一个信号量来表示渲染已经完成可以进行呈现，并有一个围栏来确保一次只渲染一帧。

创建三个类成员来存储这些信号量对象和围栏对象：

```cpp
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;
```

创建信号量，我们将添加本教程此部分的最后一个创建函数：createSyncObjects:

```cpp
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
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

...

void createSyncObjects() {

}
```

创建信号量需要填写VkSemaphoreCreateInfo，但在API的当前版本中，它实际上没有除sType之外的任何必填字段：

```cpp
void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
```

未来的Vulkan API版本或扩展可能会为flags和pNext参数添加功能，就像其他结构体一样。

创建围栏需要填写VkFenceCreateInfo：

```cpp
VkFenceCreateInfo fenceInfo{};
fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
```

使用vkCreateSemaphore & vkCreateFence按照熟悉的模式创建信号量和围栏：

```cpp
if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to create semaphores!");
}
```

在程序结束时，所有命令都已完成并且不再需要同步时，应清理信号量和围栏：

```cpp
void cleanup() {
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);
}
```

接下来是主要的绘图函数！

#### 4. Waiting for the previous frame

在帧开始时，我们需要**等待前一帧完成**，以便命令缓冲和信号量可以使用。为此，我们调用vkWaitForFences：

```cpp
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
}
```

vkWaitForFences函数接受一个围栏数组，并在主机上等待任何或所有围栏被标记为已完成状态后才返回。这里我们传递的VK_TRUE表示我们想要等待所有围栏，但在单个围栏的情况下并不重要。此函数还有一个超时参数，我们将其设置为64位无符号整数的最大值UINT64_MAX，这实际上禁用了超时。

等待之后，我们需要手动用vkResetFences调用将围栏复位到未标记状态：

```cpp
vkResetFences(device, 1, &inFlightFence);
```

在我们可以继续之前，我们的设计中出现了一个小问题。在第一帧我们调用drawFrame()，它立即等待inFlightFence被标记。只有在一帧渲染完成后，inFlightFence才会被标记，但由于这是第一帧，所以没有前一帧可以标记围栏！因此，vkWaitForFences()无限阻塞，等待永远不会发生的事情。

对于这个难题，有许多解决方案，API内置了一个巧妙的应对方法。创建一个已经标记为完成的围栏，这样第一次调用vkWaitForFences()就会立即返回，因为围栏已经被标记。

为此，我们在VkFenceCreateInfo中添加VK_FENCE_CREATE_SIGNALED_BIT标志：

```cpp
void createSyncObjects() {
    ...

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ...
}
```

#### 5. Acquiring an image from the swap chain

在drawFrame函数中，我们需要做的下一件事就是从交换链获取图像。回想一下，交换链是一个扩展特性，所以我们必须使用带有vk*KHR命名约定的函数：

```cpp
void drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}
```

vkAcquireNextImageKHR的前两个参数是逻辑设备和我们希望获取图像的交换链。第三个参数指定一个纳秒级的超时时间，以便图像变得可用。使用64位无符号整数的最大值意味着我们有效地禁用了超时。

接下来的两个参数指定了当显示引擎完成图像使用时，将被标记的同步对象。那是我们可以开始绘制图像的时间点。这里可以指定信号量、围栏或者两者都有。我们将在这里使用我们的imageAvailableSemaphore。

最后一个参数指定一个变量来输出已经可用的交换链图像的索引。该索引是指我们swapChainImages数组中的VkImage。我们将使用该索引来选择VkFramebuffer。

#### 6. Recording the command buffer

有了指定要使用的交换链图像的imageIndex，我们现在可以记录命令缓存。首先，我们在命令缓存上调用vkResetCommandBuffer，以确保其能够被记录。

```cpp
vkResetCommandBuffer(commandBuffer, 0);
```

vkResetCommandBuffer的第二个参数是一个VkCommandBufferResetFlagBits标志。由于我们不想做任何特殊的事情，我们将其保留为0。

现在调用函数recordCommandBuffer来记录我们想执行的命令。

```cpp
recordCommandBuffer(commandBuffer, imageIndex);
```

有了完全记录的命令缓存，我们现在可以提交它。

#### 7. Submitting the command buffer

队列提交和同步通过VkSubmitInfo结构中的参数进行配置。

```cpp
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores = waitSemaphores;
submitInfo.pWaitDstStageMask = waitStages;
```
前三个参数指定在执行开始之前应等待哪些信号量以及在管道的哪个阶段等待。我们希望在图像可用之前等待写入颜色，所以我们指定写入颜色附件的图形管道的阶段。这意味着理论上，实现可以在图像还不可用时就开始执行我们的顶点着色器等任务。waitStages数组中的每个条目对应pWaitSemaphores中具有相同索引的信号量。

```cpp
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffer;
```
下两个参数指定实际提交执行的命令缓冲区。我们直接提交我们拥有的单个命令缓冲区。

```cpp
VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = signalSemaphores;
```
signalSemaphoreCount和pSignalSemaphores参数指定了命令缓冲区完成执行后要发出信号的信号量。在我们的情况下，我们使用renderFinishedSemaphore用于此目的。

```cpp
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
}
```
我们现在可以使用vkQueueSubmit将命令缓冲区提交给图形队列。该函数将一组VkSubmitInfo结构作为参数，当工作负载更大时，这种方式更高效。最后一个参数引用一个可选围栏，在命令缓冲区完成执行时会发出信号。这使我们能知道何时可以安全地重复使用命令缓冲区，因此我们想要将其赋值给inFlightFence。现在在下一帧中，CPU会等待此命令缓冲区完成执行后再记录新的命令进入其中。

#### 8. Subpass dependencies

请记住，渲染过程中的子通道会自动处理图像布局转换。这些转换由子通道依赖来控制，这些依赖指定了子通道之间的内存和执行依赖关系。我们现在只有一个子通道，但是在这个子通道之前和之后的操作也被视为隐式的"子通道"。

有两个内置的依赖关系负责在渲染过程开始和结束时的转换，但前者并未在正确的时间发生。它假设转换发生在管线的开始，但我们在那个时候还没有获取到图像！处理这个问题有两种方式。我们可以将imageAvailableSemaphore的waitStages更改为VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT来确保在图像可用之前不开始渲染过程，或者我们可以让渲染过程等待VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT阶段。我决定在这里使用第二种选项，因为这是一个很好的理由去查看子通道依赖以及他们如何工作。

子通道依赖项在VkSubpassDependency结构体中指定。去createRenderPass函数并添加一个：

```cpp
VkSubpassDependency dependency{};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
dependency.dstSubpass = 0;
```
前两个字段指定了依赖关系和依赖子通道的索引。特殊值VK_SUBPASS_EXTERNAL根据其在srcSubpass或dstSubpass中的指定，分别指代渲染过程之前或之后的隐式子通道。索引0则指我们的子通道，它是唯一且排在首位的子通道。dstSubpass必须始终高于srcSubpass，以防止依赖图中的循环（除非其中一个子通道是VK_SUBPASS_EXTERNAL）。

```cpp
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;
```
接下来的两个字段指定了要等待的操作以及这些操作发生的阶段。我们需要等待交换链完成从图像中读取的操作，然后我们才能访问它。通过等待颜色附件输出阶段本身，可以实现这一点。

```cpp
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```
应该等待这个阶段的操作处于颜色附件阶段，并涉及写入颜色附件。这些设置将防止在实际需要（并允许）时转换发生：当我们希望开始向其写入颜色时。

```cpp
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```
VkRenderPassCreateInfo结构体有两个字段用于指定依赖项数组。

#### 9. Presentation

绘制帧的最后一步是将结果提交回交换链，以便它最终显示在屏幕上。展示是通过drawFrame函数末尾的VkPresentInfoKHR结构进行配置的。

```cpp
VkPresentInfoKHR presentInfo{};
presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

presentInfo.waitSemaphoreCount = 1;
presentInfo.pWaitSemaphores = signalSemaphores;
```
前两个参数指定在展示可以发生之前要等待的信号量，就像VkSubmitInfo一样。由于我们想等待命令缓冲区完成执行，从而绘制出我们的三角形，我们接收将被信号化并等待它们的信号量，因此我们使用signalSemaphores。

```cpp
VkSwapchainKHR swapChains[] = {swapChain};
presentInfo.swapchainCount = 1;
presentInfo.pSwapchains = swapChains;
presentInfo.pImageIndices = &imageIndex;
```
接下来的两个参数指定要向其展示图像的交换链和每个交换链的图像索引。这几乎总是单一的。

```cpp
presentInfo.pResults = nullptr; // Optional
```
还有一个叫做pResults的可选参数。它允许你指定一个VkResult值数组，用于检查每个独立的交换链是否成功展示。如果你只使用一个交换链，那么这不是必需的，因为你可以简单地使用呈现函数的返回值。

```cpp
vkQueuePresentKHR(presentQueue, &presentInfo);
```
vkQueuePresentKHR函数提交了向交换链展示一个图像的请求。我们将在下一章为vkAcquireNextImageKHR和vkQueuePresentKHR添加错误处理，因为他们的失败并不一定意味着程序应该终止，与我们迄今为止看到的函数不同。

如果你到目前为止做得都正确，那么当你运行你的程序时，你应该可以看到以下内容：

![img](https://vulkan-tutorial.com/images/triangle.png)

这个彩色的三角形可能看起来和你在图形教程中看到的有些不同。这是因为本教程让着色器在线性颜色空间中插值，并在之后转换为sRGB颜色空间。请参见这篇博客文章，以了解这两者之间的区别。

太棒了！不幸的是，你会发现当启用验证层时，程序在你关闭它的时候就崩溃了。debugCallback打印到终端的消息告诉我们原因：

![img](https://vulkan-tutorial.com/images/semaphore_in_use.png)

请记住，drawFrame中的所有操作都是异步进行的。这意味着当我们退出mainLoop的循环时，绘图和展示操作可能仍在进行中。在此过程中清理资源是个坏主意。

要解决这个问题，我们应该等待逻辑设备完成操作，然后退出mainLoop并销毁窗口：

```cpp
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}
```
你也可以等待特定命令队列中的操作完成，使用vkQueueWaitIdle。这些函数可以作为一种非常基础的方式来执行同步。你会发现当关闭窗口时，程序现在没有问题地退出了。

#### 10. Conclusion

900多行代码后，我们终于看到了画面弹出在屏幕上！启动一个Vulkan程序确实需要大量的工作，但需要取走的信息是，通过其明确性，Vulkan给予你巨大的控制能力。我建议你现在花些时间重新阅读代码，并建立一个关于程序中所有Vulkan对象的目的以及它们如何相互关联的心理模型。我们将在此基础上扩展程序的功能。

下一章将扩展渲染循环，以处理多帧。

### 1.4.4 Frames in flight

目前我们的渲染循环有一个明显的缺陷。在渲染下一帧之前，我们需要等待上一帧完成，这导致了主机的不必要空闲。

解决这个问题的方法是允许同时有多个帧在飞行中，也就是说，允许一个帧的渲染不干扰下一个帧的记录。那么我们如何做到这一点呢？任何在渲染过程中被访问和修改的资源都必须被复制。因此，我们需要多个命令缓冲区、信号量和栅栏。在后续章节中，我们还会添加其他资源的多个实例，所以我们会再次看到这个概念。

首先，在程序的顶部添加一个常数，定义应该同时处理多少帧：

```c++
const int MAX_FRAMES_IN_FLIGHT = 2;
```
我们选择数字2，因为我们不希望CPU超前于GPU太多。有2个飞行中的帧时，CPU和GPU可以同时进行各自的任务。如果CPU提前结束，它将等待GPU完成渲染，然后提交更多工作。对于3个或更多的飞行中的帧，CPU可能超过GPU，增加了帧延迟。通常，额外的延迟是不希望的。但是，让应用程序控制飞行中的帧数量是Vulkan明确性的另一个例子。

每一帧都应该有自己的命令缓冲区、信号量集和栅栏。重命名并改变它们为对象的std::vectors：

```c++
std::vector<VkCommandBuffer> commandBuffers;

...

std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
```
接着，我们需要创建多个命令缓冲区。将createCommandBuffer重命名为createCommandBuffers。接下来我们需要调整命令缓冲区向量的大小为MAX_FRAMES_IN_FLIGHT的大小，改变VkCommandBufferAllocateInfo使其包含那么多命令缓冲区，然后改变目标为我们的命令缓冲区向量：

```c++
void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    ...
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}
```
createSyncObjects函数应被改变以创建所有的对象：

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
```
同样的，它们也应该全部被清理：

```c++
void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    ...
}
```
记住，因为当我们释放命令池时，命令缓冲区会为我们自动释放，所以我们没有额外的命令缓冲区清理工作要做。

为了在每个帧使用正确的对象，我们需要跟踪当前的帧。我们将使用一个帧索引来实现这一点：

```c++
uint32_t currentFrame = 0;
```
现在可以修改drawFrame函数，使用正确的对象：

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    vkResetCommandBuffer(commandBuffers[currentFrame],  0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    ...

    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    ...

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    ...

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    ...

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
}
```
当然，我们不应该忘记每次都进入下一个帧：

```c++
void drawFrame() {
    ...

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```
通过使用模运算符（%），我们确保在每个MAX_FRAMES_IN_FLIGHT队列帧之后帧索引都会循环。

我们现在已经实现了所有需要的同步，以确保队列中的工作帧不超过MAX_FRAMES_IN_FLIGHT，并且这些帧不会相互干扰。请注意，像最后的清理这样的代码其他部分，依赖于更粗糙的同步，如vkDeviceWaitIdle，是没问题的。你应该根据性能需求决定使用哪种方法。

要通过例子了解更多关于同步的信息，请查看Khronos的这个详尽的概述。

在下一章中，我们将处理一个规范的Vulkan程序所需的另一个小事情。
