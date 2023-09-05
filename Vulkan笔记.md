# Vulkan Note

By: *xy.zhu* & *GPT 4.0*

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

### 1.2.1 Window Surface

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

### 1.2.2 Swap Chain

**Vulkan**没有**"**默认帧缓冲**"**的概念，因此它需要一个会拥有我们要渲染的缓冲区的基础设施，然后我们再将其可视化到屏幕上。
这个基础设施被称为交换链，必须在**Vulkan**中显式创建。交换链本质上是一个等待呈现到屏幕的图像队列。我们的应用程序会获取此类图像以进行绘制，然后将其返回到队列中。队列的工作方式以及从队列呈现图像的条件取决于交换链的设置，但交换链的主要目的是同步图像的呈现与屏幕的刷新率。

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
  ：交换链是一个队列，当显示刷新时，显示器从队列前面取出一个图像，程序在队列后面插入渲染的图像。如果队列已满，那么程序必须等待。这与现代游戏中找到的垂直同步最相似。显示刷新的那一刻被称为“**垂直空白**”**。**
- **VK_PRESENT_MODE_FIFO_RELAXED_KHR**：如果应用程序延迟，并且在最后一个垂直空白时队列为空，那么此模式只与前一个模式有所不同。而不是等待下一个垂直空白，图像在最终到达时立即传输。这可能会导致可见的撕裂。

- **VK_PRESENT_MODE_MAILBOX_KHR**
  ：这是第二种模式的另一种变体。当队列已满时，而不是阻塞应用程序，已经排队的图像简单地被新的图像替换。此模式可用于尽可能快地渲染帧，同时仍然避免撕裂，导致比标准垂直同步更少的延迟问题。这通常被称为”**三重缓冲**”，尽管存在三个缓冲区并不一定意味着帧率是解锁的。

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

#### [2].4 Creating the swap chain

除了上述选择的三个参数外，imageArrayLayers指定每个图像由多少层组成。除非你正在开发立体3D应用程序，否则这总是1。imageUsage位字段指定我们将如何使用交换链中的图像进行操作。在本教程中，我们将直接渲染到它们，这意味着它们被用作颜色附件。也有可能你会首先渲染图像到一个独立的图像中，以执行像后处理这样的操作。在那种情况下，你可能会使用类似VK_IMAGE_USAGE_TRANSFER_DST_BIT的值，并使用内存操作将渲染的图像转移到交换链图像。

接下来，我们需要指定如何处理在多个队列家族间使用的交换链图像。如果我们的应用中图形队列家族与显示队列不同，那么就会出现这种情况。我们将在图形队列上绘制交换链图像，然后在显示队列上提交它们。处理从多个队列访问的图像有两种方式：

- `VK_SHARING_MODE_EXCLUSIVE`：一个图像在一次只被一个队列家族拥有，必须显式转移所有权才能在另一个队列家族中使用。此选项提供了最佳性能。
- `VK_SHARING_MODE_CONCURRENT`：图像可在多个队列家族之间使用，无需显式所有权转移。

如果队列家族有差异，那么在本教程中我们将使用并发模式，以避免做所有权章节，因为这涉及到一些概念，更好在后面的时间解释。并发模式要求你提前指定哪些队列家族之间将共享所有权，使用参数`queueFamilyIndexCount`
和`pQueueFamilyIndices`进行指定。如果图形队列家族和显示队列家族是相同的（这在大多数硬件上都会是这种情况），那么我们应该坚持使用独占模式，因为并发模式要求你指定至少两个不同的队列家族。

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
> 同时，在Vulkan中，图像的所有权需要在不同的队列家族之间转移。这就引出了你之前提到的两种模式：`VK_SHARING_MODE_EXCLUSIVE`
> 和`VK_SHARING_MODE_CONCURRENT`
>
。使用独占模式时，一个图像在一次只被一个队列家族拥有，如果想要在另一个队列家族中使用，必须显式转移所有权；而使用并发模式时，图像可以在多个队列家族之间使用，无需显式转移所有权。所以，选择何种模式取决于你的应用是否需要在多个队列家族之间共享交换链图像。

> **"图形队列家族和显示队列家族是相同或者不同"，是什么意思？**
>
> 在Vulkan渲染管线中，队列家族(queue families)是负责处理不同工作的单元。例如，一些队列家族可能特化于处理图形渲染任务，而另一些则专注于在显示设备上呈现图像。
>
> - **图形队列家族(Graphics Queue Family)**：这种队列家族主要负责图形渲染操作，例如绘制三维对象、应用纹理等。
> - **显示队列家族(Presentation Queue Family)**：这种队列家族负责将图像从交换链传递到显示设备上。
>
> 通常，一个物理设备(如GPU)
>
可以有多个队列家族，每个都支持不同类型的操作。不过，并非所有设备上的所有队列家族都必须是不同的。在某些硬件上，图形和显示操作可能由同一个队列家族处理，这意味着图形队列家族和显示队列家族是相同的。然而，在其他硬件上，图形和显示操作可能由不同的队列家族处理，即图形队列家族和显示队列家族是不同的。
>
> 所以，"图形队列家族和显示队列家族是相同或者不同"
>
，实际上是指示了你的应用程序在处理图形渲染和图像显示时是否需要切换队列家族。如果它们是相同的，那么你的应用程序可以简化逻辑，使得所有的图形和显示任务都在同一个队列家族中进行。如果它们不同，则需要适当地处理队列家族之间的切换和资源所有权转移。

### 1.2.3 Image View

要在渲染管线中使用任何 VkImage，包括交换链中的那些，我们必须创建一个 VkImageView 对象。图像视图Image
View实质上就是图像的一个视窗。它描述了如何访问图像以及访问图像的哪个部分，例如，是否应将其视为没有任何 mipmap 层级的2D纹理深度纹理。

每个 VkImage 通常都需要一个对应的 VkImageView。VkImageView 本质上是 VkImage
的一个视图，在渲染过程中它描述了如何访问图像以及访问图像的哪个部分。这使得你可以选择图像的一部分进行操作，或者指定图像的特定用途（例如颜色附件、深度附件等）。这种灵活性使得你在不同的渲染阶段可以以不同的方式查看和使用同一
VkImage。

```C++
// Image View
	void createImageViews() {
		m_SwapChainImageViews.resize(swapChainImages.size());
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
			                      &m_SwapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image views!");
			}
		}
	}
```

> **VkImageView 与 Surface**
>
>`VkImageView`和`Surface`都是Vulkan中的重要概念，它们各自在图像渲染流程中扮演着重要角色。然而，这两者的功能和使用场景有明显的差异。
>
>### VkImageView
>
>`VkImageView`是对`VkImage`
> 对象的一个视图或者子集。这个视图定义了如何访问图像以及如何将图像解释为特定的像素格式。例如，你可以创建一个视图来显示图像的一个子集、特定的层级、特定的色彩通道等等。`VkImageView`
> 主要用于图像资源的读写操作，比如纹理采样、附件读写等。
>
>### Surface
>
>`Surface`在Vulkan中代表一个平台相关的窗口或者界面，用于显示渲染后的图像。无论你在哪种平台(Windows，Linux，Android等)
> 上运行Vulkan程序，`Surface`都是与特定平台窗口系统交互的桥梁，用于将Vulkan图像内容展示到屏幕上。
>
>### 关系
>
>`VkImageView`和`Surface`之间的关系可以简单概括为：`VkImageView`常用于管理和使用图像数据，而`Surface`
> 则用于将这些图像数据显示到用户的屏幕上。当我们完成一次Vulkan的渲染操作时，必须先通过`VkImageView`
> 将图像资源呈现到一个 `VkFramebuffer`, 然后再通过`Surface`将它们呈现到应用程序窗口中。
>
>换句话说，`VkImageView`与`Surface`一起工作，以实现图像的创建、处理和最终的显示。
>
>希望这能帮助您更好地理解`VkImageView`和`Surface`在Vulkan中的作用和关系。

## 1.3 Graphics pipeline basics

### 1.3.1 Introduction

![img](https://vulkan-tutorial.com/images/vulkan_simplified_pipeline.svg)

1. 输入装配器从你指定的缓冲区收集原始顶点数据，并可能使用索引缓冲区来重复某些元素，无需复制顶点数据本身。

2. 每个顶点都会运行顶点着色器，通常应用变换将顶点位置从模型空间转换到屏幕空间。它还会将每个顶点的数据传递到管道中。

3. 曲面细分着色器允许你根据某些规则对几何体进行细分，以提高网格质量。这通常用于使砖墙和楼梯等表面在靠近时看起来不那么平坦。

4. 几何着色器在每个图元（三角形，线，点）上运行，并可以丢弃它或输出比进入的图元更多的图元。这与细分着色器相似，但要灵活得多。然而，在现今的应用程序中并没有大量使用，因为除了
   Intel 的集成 GPU 外，大多数显卡的性能并不好。

5. 光栅化阶段将图元离散化为片段。这些是他们在帧缓冲区上填充的像素元素。任何落在屏幕外的片段都会被丢弃，顶点着色器输出的属性会在片段中插值，如图所示。通常也会在此处丢弃位于其他图元片段后面的片段，因为有深度测试。

6. 片段着色器会为每个幸存的片段调用，并确定将片段写入哪个帧缓冲器，以及使用哪种颜色和深度值。它可以使用来自顶点着色器的插值数据来实现这一点，这可以包括纹理坐标和用于照明的法线等内容。

7. 颜色混合阶段应用操作以混合映射到帧缓冲器中同一像素的不同片段。片段可以简单地覆盖彼此，或者根据透明度进行加和或混合。

绿色阶段被称为固定功能阶段。这些阶段允许你使用参数调整它们的操作，但它们的工作方式是预先定义的。

另一方面，橙色阶段是可编程的，这意味着你可以向图形卡上传自己的代码，以精确地应用你想要的操作。这允许你使用片段着色器来实现从纹理和照明到光线追踪器等任何东西。这些程序同时在许多
GPU 核心上运行，以并行处理许多对象，例如顶点和片段。

如果你以前使用过 OpenGL 和 Direct3D 等旧API，那么你会习惯于随心所欲地更改任何管道设置，如使用 glBlendFunc 和
OMSetBlendState 等调用。Vulkan
中的图形管道几乎完全是不可变的，所以如果你想要更改着色器，绑定不同的帧缓冲器或更改混合函数，你必须从头开始重新创建管道。缺点是你将不得不创建一些管道，这些管道代表了你在渲染操作中想要使用的所有不同状态的组合。然而，由于你将要在管道中执行的所有操作都是预先知道的，驱动程序可以更好地为其优化。

根据你打算做什么，一些可编程阶段是可选的。例如，如果你只是绘制简单的几何体，可以禁用曲面细分和几何阶段。如果你只对深度值感兴趣，那么你可以禁用片段着色器阶段，这对生成阴影图非常有用。

### 1.3.2 Shader modules

与早期的API不同，Vulkan中的着色器代码必须以字节码格式指定，而不是像GLSL和HLSL那样的人类可读语法。这种字节码格式被称为SPIR-V，旨在与Vulkan和OpenCL（两者均为Khronos
API）一起使用。它是一种可以用来编写图形和计算着色器的格式，但在这个教程中，我们将专注于在Vulkan的图形管道中使用的着色器。

使用字节码格式的优点在于，由GPU厂商编写的将着色器代码转换为本地代码的编译器复杂性明显减小。过去的经验表明，对于像GLSL这样的人类可读语法，一些GPU供应商对标准的解释相当灵活。如果你恰好用这些供应商的GPU编写了非平凡的着色器，那么你可能会冒着其他供应商的驱动程序因语法错误拒绝你的代码的风险，甚至更糟糕的情况是，由于编译器错误，你的着色器运行得不一样。有了像SPIR-V这样直接的字节码格式，这种情况希望能够避免。

然而，这并不意味着我们需要手工编写这个字节码。Khronos发布了他们自己的供应商独立的编译器，可以将GLSL编译成SPIR-V。这个编译器设计用来验证你的着色器代码是否完全符合标准，并生成一个你可以与你的程序一起发货的SPIR-V二进制文件。你也可以把这个编译器作为一个库，以在运行时生成SPIR-V，但在这个教程中我们不会这么做。尽管我们可以通过glslangValidator.exe直接使用这个编译器，但我们将使用Google的**glslc.exe**。glslc的优点在于，它使用与GCC和Clang等知名编译器相同的参数格式，并包含一些额外的功能，如includes。它们都已经包含在VulkanSDK中，所以你不需要下载任何额外的东西。

GLSL是一种具有C风格语法的着色语言。用它编写的程序有一个主函数，每个对象都会调用该函数。GLSL使用全局变量处理输入和输出，而不是使用参数作为输入和返回值作为输出。该语言包含许多用于图形编程的特性，如内置向量和矩阵原语。函数用于操作如叉积、矩阵-向量积和围绕向量的反射等操作。向量类型被称为vec，后面带有表示元素数量的数字。例如，一个3D位置将存储在vec3中。可以通过成员如.x来访问单个组件，也可以同时从多个组件创建新的向量。例如，表达式vec3(1.0, 2.0, 3.0).xy会导致vec2。向量的构造函数也可以接收向量对象和标量值的组合。例如，vec3可以用vec3(vec2(1.0, 2.0), 3.0)构造。

正如前一章所提到的，我们需要编写一个顶点着色器和一个片段着色器才能在屏幕上显示出一个三角形。接下来的两节将介绍每一个着色器的GLSL代码，然后我将向你展示如何生成两个SPIR-V二进制文件并加载到程序中。

#### 1. Vertex Shader

顶点着色器处理每个传入的顶点。它将顶点的属性（如世界位置，颜色，法线和纹理坐标）作为输入。输出是裁剪坐标中的最终位置以及需要传递给片段着色器的属性，如颜色和纹理坐标。然后，这些值将通过光栅化程序在片段上进行插值，以产生平滑的渐变。

裁剪坐标是顶点着色器产生的四维向量，随后将整个向量通过其最后一个分量来转换为归一化设备坐标。这些归一化设备坐标是将帧缓冲区映射到类似于以下的[-1, 1]
乘[-1, 1]坐标系统的齐次坐标：

![img](https://vulkan-tutorial.com/images/normalized_device_coordinates.svg)

> 裁剪坐标和归一化设备坐标 (NDC) 是3D图形渲染流程中的两个不同阶段。它们都是在顶点着色阶段和光栅化阶段之间进行的变换过程的一部分。
>
> 1. **裁剪坐标 (Clip Coordinates)**
     ：当顶点位置从世界空间被转换到视图空间，再通过透视投影矩阵转换到裁剪空间时，就得到了裁剪坐标。这个四维向量用于定义视锥体（frustum），也就是我们在屏幕上实际看到的场景的范围。裁剪坐标的w分量可能不等于1，这是因为透视除法还没有发生。
> 2. **归一化设备坐标 (Normalized Device Coordinates, NDC)**
     ：通过透视除法（每个裁剪坐标的x、y、z分量都除以其w分量）得到归一化设备坐标。这是一个齐次坐标系，将视锥体映射到一个立方体，范围通常是[-1, 1]
     乘[-1, 1]乘[-1, 1]。此时，任何位于这个范围外的顶点都会被剔除或裁剪掉。
>
> 简单来说，裁剪坐标是用来确定哪些部分应该显示在屏幕上，而归一化设备坐标则是用来决定这些部分如何在屏幕上显示。

Vulkan的NDC坐标系与OpenGL不同：

- x轴：**-1**~**1**，**右**为正方向
- y轴：**-1**~**1**，**下**为正方向
- z轴： **0**~**1**，**远**为正方向

#### 2. Compiling the shaders

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

#### 3. Loading a shader

载入shader编译生成的spv字节码，然后根据字节码生成对应的顶点ShaderModule和片段ShaderModule，为顶点和片段分别建立shaderStageInfo，填充对应字段，最后声明shaderStages

```C++
VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

**关于`Shader`、`ShaderModule` 和 `ShaderStage`**

在Vulkan中，`Shader`、`ShaderModule` 和 `ShaderStage` 是着色器的三个关键组成部分，他们之间的关系如下：

1. **Shader：**
   着色器是一个程序，用于在图形管线的各种阶段执行操作。最常见的类型包括顶点着色器（对顶点数据进行操作）和片段着色器（计算像素颜色）。着色器通常用高级着色语言(
   HLSL, GLSL等)编写，并编译为字节码。
2. **ShaderModule：** 在Vulkan中，着色器代码（已经被编译为字节码）必须封装在一个`VkShaderModule`
   对象中。这个对象就像一个容器，存储了着色器的字节码。可以理解为，`ShaderModule` 是用于管理着色器字节码的Vulkan对象。
3. **ShaderStage：**
   在Vulkan的渲染管线中，每个阶段都可以使用一个着色器。这些阶段包括顶点处理、几何处理、光栅化、片段处理等。`ShaderStage`
   定义了着色器应该在管线的哪个阶段运行。例如，顶点着色器在顶点处理阶段运行，片段着色器在片段处理阶段运行。

因此，你首先要写一个着色器，然后将其编译为字节码并包装在一个`ShaderModule`中。最后，你需要创建一个`ShaderStage`
来指定在渲染管线的哪个阶段使用这个`ShaderModule`。

### 1.3.3 Fixed functions

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

- 属性描述：传递给顶点着色器的属性的类型，从哪个绑定加载它们，以及在哪个偏移位置
  因为我们直接在顶点着色器中硬编码顶点数据，所以我们会填充这个结构来指定现在没有顶点数据需要加载。我们将在顶点缓冲区章节中回到这一点。

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

通常情况下，顶点按照顺序索引从顶点缓冲加载，但是有了元素缓冲器，你可以自己指定要使用的索引。这允许你执行优化操作，如重复使用顶点。如果你将primitiveRestartEnable成员设置为VK_TRUE，那么通过使用0xFFFF或0xFFFFFFFF
的特殊索引，就可以在_STRIP拓扑模式中打断线段和三角形。

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

minDepth和maxDepth值指定了帧缓冲器使用的深度值范围。这些值必须在[0.0f, 1.0f]
范围内，但minDepth可能比maxDepth大。如果你没有做任何特殊的事情，那么你应该坚持使用0.0f和1.0f的标准值。

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

lineWidth成员很简单，它描述了线的厚度，以片段数计算。支持的最大线宽取决于硬件，任何大于1.0f的线宽都需要你启用wideLines
GPU功能。

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
> 在不使用颜色混合的情况下，新的像素颜色会直接替换已经存在的像素颜色，这在大多数场景下是可行的。然而，当我们需要呈现的对象具有
**透明或半透明**效果时，就需要考虑旧的像素颜色和新的像素颜色如何组合在一起，这就需要使用颜色混合。

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

#### 9. Conclusion

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

initialLayout 指定了渲染通道开始前图像的布局。finalLayout 指定了渲染通道结束时自动过渡到的布局。对于 initialLayout 使用
VK_IMAGE_LAYOUT_UNDEFINED 表示我们不关心图像之前的布局。这个特殊值的注意事项是，不能保证图像的内容被保留，但那没关系，因为我们无论如何都要清除它。我们希望渲染后的图像准备好通过交换链进行呈现，这就是我们使用
VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 作为 finalLayout 的原因。

#### 3. Subpasses and attachment references

一个单独的渲染通道可以包含多个子通道。子通道是依赖于前一次过程中帧缓冲区内容的后续渲染操作，例如，一个接一个应用的后处理效果序列。如果你将这些渲染操作分组成一个渲染通道，那么
Vulkan 能够重新排序这些操作，并节省内存带宽，可能提供更好的性能。然而，对于我们的第一个三角形，我们将坚持使用一个子通道。

每个子通道都引用了我们在前面章节中使用结构描述的一个或多个附件。这些引用本身就是 VkAttachmentReference 结构体。

> 在 Vulkan 中，一条渲染管线可以关联多个 render pass。一个 render pass 定义了一系列的渲染操作及其依赖关系。你可以将多个不同的渲染操作组合在一个
> render pass 中以实现更有效的资源管理和可能的性能优化。但是，在每一时刻，只有一个 render pass 可以在渲染管线中激活并执行。
>
> 值得注意的是，虽然一个渲染管线可以关联多个 render pass，但**每次 draw call 只能在一个特定的 render pass 上下文中进行**
> 。这意味着，你不能在一个 draw call 中同时使用两个或更多的 render pass。

> **`VkImageView`和`attachment`**
>
> 在Vulkan中，`VkImageView`和附件（attachment）之间的关系非常紧密。
>
>
在渲染过程中，附件是一个用于读取、写入和存储像素数据的抽象概念。每个附件都关联着一个图像资源，这个图像资源实际上就是通过`VkImageView`
来表示的。
>
> 当我们创建一个`VkRenderPass`时，需要定义一组附件描述（attachment
> descriptions），每个附件描述都标识出在渲染管线的哪个阶段使用该附件，以及如何使用它（例如颜色缓冲、深度缓冲等）。然而，这些描述并没有指定具体的数据存储位置，也就是说，它们并没有直接关联到一个实际的图像资源。
>
> 这就是`VkImageView`的作用所在。当我们创建一个`VkFramebuffer`对象时，需要为每个附件提供一个对应的`VkImageView`
> 对象。这样，每个附件就会被绑定到一个实际的图像资源，即`VkImageView`所引用的`VkImage`。
>
> 因此，可以说附件在逻辑上代表了渲染管线中的一个输入/输出位置，而`VkImageView`则提供了访问这个位置所需的具体图像资源的方式。

### 1.3.5 Conclusion

我们现在可以将前面章节的所有结构和对象组合起来，创建图形管线！以下是我们现在拥有的对象类型，做一个快速回顾：

- 着色器阶段：定义图形管线中可编程阶段功能的着色器模块
- 固定函数状态：定义管线中固定函数阶段的所有结构，如输入装配、光栅化器、视区和颜色混合
- 管线布局：由着色器引用并可在绘制时更新的均匀值和推送值
- 渲染通道：被管线阶段引用及其使用的附件

这些组合完全定义了图形管线的功能，因此我们现在可以开始填充位于createGraphicsPipeline函数末尾的VkGraphicsPipelineCreateInfo结构。但在调用vkDestroyShaderModule之前，因为这些在创建过程中仍然需要使用。

实际上还有两个参数：`basePipelineHandle`和`basePipelineIndex`
。Vulkan允许你通过从现有的管线派生来创建一个新的图形管线。管线衍生物的理念是，当管线与现有的管线有很多功能相同时，设置管线的成本会更低，且从同一父管线切换也能更快地完成。你可以使用basePipelineHandle指定一个现有管线的句柄，或者使用basePipelineIndex通过索引引用即将创建的另一个管线。目前只有一个管线，所以我们只需指定一个空句柄和一个无效索引。只有在VkGraphicsPipelineCreateInfo的flags字段中也指定了VK_PIPELINE_CREATE_DERIVATIVE_BIT标志时，才会使用这些值。

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
> 而`VkFramebuffer`则是渲染操作的最终目标，它包含一个或多个`VkImageView`对象。每个`VkImageView`
> 都代表了一个附件（attachment），这些附件在渲染管线的不同阶段用于读取、写入和存储像素数据。
>
> 当你创建`VkFramebuffer`时，需要指定与其兼容的`VkRenderPass`，并提供绑定到渲染通道中附件描述的`VkImageView`
> 对象数组。这就意味着，渲染结果将会被写入由`VkFramebuffer`引用的`VkImageView`所表示的附件。因此，具体来说，`VkImageView`
> 定义了怎么样以及在哪里存储像素数据，而`VkFramebuffer`则定义了在哪些`VkImageView`中存储渲染结果。
>
> 因此，可以说`VkFramebuffer`和`VkImageView`在Vulkan的渲染过程中共同扮演了关键角色。
>
> 你可以将`VkFramebuffer`视为指向一组`VkImageView`对象的引用或指针。实际上，`VkFramebuffer`
> 并不直接包含图像数据，而是通过引用`VkImageView`对象来间接访问这些数据。
>
> 在创建`VkFramebuffer`时，你需要提供一个`VkImageView`对象数组，每个`VkImageView`
> 都代表了一个附件（可能是颜色、深度、模板缓冲区等）。当渲染操作发生时，这些附件被用于存储输入和输出的像素数据。
>
> 所以说，`VkFramebuffer`可以看作是一个指针，它指向了执行渲染操作所需的所有附件。但请注意，这只是一个比喻，实际的Vulkan
> API比这个概念复杂得多。

### 1.4.2 Command buffers

在Vulkan中，诸如绘图操作和内存传输等命令不是直接通过函数调用执行的。你必须将要执行的所有操作记录在命令缓冲区对象中。这样做的好处是，当我们准备告诉
Vulkan 我们想要做什么时，所有的命令一起提交，因为所有的命令都一起可用，Vulkan 可以更有效地处理这些命令。此外，如果需要，这还允许在多个线程中进行命令记录。

#### 1. 命令池 Command Pool

在我们创建命令缓冲区之前，需要先创建一个命令池。命令池负责管理用于存储缓冲区的内存，而命令缓冲区则从其中分配。添加一个新的类成员来存储
VkCommandPool。

`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`和`VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`都是命令池创建标志，但它们的含义和用途有所不同：

1. `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`:
   这个标志表示命令缓冲区可能会频繁地被新命令重新记录（rerecorded）。在Vulkan中，你可以把一系列操作（例如绘图或内存传输等）录制到一个命令缓冲区中，然后在需要时执行这些操作。如果你知道某个命令缓冲区会经常被重用（即清空并填入新的命令），那么就可以在创建命令池时使用这个标志，Vulkan可能会根据此标志优化其内部的内存分配策略。
2. `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`:
   这个标志允许单独重置命令缓冲区。默认情况下，从同一个命令池分配的所有命令缓冲区必须一起被重置，也就是说，你不能只重置其中的一个，必须重置所有的。如果你设置了这个标志，那么就可以选择性地重置某个特定的命令缓冲区，而不是一口气重置所有的。

这两个标志在某些情况下是很有用的。例如，如果你的应用程序每帧都要录制新的命令（比如因为摄像机移动、物体动画等），那么`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`
可能会提高效率。同样，如果你的应用程序需要管理大量的命令缓冲区，并且希望能够灵活地重置它们，那么`VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`
就可能派上用场。

通过在设备队列（例如我们获取的图形和演示队列）上提交命令缓冲区来执行它们。每个命令池只能分配在单一类型的队列上提交的命令缓冲区。我们将记录用于绘制的命令，这就是我们选择图形队列族的原因。

#### 2. Command buffer

命令缓冲区在其命令池被销毁时会自动释放，所以我们不需要进行显式的清理。

现在我们将开始编写一个createCommandBuffer函数，从命令池中分配单个命令缓冲区。
命令缓冲区使用vkAllocateCommandBuffers函数进行分配，该函数接受一个VkCommandBufferAllocateInfo结构体作为参数，该结构体指定了命令池和要分配的缓冲区的数量：

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
> 当你"记录"
>
一个命令缓冲区时，你会使用各种Vulkan函数来填充（或记录）该命令缓冲区。这些函数对应的行为可能包括渲染、内存传输、计算等。这些命令不会立即被执行，而是存储在命令缓冲区中以供稍后执行。这样做的好处是可以预先准备多个操作，然后一次性提交给GPU进行处理，从而实现更有效的并行计算和资源管理。
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
> 总的来说，命令池、命令缓冲区和"record"
> 都是Vulkan中命令组织和调度过程的重要组成部分。首先你通过命令池创建并管理命令缓冲区，然后你将一系列命令“记录”到这些命令缓冲区中，最后再将这些缓冲区提交给GPU执行。

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

Vulkan的一个核心设计理念是GPU上执行的同步是显式的。操作的顺序由我们使用各种同步原语来定义，这些原语告诉驱动程序我们希望事物按照什么顺序运行。这意味着许多开始在GPU上执行工作的Vulkan
API调用都是异步的，这些函数将在操作完成之前返回。

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
> -
信号量：主要用于GPU与GPU之间的同步。例如，在两个不同的队列操作（可能是不同的GPU任务）之间添加顺序性。当一个操作完成时，它会发出（signal）一个信号量，然后另一个操作在开始之前等待（wait）这个信号量。这样可以保证操作的执行顺序。信号量主要关注GPU上的操作顺序，对于CPU来说，基本是透明的。
> -
围栏：主要用于CPU与GPU之间的同步。当我们提交一个任务到GPU执行，并且需要知道何时完成时，我们会使用一个围栏。任务完成后，围栏被标记为已信号状态，然后我们可以让CPU等待这个围栏变为已信号状态。这样可以确保在CPU继续之前，GPU的工作已经完成。因此，围栏主要关注主机（CPU）与设备（GPU）之间的同步。
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

在我们可以继续之前，我们的设计中出现了一个小问题。在第一帧我们调用drawFrame()
，它立即等待inFlightFence被标记。只有在一帧渲染完成后，inFlightFence才会被标记，但由于这是第一帧，所以没有前一帧可以标记围栏！因此，vkWaitForFences()
无限阻塞，等待永远不会发生的事情。

对于这个难题，有许多解决方案，API内置了一个巧妙的应对方法。创建一个已经标记为完成的围栏，这样第一次调用vkWaitForFences()
就会立即返回，因为围栏已经被标记。

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

在drawFrame函数中，我们需要做的下一件事就是从交换链获取图像。回想一下，交换链是一个扩展特性，所以我们必须使用带有vk*
KHR命名约定的函数：

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

请记住，渲染过程中的子通道会自动处理图像布局转换。这些转换由子通道依赖来控制，这些依赖指定了子通道之间的内存和执行依赖关系。我们现在只有一个子通道，但是在这个子通道之前和之后的操作也被视为隐式的"
子通道"。

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

### 1.5 Swap chain recreation

#### Introduction

我们现在的应用程序可以成功地绘制一个三角形，但还有一些情况没有得到适当地处理。窗口表面有可能发生变化，使得交换链与其不再兼容。造成此类情况的原因之一是窗口大小的改变。我们需要捕获这些事件并重新创建交换链。

#### Recreating the swap chain

创建一个新的recreateSwapChain函数，该函数调用createSwapChain以及所有取决于交换链或窗口大小的对象的创建函数。

```cpp
void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    createSwapChain();
    createImageViews();
    createFramebuffers();
}
```

我们首先调用vkDeviceWaitIdle，因为正如上一章所述，我们不应接触可能仍在使用的资源。显然，我们必须重新创建交换链。图像视图需要被重新创建，因为它们直接基于交换链图像。最后，帧缓冲区直接依赖于交换链图像，因此也必须被重新创建。

为确保在重新创建它们之前清理掉这些对象的旧版本，我们应该将一部分清理代码移到一个单独的函数中，我们可以从recreateSwapChain函数中调用它。我们称它为cleanupSwapChain:

```cpp
void cleanupSwapChain() {

}

void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}
```

请注意，为了简化，我们在此处没有重新创建renderpass。理论上，在应用程序的生命周期中，交换链图像格式可能会发生变化，例如，当将窗口从标准范围移动到高动态范围监视器时。这可能要求应用程序重新创建renderpass，以确保动态范围间的变化得到适当的反映。

我们将把所有作为交换链刷新的一部分重新创建的对象的清理代码从cleanup移动到cleanupSwapChain：

```cpp
void cleanupSwapChain() {
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
        vkDestroyImageView(device, m_SwapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void cleanup() {
    cleanupSwapChain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

注意，在chooseSwapExtent中，我们已经查询过新的窗口分辨率，以确保交换链图像具有（新的）正确尺寸，所以无需修改chooseSwapExtent（记住，我们在创建交换链时，已经必须使用glfwGetFramebufferSize来获取表面的像素分辨率）。

重新创建交换链就这么简单！然而，这种方法的缺点是我们需要在创建新的交换链之前停止所有的渲染。在从旧交换链的一个图像上还在进行绘制命令的同时，是可以创建一个新的交换链的。你需要将旧的交换链传递到VkSwapchainCreateInfoKHR结构的oldSwapChain字段，并在你完成使用后立即销毁旧的交换链。

#### Suboptimal or out-of-date swap chain

现在我们只需要弄明白何时需要重新创建交换链，并调用我们新的recreateSwapChain函数。幸运的是，Vulkan通常会在演示过程中告诉我们交换链不再适用。vkAcquireNextImageKHR和vkQueuePresentKHR函数可以返回以下特殊值来表示这一点。

VK_ERROR_OUT_OF_DATE_KHR: 交换链已经与表面不兼容，不能再用于渲染。通常在窗口调整大小后发生。
VK_SUBOPTIMAL_KHR: 交换链仍可以成功地呈现到表面，但表面属性已不完全匹配。

```cpp
VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
}
```

如果在尝试获取图像时交换链已过期，那么就无法再对其进行呈现。因此我们应立即重新创建交换链，并在下一个drawFrame调用中再尝试。

你也可以选择在交换链处于次优状态时这么做，但我选择在这种情况下继续进行，因为我们已经获取了一张图像。VK_SUCCESS和VK_SUBOPTIMAL_KHR都被认为是"
成功"的返回代码。

```cpp
result = vkQueuePresentKHR(presentQueue, &presentInfo);

if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
}

currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

vkQueuePresentKHR函数返回具有相同含义的相同值。在这种情况下，我们也会重新创建交换链，即使它是次优的，因为我们想要获得最好的可能结果。

#### Fixing a deadlock

如果我们现在试图运行代码，可能会遇到死锁。调试代码时，我们发现应用程序到达vkWaitForFences但永远不会继续执行。这是因为当vkAcquireNextImageKHR返回VK_ERROR_OUT_OF_DATE_KHR时，我们重新创建了交换链，然后从drawFrame返回。但在这之前，当前帧的围栏已经等待并重置。由于我们立即返回，没有工作被提交执行，围栏永远不会被信号量触发，导致vkWaitForFences永远等待。

幸运的是，有一个简单的修复办法。延迟重置围栏，直到我们确信我们将提交使用它的工作。因此，如果我们提前返回，围栏仍然被信号量触发，并且下次我们使用相同的围栏对象时，vkWaitForFences就不会产生死锁。

drawFrame的开始部分现在应该是这样的：

```cpp
vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

uint32_t imageIndex;
VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
}

// Only reset the fence if we are submitting work
vkResetFences(device, 1, &inFlightFences[currentFrame]);
```

#### Handling resizes explicitly

虽然许多驱动和平台会在窗口调整大小后自动触发
VK_ERROR_OUT_OF_DATE_KHR，但并不能保证总会发生。这就是为什么我们要添加额外的代码来显式处理大小调整。首先增加一个新的成员变量标志窗口大小已经改变：

```c++
std::vector<VkFence> inFlightFences;

bool framebufferResized = false;
```

然后修改 `drawFrame` 函数，检查此标志：

```c++
if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    ...
}
```

在 `vkQueuePresentKHR` 后进行上述操作非常重要，以确保信号量处于一致状态，否则可能无法正确等待已触发的信号量。现在，我们可以使用
GLFW 框架中的 `glfwSetFramebufferSizeCallback` 函数设置回调来检测尺寸调整：

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {

}
```

之所以创建一个静态函数作为回调，是因为 GLFW 不知道如何将正确的 this 指针作为参数来调用我们的 HelloTriangleApplication
实例中的成员函数。

然而，我们在回调中获取了对 GLFWwindow 的引用，并且 GLFW 还有另一个函数允许你在其中存储任意指针：`glfwSetWindowUserPointer`：

```c++
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
glfwSetWindowUserPointer(window, this);
glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
```

现在，可以通过 `glfwGetWindowUserPointer` 从回调中取出该值，然后正确设置标志：

```c++
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
```

现在试着运行程序，调整窗口大小，看看帧缓冲区是否能正确地随窗口一起调整大小。

#### Handling minimization

交换链可能会过时的另一个情况是窗口大小改变的特殊类型：窗口最小化。这个情况特殊的原因在于，它会导致帧缓冲区的大小变为
0。在本教程中，我们将通过扩展 `recreateSwapChain` 函数来处理，直到窗口再次处于前台：

```c++
void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    ...
}
```

首次调用 `glfwGetFramebufferSize` 处理了尺寸已经正确，而 `glfwWaitEvents` 则没什么可等待的情况。

恭喜你，你已经完成了你的第一个良好表现的 Vulkan 程序！在接下来的章节中，我们将去掉顶点着色器中的硬编码顶点，真正使用顶点缓冲区。

# 2. Vertex buffer

## 2.1 Vertex input description

### Introduction

在接下来的几个章节中，我们将用内存中的顶点缓冲替换顶点着色器中的硬编码顶点数据。我们将从创建一个可由CPU访问的缓冲区并使用memcpy直接将顶点数据复制到其中的最简单方法开始，然后我们将看到如何**使用阶段缓冲将顶点数据复制到高性能内存中**。

### Vertex shader

首先改变顶点着色器，不再将顶点数据包含在着色器代码本身中。顶点着色器使用'in'关键字从顶点缓冲中获取输入。

```glsl
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

'inPosition'和'inColor'变量是顶点属性。它们是在顶点缓冲中逐顶点指定的属性，就像我们使用两个数组手动为每个顶点指定位置和颜色一样。确保重新编译顶点着色器！

就像 'fragColor'，'layout(location = x)'注解为我们稍后用于引用的输入赋予索引。需要知道的重要一点是，某些类型，如64位向量 'dvec3'，使用多个插槽。这意味着其后的索引必须至少高出2：

```glsl
layout(location = 0) in dvec3 inPosition;
layout(location = 2) in vec3 inColor;
```

现在，使用 Vertex 结构来指定一组顶点数据。我们使用的位置和颜色值与之前完全相同，但现在它们被合并到一个顶点数组中。这就是所谓的顶点属性交错。

**绑定描述**

下一步是告诉 Vulkan 如何将此数据格式传递给顶点着色器，一旦它被上传到 GPU 内存中。需要两种类型的结构来传递这些信息。

第一个结构是 VkVertexInputBindingDescription，我们将向 Vertex 结构添加成员函数，以填充正确的数据。

```cpp
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        return bindingDescription;
    }
};
```

顶点绑定描述了从内存加载数据到各个顶点的速率。它指定了数据条目之间的字节数，以及是否在每个顶点或每个实例后移动到下一个数据条目。

```cpp
VkVertexInputBindingDescription bindingDescription{};
bindingDescription.binding = 0;
bindingDescription.stride = sizeof(Vertex);
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
```

我们所有的 per-vertex 数据都打包在一个数组中，因此我们只会有一个绑定。 绑定参数指定绑定数组中的绑定索引。stride 参数指定了从一个条目到下一个条目的字节数，inputRate 参数可以有以下几个值：

1. VK_VERTEX_INPUT_RATE_VERTEX：在每个顶点后移动到下一个数据条目
2. VK_VERTEX_INPUT_RATE_INSTANCE：在每个实例后移动到下一个数据条目

我们不会使用实例渲染，因此会坚持使用 per-vertex 数据。

**属性描述**

描述如何处理顶点输入的第二个结构是 VkVertexInputAttributeDescription。我们将在 Vertex 中添加另一个辅助函数来填充这些结构。

```cpp
#include <array>

...

static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    return attributeDescriptions;
}
```

如函数原型所示，将要有两个这样的结构。属性描述结构描述了如何从源自绑定描述的顶点数据块中提取顶点属性。我们有 position 和 color 这两个属性，所以需要两个属性描述结构。

```cpp
attributeDescriptions[0].binding = 0;
attributeDescriptions[0].location = 0;
attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
attributeDescriptions[0].offset = offsetof(Vertex, pos);
```

绑定参数告诉 Vulkan per-vertex 数据来自哪个绑定。location 参数引用了顶点着色器中输入的位置指令。顶点着色器中位置为 0 的输入是位置，该位置有两个 32 位浮点数分量。

format 参数描述了属性的数据类型。可能让人感到困惑的是，这些格式是使用与颜色格式相同的枚举来指定。以下着色器类型和格式通常一起使用：

1. float: VK_FORMAT_R32_SFLOAT
2. vec2: VK_FORMAT_R32G32_SFLOAT
3. vec3: VK_FORMAT_R32G32B32_SFLOAT
4. vec4: VK_FORMAT_R32G32B32A32_SFLOAT

你可以看到，应该使用颜色通道数量与着色器数据类型中的组件数量匹配的格式。允许使用的通道数量超过着色器中的组件数量，但它们将被静默丢弃。如果通道数量低于组件数量，则 BGA 组件将使用默认值 (0, 0, 1)。颜色类型 (SFLOAT, UINT, SINT) 和位宽也应匹配着色器输入的类型。请参阅以下示例：

1. ivec2: VK_FORMAT_R32G32_SINT，32 位有符号整数的 2 组件向量
2. uvec4: VK_FORMAT_R32G32B32A32_UINT，32 位无符号整数的 4 组件向量
3. double: VK_FORMAT_R64_SFLOAT，双精度（64 位）浮点数

format 参数隐式定义了属性数据的字节大小，offset 参数指定了从 per-vertex 数据开始读取的字节数。绑定正在一次加载一个 Vertex，并且位置属性（pos）在距离此结构开头 0 字节的偏移处。这是使用 offsetof 宏自动计算的。

```cpp
attributeDescriptions[1].binding = 0;
attributeDescriptions[1].location = 1;
attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
attributeDescriptions[1].offset = offsetof(Vertex, color);
```

颜色属性的描述方式大致相同。

**管线顶点输入**

我们现在需要设置图形管线，通过引用 createGraphicsPipeline 中的结构，使其接受此格式的顶点数据。在 vertexInputInfo 结构中找到并修改两个描述：

```cpp
auto bindingDescription = Vertex::getBindingDescription();
auto attributeDescriptions = Vertex::getAttributeDescriptions();

vertexInputInfo.vertexBindingDescriptionCount = 1;
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
```

现在，管线已经准备好接受 vertices 容器格式的顶点数据，并将其传递给我们的顶点着色器。如果您现在启用验证层运行程序，您会看到它抱怨没有顶点缓冲区绑定到绑定。下一步是创建一个顶点缓冲区，并将顶点数据移到其中，以便 GPU 能够访问它。

## 2.2 Vertex buffer creation

### Introduction

在Vulkan中，缓冲区是用于存储可以由图形卡读取的任意数据的内存区域。它们可以用来存储顶点数据（我们将在本章中这样做），但也可以用于许多其他目的，我们将在未来的章节中探索。与到目前为止我们一直处理的Vulkan对象不同，缓冲区并不会自动分配内存。前面几章的工作已经表明，Vulkan API将几乎所有事物的控制权交给了程序员，内存管理就是其中之一。

### Buffer creation

新建一个createVertexBuffer函数，并在initVulkan中createCommandBuffers之前调用它。

```c++
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
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

...

void createVertexBuffer() {

}
```
创建一个缓冲区需要我们填充一个VkBufferCreateInfo结构体。

```c++
VkBufferCreateInfo bufferInfo{};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = sizeof(vertices[0]) * vertices.size();
```
结构体的第一个字段是size，它指定缓冲区的大小（以字节为单位）。使用sizeof计算顶点数据的字节大小很简单。

```c++
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
```
第二个字段是usage，它指定缓冲区中的数据将被用于哪些目的。使用位或可以指定多个用途。我们的用例将是一个顶点缓冲区，我们将在未来的章节中查看其他类型的用途。

```c++
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```
就像交换链中的图像一样，缓冲区也可以由特定的队列族拥有，或者同时在多个队列族之间共享。缓冲区只从图形队列中使用，所以我们可以坚持使用独占访问模式。

flags参数用于配置稀疏缓存器内存，现在暂时不需要考虑。我们保持默认值0。

现在我们可以用vkCreateBuffer创建缓冲区。定义一个类成员来保存缓冲区句柄，并命名为vertexBuffer。

```c++
VkBuffer vertexBuffer;

...

void createVertexBuffer() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }
}
```
缓冲区应该在程序结束之前可用于渲染命令，并且不依赖于交换链，所以我们将在原始的cleanup函数中清理它：

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);

    ...
}
```

### Memory requirements

缓冲区已创建，但实际上还没有分配任何内存。为缓冲区分配内存的第一步是使用恰当命名的vkGetBufferMemoryRequirements函数查询其内存需求。

```c++
VkMemoryRequirements memRequirements;
vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
```
VkMemoryRequirements结构体有三个字段：

- size：所需内存量的大小（以字节为单位），可能与bufferInfo.size不同。
- alignment：缓冲区开始的内存区域的偏移量（以字节为单位），取决于bufferInfo.usage和bufferInfo.flags。
- memoryTypeBits：适合缓冲区的内存类型的位字段。

显卡可以提供不同类型的内存供分配。每种内存类型在允许的操作和性能特性方面都有所不同。我们需要结合缓冲区的要求和我们自己的应用程序要求来找出要使用的正确的内存类型。让我们为此创建一个新的函数findMemoryType。

```c++
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

}
```
首先，我们需要使用vkGetPhysicalDeviceMemoryProperties查询关于可用内存类型的信息。

```c++
VkPhysicalDeviceMemoryProperties memProperties;
vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
```
VkPhysicalDeviceMemoryProperties结构体有两个数组memoryTypes和memoryHeaps。内存堆是像专用VRAM和RAM中的交换空间（当VRAM用完时）这样的独立内存资源。这些堆中存在不同类型的内存。现在，我们只关心内存的类型，而不是它来自哪个堆，但你可以想象这可能会影响性能。

让我们首先找到一个适合缓冲区本身的内存类型：

```c++
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (typeFilter & (1 << i)) {
        return i;
    }
}

throw std::runtime_error("failed to find suitable memory type!");
```
typeFilter参数将用于指定适合的内存类型的位字段。这意味着我们可以通过简单地遍历它们并检查相应的位是否设置为1来找到适合的内存类型的索引。

然而，我们不仅对适合顶点缓冲区的内存类型感兴趣。我们还需要能够将我们的顶点数据写入那个内存。memoryTypes数组由VkMemoryType结构体组成，它们指定每种内存类型的堆和属性。属性定义了内存的特殊特性，比如能够映射它，这样我们就可以从CPU写入它。这个属性用VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT标示，但我们也需要使用VK_MEMORY_PROPERTY_HOST_COHERENT_BIT属性。我们在映射内存时会看到原因。

现在我们可以修改循环，也检查这个属性的支持：

```c++
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
    }
}
```
我们可能有多个期望的属性，所以我们应该检查位运算AND的结果不仅非零，而且等于期望的属性位字段。如果有一种适合缓冲区的内存类型同时也具有我们需要的所有属性，那么我们返回其索引，否则我们抛出异常。

### Memory allocation

我们现在有了确定正确内存类型的方法，所以我们可以通过填充VkMemoryAllocateInfo结构体实际分配内存。

```c++
VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
```
现在，内存分配就像指定大小和类型一样简单，这两者都是根据顶点缓冲区的内存需求和期望的属性派生出来的。创建一个类成员来存储内存句柄，并用vkAllocateMemory进行分配。

```c++
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

...

if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate vertex buffer memory!");
}
```
如果内存分配成功，我们现在可以通过使用 vkBindBufferMemory 将这段内存与缓冲区关联起来：

```cpp
vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
```

前三个参数的含义是显而易见的，第四个参数是内存区域的偏移量。由于这部分内存专门分配给顶点缓冲区，所以偏移量简单地为 0。如果偏移量非零，则要求它能被 memRequirements.alignment 整除。

当然，就像 C++ 中的动态内存分配一样，某个时候内存应该被释放。绑定到缓冲对象的内存可以在缓冲不再使用后被释放，所以让我们在缓冲被销毁后释放它：

```cpp
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
}
```

### Filling the vertex buffer

现在是将顶点数据复制到缓冲区的时间了。这需要通过 vkMapMemory 将缓冲区内存映射到 CPU 可访问的内存中完成。

```cpp
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
```

这个函数允许我们访问由偏移量和大小定义的指定内存资源的区域。这里的偏移量和大小分别为 0 和 bufferInfo.size。还可以指定特殊值 VK_WHOLE_SIZE 来映射所有的内存。倒数第二个参数可用于指定标志，但在当前的 API 中还没有任何可用的。它必须设为值 0。最后一个参数指定了映射内存的指针的输出。

```cpp
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
memcpy(data, vertices.data(), (size_t) bufferInfo.size);
vkUnmapMemory(device, vertexBufferMemory);
```

你现在可以简单地将顶点数据 memcpy 到映射的内存中，并使用 vkUnmapMemory 再次对其进行反映射。遗憾的是，驱动程序可能并不会立即将数据复制到缓冲区内存中，例如因为缓存原因。也有可能写入缓冲区的内容在映射的内存中还不可见。处理这个问题有两种方式：

- 使用主机一致的内存堆，用 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT 表示
- 在写入映射的内存后调用 vkFlushMappedMemoryRanges，在从映射的内存读取前调用 vkInvalidateMappedMemoryRanges

我们选择了第一种方法，这确保了映射的内存始终匹配分配的内存的内容。请记住，这可能导致性能稍差一些，但我们将在下一章看到为什么这并不重要。

刷新内存范围或使用一致的内存堆意味着驱动程序将知道我们对缓冲区的写操作，但这并不意味着它们实际上已经在 GPU 上可见了。数据传输到 GPU 是一个在后台进行的操作，规范告诉我们，它保证在下一次调用 vkQueueSubmit 时完全完成。

### Binding the vertex buffer

现在剩下的就是在渲染操作期间绑定顶点缓冲。我们将扩展 recordCommandBuffer 函数来做这个。

```cpp
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

VkBuffer vertexBuffers[] = {vertexBuffer};
VkDeviceSize offsets[] = {0};
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
```

vkCmdBindVertexBuffers 函数用于将顶点缓冲器绑定到我们在上一章设置的绑定。除了命令缓冲区，前两个参数指定了我们将为其指定顶点缓冲区的偏移量和绑定数量。最后两个参数指定了要绑定的顶点缓冲区数组以及开始读取顶点数据的字节偏移量。你也应该改变对 vkCmdDraw 的调用，以传递缓冲区中的顶点数量，而不是硬编码的数字 3。

现在运行程序，你应该再次看到熟悉的三角形：

尝试修改 vertices 数组，将顶点的颜色改为白色：

```cpp
const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

再次运行程序，你应该会看到如下图像：

在下一章中，我们将研究一种不同的方法来复制顶点数据到顶点缓冲区，这种方法可以提高性能，但需要做更多的工作。

## 2.3 Staging buffer 

### Introduction

我们现在的顶点缓冲工作得很好，但允许我们从 CPU 访问它的内存类型可能不是图形卡本身读取的最优内存类型。最优的内存有 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 标志，通常在专用图形卡上不可由 CPU 访问。在这一章节中，我们将创建两个顶点缓冲区。一个是在 CPU 可访问的内存中的暂存缓冡区，用于将数据从顶点数组上传到这里，而另一个是设备本地内存中的最终顶点缓冲区。然后我们将使用一个缓冲区复制命令将数据从暂存缓冲区移动到实际的顶点缓冲区。

### Transfer queue

缓冲区复制命令需要一个支持传输操作的队列家族，这是通过 VK_QUEUE_TRANSFER_BIT 表示的。好消息是，任何具有 VK_QUEUE_GRAPHICS_BIT 或 VK_QUEUE_COMPUTE_BIT 功能的队列家族都已隐式支持 VK_QUEUE_TRANSFER_BIT 操作。在这些情况下，实现不需要在 queueFlags 中显式列出它。

如果你喜欢挑战，那么你可以尝试使用特定的队列家族进行传输操作。这将要求你对程序进行以下修改：

- 修改 QueueFamilyIndices 和 findQueueFamilies，明确查找具有 VK_QUEUE_TRANSFER_BIT 位但没有 VK_QUEUE_GRAPHICS_BIT 的队列家族。
- 修改 createLogicalDevice，以请求传输队列的句柄
- 为提交到传输队列家族的命令缓冲创建第二个命令池
- 将资源的 sharingMode 更改为 VK_SHARING_MODE_CONCURRENT，并指定图形和传输队列家族
- 将所有传输命令（如我们将在本章中使用的 vkCmdCopyBuffer）提交到传输队列，而不是图形队列

这有点工作，但它会让你了解更多关于队列家族之间如何共享资源的知识。

### Abstracting buffer creation

因为我们将在本章中创建多个缓冲区，所以将缓冲区创建移动到一个辅助函数中是个好主意。创建一个新的名为 createBuffer 的函数，并将 createVertexBuffer 中的代码（除映射外）移动到其中。

```cpp
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
```

确保添加缓冲区大小、内存属性和使用的参数，这样我们就可以使用此函数来创建许多不同类型的缓冲区。最后两个参数是写入句柄的输出变量。

你现在可以从 createVertexBuffer 中删除缓冲区创建和内存分配的代码，并调用 createBuffer 替代：

```cpp
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, vertexBufferMemory);
}
```
运行你的程序以确保顶点缓冲仍然正常工作。

### Using a staging buffer

我们现在将改变 createVertexBuffer 只作为临时缓冲区使用主机可见的缓冲区，并使用设备本地的缓冲区作为实际的顶点缓冲区。

```cpp
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
}
```

我们现在使用一个新的带有 stagingBufferMemory 的 stagingBuffer 进行映射和复制顶点数据。在这一章中，我们将使用两个新的缓冲区使用标志：

- VK_BUFFER_USAGE_TRANSFER_SRC_BIT：缓冲区可以用作内存传输操作的源。
- VK_BUFFER_USAGE_TRANSFER_DST_BIT：缓冲区可以用作内存传输操作的目标。

vertexBuffer 现在从设备本地的内存类型中分配，这通常意味着我们无法使用 vkMapMemory。但是，我们可以从 stagingBuffer 复制数据到 vertexBuffer。我们必须通过为 stagingBuffer 指定转移源标志，以及为 vertexBuffer 指定转移目标标志和顶点缓冲区使用标志，来表明我们打算这样做。

我们现在将编写一个函数，将一个缓冲区的内容复制到另一个缓冲区，称为 copyBuffer。

```cpp
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

}
```

内存传输操作是使用命令缓冲执行的，就像绘图命令一样。因此，我们首先必须分配一个临时命令缓冲。你可能希望为这种短寿命的缓冲区创建一个单独的命令池，因为实现可能可以应用内存分配优化。在这种情况下，你应该在生成命令池时使用 VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 标志。

```cpp
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
}
```
并立即开始记录命令缓冲：

```cpp
VkCommandBufferBeginInfo beginInfo{};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

vkBeginCommandBuffer(commandBuffer, &beginInfo);
```
我们只使用一次命令缓冲区，并等待从函数返回直到复制操作完成执行。使用 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 告诉驱动我们的意图是个好习惯。

```cpp
VkBufferCopy copyRegion{};
copyRegion.srcOffset = 0; // Optional
copyRegion.dstOffset = 0; // Optional
copyRegion.size = size;
vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
```
通过 `vkCmdCopyBuffer` 命令传输缓冲区的内容。它接受源和目标缓冲区作为参数，以及要复制的区域数组。这些区域在 `VkBufferCopy` 结构体中定义，包括源缓冲区偏移量、目标缓冲区偏移量和大小。这里无法指定 `VK_WHOLE_SIZE`，与 `vkMapMemory` 命令不同。

```c
vkEndCommandBuffer(commandBuffer);
```
这个命令缓冲区只包含复制命令，所以我们可以在此之后停止记录。现在执行命令缓冲区以完成传输：

```c
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffer;

vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
vkQueueWaitIdle(graphicsQueue);
```
与绘制命令不同，这次我们不需要等待任何事件。我们只想立即在缓冲区上执行传输。有两种可能的方式来等待这个传输完成。我们可以使用栅栏并用 `vkWaitForFences` 等待，或者简单地等待传输队列变为空闲状态 `vkQueueWaitIdle`。使用栅栏将允许你同时调度多个传输并等待它们全部完成，而不是一次执行一个。这可能会给驱动程序更多的优化机会。

```c
vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
```
别忘了清理用于传输操作的命令缓冲区。

现在我们可以从 `createVertexBuffer` 函数调用 `copyBuffer`，将顶点数据移动到设备本地缓冲区：

```c
createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
```
从暂存缓冲区复制数据到设备缓冲区后，我们应该清理它：

```c
...

copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

vkDestroyBuffer(device, stagingBuffer, nullptr);
vkFreeMemory(device, stagingBufferMemory, nullptr);
```

运行你的程序，验证你是否再次看到熟悉的三角形。改进可能现在还不明显，但是它的顶点数据现在正在从高性能内存加载。当我们开始渲染更复杂的几何图形时，这将很重要。

### Conclusion

应该注意，在实际应用中，你不应该为每个单独的缓冲区实际调用 `vkAllocateMemory`。同时进行的内存分配的最大数量受物理设备限制 `maxMemoryAllocationCount` 的限制，即使在像 NVIDIA GTX 1080 这样的高端硬件上，也可能低至 4096。为大量对象同时分配内存的正确方法是创建一个自定义分配器，通过使用我们在许多功能中看到的偏移参数，将单个分配在许多不同的对象之间进行切割。

你可以自己实现这样一个分配器，或者使用 GPUOpen 初始化提供的 VulkanMemoryAllocator 库。然而，对于本教程，为每个资源使用单独的分配是可以的，因为我们暂时不会接近碰到这些限制。

## 2.4 Index buffer

### Introduction

在现实世界的应用中，你将要渲染的3D网格通常会在多个三角形之间共享顶点。这种情况甚至在绘制简单的矩形时就已经发生：

绘制一个矩形需要两个三角形，这意味着我们需要一个包含6个顶点的顶点缓冲区。问题是两个顶点的数据需要被复制，导致50%的冗余。更复杂的网格只会使情况变得更糟，其中顶点平均在3个三角形中重用。解决这个问题的方法是使用索引缓冲区。

索引缓冲区本质上是指向顶点缓冲区的指针数组。它允许你重新排序顶点数据，并为多个顶点重用现有数据。上面的插图演示了如果我们有一个包含四个唯一顶点的顶点缓冲区，那么矩形的索引缓冲区看起来会是什么样子。前三个索引定义了右上角的三角形，后三个索引定义了左下角三角形的顶点。

### Index buffer creation

在这一章中，我们将修改顶点数据并添加索引数据来绘制一个像插图中那样的矩形。修改顶点数据来表示四个角：

```cpp
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
```
左上角是红色的，右上角是绿色的，右下角是蓝色的，左下角是白色的。我们将添加一个新的数组 indices 来表示索引缓冲区的内容。它应该与插图中的索引匹配，以绘制右上角的三角形和左下角的三角形。

```cpp
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};
```
根据顶点中的条目数量，可以选择使用 uint16_t 或 uint32_t 作为索引缓冲区。因为我们现在使用的唯一顶点少于65535个，所以我们可以暂时坚持使用 uint16_t。

就像顶点数据一样，索引需要被上传到 GPU 能够访问的 VkBuffer 中。定义两个新的类成员来保存索引缓冲区的资源：

```cpp
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
```

我们现在要添加的 createIndexBuffer 函数几乎与 createVertexBuffer 完全相同：

```cpp
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    ...
}

void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```
有两处明显的不同。bufferSize 现在等于索引数量乘以索引类型（uint16_t 或 uint32_t）的大小。indexBuffer 的使用应该是 VK_BUFFER_USAGE_INDEX_BUFFER_BIT 而不是 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT，这是有道理的。除此之外，过程完全相同。我们创建一个暂存缓冲区来复制 indices 的内容，然后将其复制到最终的设备本地索引缓冲区。

程序结束时，应该清理索引缓冲区，就像清理顶点缓冲区一样：

```cpp
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    ...
}
```

### Using an index buffer

使用索引缓冲区进行绘制需要对 recordCommandBuffer 进行两处修改。首先，我们需要绑定索引缓冲区，就像我们为顶点缓冲区所做的那样。不同的是，你只能有一个索引缓冲区。遗憾的是，不可能为每个顶点属性使用不同的索引，所以如果只有一个属性变化，我们仍然必须完全复制顶点数据。

```cpp
vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
```
使用 vkCmdBindIndexBuffer 绑定索引缓冲区，该函数有索引缓冲区、字节偏移量以及索引数据类型作为参数。如前所述，可能的类型是 VK_INDEX_TYPE_UINT16 和 VK_INDEX_TYPE_UINT32。

仅仅绑定索引缓冲区还不能改变任何东西，我们还需要改变绘制命令，告诉 Vulkan 使用索引缓冲区。移除 vkCmdDraw 行并用 vkCmdDrawIndexed 来替换它：

```cpp
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```
对此函数的调用非常类似于 vkCmdDraw。前两个参数指定了索引数量和实例数量。我们没有使用 instancing，因此只需指定1个实例。索引数量表示将传递给顶点着色器的顶点数。下一个参数指定了索引缓冲区中的偏移量，使用值1会导致显卡从第二个索引开始读取。倒数第二个参数指定了在索引缓冲区中添加到索引的偏移量。最后一个参数指定了 instancing 的偏移量，我们没有使用这个参数。

现在运行你的程序，你应该看到如下图：

![image](https://vulkan-tutorial.com/images/indexed_rectangle.png)

你现在知道如何通过使用索引缓冲区重用顶点来节省内存了。在未来的章节中，我们将加载复杂的3D模型，这将特别重要。

前一章已经提到，你应该从单一的内存分配中分配多个资源，如缓冲区，但实际上，你应该走得更远。驱动程序开发者建议你也将多个缓冲区，如顶点和索引缓冲区，存储到单一的 VkBuffer 中，并在像 vkCmdBindVertexBuffers 这样的命令中使用偏移量。这样做的好处是，因为你的数据更紧密，所以更利于缓存。甚至可以在不同的渲染操作中重用同一块内存区域，前提是他们的数据被刷新了。这被称为别名(aliasing)，一些 Vulkan 函数有明确的标志来说明你想这样做。

# 3. Uniform buffers

## 3.1 Descriptor layout and buffer

### Introduction

我们现在可以向顶点着色器传递每个顶点的任意属性，但是全局变量呢？从这一章开始，我们将进入3D图形编程，这需要一个模型视图投影矩阵。我们可以将其作为顶点数据包括在内，但这会浪费内存，并且每当转换发生变化时，我们都需要更新顶点缓冲区。转换很容易在每一帧都发生变化。

在Vulkan中解决这个问题的正确方法是使用资源描述符。描述符是一种让着色器自由访问像缓冲区和图片这样的资源的方式。我们将建立一个包含转换矩阵的缓冲区，并让顶点着色器通过一个描述符来访问它们。描述符的使用包括三部分：

1. 在管线创建过程中指定一个描述符布局
2. 从描述符池中分配一个描述符集
3. 在渲染过程中绑定描述符集
描述符布局指定了管线将要访问的资源类型，就像渲染通道指定了将被访问的附件类型一样。描述符集指定了将被绑定到描述符的实际缓冲区或图片资源，就像帧缓冲区指定了实际的图片视图用于绑定到渲染通道附件一样。然后在绘制命令中绑定描述符集，就像顶点缓冡区和帧缓冲区一样。

有许多类型的描述符，但在这一章，我们将处理统一缓冲对象（UBO）。我们会在未来的章节中看到其他类型的描述符，但基本流程是相同的。假设我们要在C结构体中提供顶点着色器需要的数据，如下所示：

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```
然后我们可以将数据复制到一个VkBuffer中，并通过统一缓冲对象描述符在顶点着色器中访问它，如下所示：

```c++
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

我们将在每一帧中更新模型、视图和投影矩阵，使前一章中的矩形在3D中旋转。

### Vertex shader

修改顶点着色器以包含统一缓冲对象，就像上面指定的那样。我假设你对MVP变换有所了解。如果你不熟悉的话，请参阅第一章中提到的资源。

```c++
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

注意uniform、in 和out声明的顺序无关紧要。binding指令类似于属性的location指令。我们将在描述符布局中引用此绑定。改变gl_Position行，使用转换计算最终位置在剪贴坐标中。不同于2D三角形，剪辑坐标的最后一组可能不是1，这将在转换为屏幕上的最终规范化设备坐标时导致除法。这在透视投影中被用作透视除法，对于使近处的物体看起来比远处的物体大是必不可少的。

### Descriptor set layout

下一步是在C++端定义UBO，并告诉Vulkan关于顶点着色器中的这个描述符。

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```
我们可以使用GLM中的数据类型完全匹配着色器中的定义。矩阵中的数据与着色器期望的方式二进制兼容，因此我们稍后就可以直接memcpy一个UniformBufferObject到一个VkBuffer。

我们需要为管线创建提供关于着色器中使用的每一个描述符绑定的详细信息，就像我们必须为每一个顶点属性及其位置索引做的一样。我们将设置一个新的函数来定义所有这些信息，叫做createDescriptorSetLayout。这个函数应该在管线创建之前调用，因为我们在那里需要它。

```c++
void initVulkan() {
    ...
    createDescriptorSetLayout();
    createGraphicsPipeline();
    ...
}

...

void createDescriptorSetLayout() {

}
```

每个绑定都需要通过一个VkDescriptorSetLayoutBinding结构来描述。

```c++
void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
}
```

前两个字段指定了在着色器中使用的绑定和描述符的类型，也就是统一缓冲对象。着色器变量可以表示一个统一缓冲对象数组，descriptorCount指定了数组中的值的数量。例如，在骨骼动画中，这可以用于指定骨骼中每个骨头的变换。我们的MVP变换在一个统一缓冲对象中，因此我们使用一个descriptorCount为1。

```c++
uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
```

我们还需要指定在哪些着色器阶段中将引用描述符。stageFlags字段可以是VkShaderStageFlagBits值的组合，或者是VK_SHADER_STAGE_ALL_GRAPHICS的值。在我们的情况下，我们只从顶点着色器中引用描述符。

```c++
uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
```

pImmutableSamplers字段只对与图像采样相关的描述符有关，我们稍后将介绍。你可以保留它的默认值。

所有的描述符绑定将被组合到一个单一的VkDescriptorSetLayout对象中。在pipelineLayout上方定义一个新的类成员：

```c++
VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
```

我们可以使用vkCreateDescriptorSetLayout来创建它。这个函数接受一个简单的VkDescriptorSetLayoutCreateInfo，其中包含绑定数组：

```c++
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &uboLayoutBinding;

if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
}
```

我们需要在管线创建过程中指定描述符集布局，以告诉Vulkan着色器将使用哪些描述符。在管线布局对象中指定描述符集布局。修改VkPipelineLayoutCreateInfo以引用布局对象：

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
```

你可能会想知道为什么这里可以指定多个描述符集布局，因为一个已经包含了所有的绑定。我们将在下一章中再回到这一点，那时我们将深入了解描述符池和描述符集。

描述符布局应当在我们可能创建新的图形管线的整个过程中保持存在，即直到程序结束：

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    ...
}
```

### Uniform buffer

在下一章中，我们将指定包含着色器UBO数据的缓冲区，但首先我们需要创建这个缓冲区。我们将在每帧中复制新的数据到统一缓冲区，所以使用暂存缓冲区并没有任何意义。在这种情况下，它只会增加额外的开销，可能反而会降低性能。

我们应该有多个缓冲区，因为同时可能有多个帧在处理中，我们不希望在准备下一帧的时候更新正在被前一帧读取的缓冲区！因此，我们需要拥有与飞行中的帧数一样多的统一缓冲区，并且写入一个当前没有被GPU读取的统一缓冲区。

为此，添加新的类成员用于uniformBuffers和uniformBuffersMemory：

```c++
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;
```

同样地，创建一个新函数createUniformBuffers，在createIndexBuffer后调用并分配缓冲区：

```c++
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    ...
}

...

void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}
```

我们使用vkMapMemory映射创建后的缓冲区，以便稍后获取一个指针进行数据写入。缓冲区在应用程序的整个生命周期内都保持映射到这个指针。这种技术称为"持久映射"，适用于所有的Vulkan实现。由于映射不是免费的，在我们需要更新缓冲区时，不必每次都映射它可以提高性能。

统一数据将会被所有的绘制调用所使用，所以只有在停止渲染时才应该销毁包含它的缓冲区。

```c++
void cleanup() {
    ...

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    ...

}
```

### Updating uniform data

创建一个新的函数updateUniformBuffer，并在提交下一个帧之前从drawFrame函数中调用它：

```c++
void drawFrame() {
    ...

    updateUniformBuffer(currentFrame);

    ...

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    ...
}

...

void updateUniformBuffer(uint32_t currentImage) {

}
```
这个函数将在每一帧生成一个新的变换，使得几何体围绕着自己旋转。我们需要包含两个新的头文件来实现这个功能：

```c++
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
```
glm/gtc/matrix_transform.hpp头文件提供了一些函数，可用于生成模型变换如glm::rotate，视图变换如glm::lookAt以及投影变换如glm::perspective。GLM_FORCE_RADIANS定义是必要的，以确保像glm::rotate这样的函数使用弧度作为参数，避免任何可能的混淆。

chrono标准库头文件提供了进行精确计时的函数。我们将使用它来确保几何体每秒旋转90度，无论帧频如何。

```c++
void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
}
```

updateUniformBuffer函数将先计算出渲染开始以来以浮点精度计算的时间。

我们现在将在统一缓冲对象中定义模型、视图和投影变换。模型旋转将是围绕Z轴的简单旋转，使用时间变量：

```c++
UniformBufferObject ubo{};
ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```
glm::rotate函数接受现有的变换、旋转角度和旋转轴作为参数。glm::mat4(1.0f)构造函数返回恒等矩阵。使用旋转角度time * glm::radians(90.0f)可以达到每秒旋转90度的目的。

```c++
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

对于视图变换，我决定从上面45度的角度看几何体。glm::lookAt函数接受眼睛位置、中心位置和上轴作为参数。

```c++
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
```

我选择使用一个具有45度垂直视场的透视投影。其他参数是纵横比，近视平面和远视平面。使用当前交换链范围计算纵横比是很重要的，以考虑到窗口在调整大小后的新宽度和高度。

```c++
ubo.proj[1][1] *= -1;
```

GLM最初设计用于OpenGL，其中剪辑坐标的Y坐标是反转的。补偿的最简单方法是在投影矩阵的Y轴的缩放因子上反转符号。如果你不这样做，那么图像将会被反向渲染。

所有的变换现在都已定义，所以我们可以将统一缓冲对象中的数据复制到当前的统一缓冲器中。这与我们对顶点缓冲器的操作方式完全相同，只是不需要阶段缓冲器。如前所述，我们只映射一次统一缓冲器，所以我们可以直接写入，无需再次映射：

```c
memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
```

这种使用UBO的方式并非向着色器传递频繁变化的值的最有效方式。向着色器传递小型数据缓冲的更有效方式是推送常数。我们可能会在未来的章节中讨论这些。

在下一章中，我们将看到描述符集，它实际上会将VkBuffers绑定到统一缓冲器描述符，从而使着色器能够访问这些变换数据。

## 3.2 Descriptor pool and sets

### Introduction

上一章的描述符布局描述了可以绑定的描述符类型。在本章中，我们将为每个VkBuffer资源创建一个描述符集，以将其绑定到统一缓冲器描述符。

### Descriptor pool

描述符集不能直接创建，它们必须像命令缓冲器一样从池中分配。对于描述符集来说，这种等效的东西毫不奇怪地被称为描述符池。我们将编写一个新函数createDescriptorPool来设置它。

```c++
void initVulkan() {
    ...
    createUniformBuffers();
    createDescriptorPool();
    ...
}

...

void createDescriptorPool() {

}
```
我们首先需要描述我们的描述符集将包含哪些描述符类型以及它们的数量，使用VkDescriptorPoolSize结构。

```c++
VkDescriptorPoolSize poolSize{};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```
我们将为每一帧分配其中一个描述符。这个池大小结构被主要的VkDescriptorPoolCreateInfo引用：

```c++
VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
```
除了可用的单个描述符的最大数量外，我们还需要指定可能分配的描述符集的最大数量：

```c++
poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```

该结构有一个类似于命令池的可选标志，用来确定是否可以释放单个描述符集：VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT。我们在创建描述符集后不会去触碰它，所以我们不需要这个标志。你可以保留flags的默认值0。

```c++
VkDescriptorPool descriptorPool;

...

if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
}
```

添加一个新的类成员来存储描述符池的句柄，并调用vkCreateDescriptorPool来创建它。

### Descriptor set

现在我们可以分配描述符集本身了。为此添加一个createDescriptorSets函数：

```c++
void initVulkan() {
    ...
    createDescriptorPool();
    createDescriptorSets();
    ...
}

...

void createDescriptorSets() {

}
```

描述符集分配是通过VkDescriptorSetAllocateInfo结构体描述的。你需要指定要从中分配的描述符池，要分配的描述符集的数量，以及它们基于的描述符布局：

```c++
std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
VkDescriptorSetAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
allocInfo.pSetLayouts = layouts.data();
```

在我们的情况下，我们将为每个正在执行的帧创建一个描述符集，所有的描述符集都有相同的布局。不幸的是，我们确实需要所有布局的副本，因为下一个函数需要一个与集合数量匹配的数组。

添加一个类成员来保存描述符集句柄，并用vkAllocateDescriptorSets进行分配：

```c++
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

...

descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
}
```

你不需要显式清理描述符集，因为它们会在描述符池销毁时自动释放。调用vkAllocateDescriptorSets将分配描述符集，每个描述符集都有一个统一缓冲器描述符。

```c++
void cleanup() {
    ...
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    ...
}
```

描述符集现在已经分配，但是其中的描述符仍然需要配置。现在我们将添加一个循环来填充每个描述符：

```cpp
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

}
```

像我们的统一缓冲区描述符这样引用缓冲区的描述符，是通过VkDescriptorBufferInfo结构来配置的。此结构指定包含描述符数据的缓冲区及其内部的区域。

```cpp
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
}
```

如果你像这种情况一样覆盖整个缓冲区，则也可以使用VK_WHOLE_SIZE值作为范围。描述符的配置通过使用vkUpdateDescriptorSets函数更新，该函数接受一个VkWriteDescriptorSet结构的数组作为参数。

```cpp
VkWriteDescriptorSet descriptorWrite{};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSets[i];
descriptorWrite.dstBinding = 0;
descriptorWrite.dstArrayElement = 0;
```

前两个字段指定要更新的描述符集和绑定。我们给我们的统一缓冲区绑定了索引0。请记住，描述符可以是数组，因此我们还需要指定我们想要更新的数组中的第一个索引。我们没有使用数组，所以索引就是0。

```cpp
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrite.descriptorCount = 1;
```

我们需要再次指定描述符的类型。可以一次在数组中更新多个描述符，从索引dstArrayElement开始。descriptorCount字段指定你想要更新的数组元素数量。

```cpp
descriptorWrite.pBufferInfo = &bufferInfo;
descriptorWrite.pImageInfo = nullptr; // Optional
descriptorWrite.pTexelBufferView = nullptr; // Optional
```

最后一个字段引用了一个包含descriptorCount结构的数组，这些结构实际上配置了描述符。根据描述符的类型，你实际需要使用其中的哪一个。pBufferInfo字段用于引用缓冲区数据的描述符，pImageInfo用于引用图像数据的描述符，pTexelBufferView用于引用缓冲区视图的描述符。我们的描述符基于缓冲区，所以我们使用pBufferInfo。

```cpp
vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

使用vkUpdateDescriptorSets应用更新。它接受两种类型的数组作为参数：一个是VkWriteDescriptorSet的数组，另一个是VkCopyDescriptorSet的数组。后者可以用于复制描述符，如其名称所示。

### Using descriptor sets

我们现在需要更新recordCommandBuffer函数，以便实际绑定每个帧的正确描述符集到着色器中的描述符，这需要在vkCmdDrawIndexed调用之前完成：

```cpp
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

与顶点和索引缓冲区不同，描述符集对于图形管道并非独特。因此，我们需要指定是否希望将描述符集绑定到图形或计算管道。下一个参数是描述符基于的布局。接下来的三个参数指定了第一个描述符集的索引，要绑定的集合数量，以及要绑定的集合数组。我们会稍后回顾这一点。最后两个参数指定了用于动态描述符的偏移量数组。我们将在未来的章节中看到这些。

如果你现在运行程序，那么你会发现不幸的是什么都看不见。问题在于，由于我们在投影矩阵中进行了Y翻转，顶点现在被按逆时针顺序而不是顺时针顺序绘制。这导致了背面剔除的发生，阻止了任何几何体的绘制。转到createGraphicsPipeline函数，并修改VkPipelineRasterizationStateCreateInfo中的frontFace以纠正这一点：

```cpp
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

再次运行你的程序，你现在应该能看到以下内容：

![img](https://vulkan-tutorial.com/images/spinning_quad.png)

矩形已经变成了正方形，因为投影矩阵现在纠正了宽高比。updateUniformBuffer负责处理屏幕大小调整，所以我们不需要在recreateSwapChain中重新创建描述符集。

### Alignment requirements

到目前为止，我们只是粗略地介绍了C++结构中的数据应如何与着色器中的uniform定义匹配。似乎简单地在两者之间使用相同的类型就足够了：

```cpp
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```
然而，这还不算完。例如，尝试修改结构和着色器，使其看起来像这样：

```cpp
struct UniformBufferObject {
    glm::vec2 foo;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```
重新编译你的着色器和程序并运行它，你会发现你至今努力工作的彩色正方形消失了！这是因为我们没有考虑到对齐要求。

Vulkan期望你的结构中的数据以特定的方式在内存中对齐，例如：

- 标量必须被N（=4字节，给定32位浮点数）对齐。
- vec2必须被2N（=8字节）对齐。
- vec3或vec4必须被4N（=16字节）对齐。
- 嵌套结构必须按其成员的基础对齐进行四舍五入到16的倍数进行对齐。
- mat4矩阵必须与vec4有相同的对齐。

你可以在规范中找到对齐要求的完整列表。

我们原始的只有三个mat4字段的着色器已经满足了对齐要求。由于每个mat4的大小是4 x 4 x 4=64字节，model的偏移量为0，view的偏移量为64，proj的偏移量为128。所有这些都是16的倍数，这就是为什么它能正常工作。

新的结构以一个大小只有8字节的vec2开始，因此把所有的偏移量都打乱了。现在model的偏移量为8，view的偏移量为72，proj的偏移量为136，这些都不是16的倍数。为了解决这个问题，我们可以使用C++11引入的alignas指定符：

```cpp
struct UniformBufferObject {
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```
如果你现在再次编译并运行你的程序，你应该会看到着色器再次正确接收到其矩阵值。

幸运的是，大部分时间我们都不必考虑这些对齐要求。我们可以在包含GLM之前定义GLM_FORCE_DEFAULT_ALIGNED_GENTYPES：

```cpp
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
```
这将强制GLM使用一个版本的vec2和mat4，这些对齐要求已经为我们指定了。如果你添加了这个定义，那么你可以删除alignas指定符，你的程序应该仍然可以工作。

不幸的是，如果你开始使用嵌套结构，这种方法可能会出问题。考虑在C++代码中的以下定义：

```cpp
struct Foo {
    glm::vec2 v;
};

struct UniformBufferObject {
    Foo f1;
    Foo f2;
};
```
以及以下着色器定义：

```glsl
struct Foo {
    vec2 v;
};

layout(binding = 0) uniform UniformBufferObject {
    Foo f1;
    Foo f2;
} ubo;
```
在这种情况下，f2将有一个偏移量为8，而它应该有一个偏移量为16，因为它是一个嵌套结构。在这种情况下，你必须自己指定对齐：

```cpp
struct UniformBufferObject {
    Foo f1;
    alignas(16) Foo f2;
};
```
这些陷阱是一个很好的理由，让我们始终明确关于对齐。这样，你就不会被对齐错误的奇怪症状所捉襟见肘。

```cpp
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
```
在删除foo字段后，不要忘记重新编译你的着色器。

### Multiple descriptor sets

正如一些结构和函数调用所暗示的，实际上可以同时绑定多个描述符集。在创建管道布局时，你需要为每个描述符集指定一个描述符布局。然后，着色器可以像这样引用特定的描述符集：

```glsl
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```
你可以使用此功能将按对象变化的描述符和共享的描述符放入不同的描述符集。在这种情况下，你避免了在绘制调用过程中重绑定大部分的描述符，这可能更有效率。

# 4. Texture mapping

## 4.1 Images

### Introduction

到目前为止，我们一直使用每顶点颜色对几何图形进行着色，这是一种相当有限的方法。在本教程的这一部分，我们将实现纹理映射，使几何图形看起来更有趣。这也将让我们能在未来的章节中加载和绘制基础的3D模型。

向我们的应用程序添加一个纹理将涉及以下步骤：

- 创建由设备内存支持的图像对象
- 用图片文件中的像素填充它
- 创建一个图像采样器
- 添加一个组合的图像采样器描述符以从纹理中采样颜色
- 
我们之前已经处理过图像对象，但那些是由交换链扩展自动创建的。这次我们将不得不自己创建一个。创建一个图像并填充数据与创建顶点缓冲区类似。我们将开始通过创建阶段资源并填充像素数据，然后我们将这些数据复制到我们用于渲染的最终图像对象。尽管可以为此目的创建阶段图像，但Vulkan还允许你从VkBuffer复制像素到图像，而这个API在某些硬件上实际上更快。我们将首先创建这个缓冲区并填充像素值，然后我们将创建一个图像以复制像素。创建图像与创建缓冡区没有太大不同。它涉及查询内存需求、分配设备内存和绑定，就像我们之前见过的一样。

然而，在处理图像时，我们需要额外注意的事情。图像可以有不同的布局，影响像素在内存中的组织方式。由于图形硬件的工作方式，简单地按行存储像素可能不会带来最佳性能。在对图像执行任何操作时，你必须确保它们具有适用于该操作的最优布局。我们实际上已经在指定渲染通道时看到了一些这样的布局：

- VK_IMAGE_LAYOUT_PRESENT_SRC_KHR：最适合呈现
- VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL：作为从片段着色器写入颜色的附件最佳
- VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL：作为传输操作中的源最佳，如vkCmdCopyImageToBuffer
- VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL：作为传输操作中的目标最佳，如vkCmdCopyBufferToImage
- VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL：最适合从着色器采样

转换图像布局的最常见方法之一是使用管线障碍。管线障碍主要用于同步资源的访问，比如确保在读取图像之前已经写入，但它们也可以用于转换布局。在本章中，我们将看到如何使用管线障碍来进行布局转换。当使用VK_SHARING_MODE_EXCLUSIVE时，障碍还可以用于转移队列家族所有权。

### Image library

有许多库可以用于加载图像，你甚至可以编写自己的代码来加载简单的格式，如BMP和PPM。在本教程中，我们将使用stb集合中的stb_image库。它的优点是所有的代码都在一个文件中，所以不需要任何复杂的构建配置。下载stb_image.h并将其存储在一个方便的位置，比如你保存GLFW和GLM的目录。将该位置添加到你的包含路径中。

Visual Studio：

将含有stb_image.h的目录添加到"附加包含目录"路径中。

![img](https://vulkan-tutorial.com/images/include_dirs_stb.png)

Makefile：

将含有stb_image.h的目录添加到GCC的包含目录中：

```bash
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
STB_INCLUDE_PATH = /home/user/libraries/stb

...

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH)
```

### Loading an image

像这样包含图像库：

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

默认情况下，头文件只定义函数的原型。一个代码文件需要用STB_IMAGE_IMPLEMENTATION定义来包含函数体，否则我们会遇到链接错误。

```cpp
void initVulkan() {
    ...
    createCommandPool();
    createTextureImage();
    createVertexBuffer();
    ...
}

...

void createTextureImage() {

}
```
创建一个新函数createTextureImage，在其中我们将加载图像并将其上传到一个Vulkan图像对象中。我们将使用命令缓冲区，所以应该在createCommandPool之后调用。

在shaders目录旁边创建一个新的textures目录以存储纹理图像。我们将从该目录加载名为texture.jpg的图像。我选择使用以下CC0许可的图像，大小调整为512 x 512像素，但你可以选择任何你想要的图像。该库支持大多数常见的图像文件格式，如JPEG，PNG，BMP和GIF。

![img](https://vulkan-tutorial.com/images/texture.jpg)



```C++
void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
```

使用这个库加载图像非常容易：

```cpp
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
}
```

stbi_load函数将文件路径和要加载的通道数作为参数。STBI_rgb_alpha值强制图像加载带有alpha通道，即使它没有，这对于将来与其他纹理保持一致性是很好的。中间三个参数是图像的宽度、高度和实际通道数的输出。返回的指针是像素值数组的第一个元素。在STBI_rgb_alpha的情况下，像素按行排列，每个像素4个字节，总共有texWidth * texHeight * 4个值。

### Staging buffer

现在我们将在主机可见内存中创建一个缓冲区，以便我们可以使用vkMapMemory并将像素复制到它。在createTextureImage函数中添加此临时缓冲区的变量：

```cpp
VkBuffer stagingBuffer;
VkDeviceMemory stagingBufferMemory;
```

缓冲区应位于主机可见内存中，以便我们可以映射它，并且应该可以用作传输源，以便稍后我们可以将其复制到一个图像中：

```cpp
createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
```

然后我们可以直接将从图像加载库获取的像素值复制到缓冲区：

```cpp
void* data;
vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
vkUnmapMemory(device, stagingBufferMemory);
```

现在不要忘记清理原始像素数组：

```cpp
stbi_image_free(pixels);
```

### Texture Image

尽管我们可以设置着色器以访问缓冲区中的像素值，但是为了这个目的，在Vulkan中使用图像对象更好。图像对象会通过允许我们使用二维坐标，使得检索颜色更容易和更快。图像对象中的像素被称为像素点，从这一点开始我们将使用这个名字。添加以下新的类成员：

```cpp
VkImage textureImage;
VkDeviceMemory textureImageMemory;
```

图像的参数在VkImageCreateInfo结构体中指定：

```cpp
VkImageCreateInfo imageInfo{};
imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
imageInfo.imageType = VK_IMAGE_TYPE_2D;
imageInfo.extent.width = static_cast<uint32_t>(texWidth);
imageInfo.extent.height = static_cast<uint32_t>(texHeight);
imageInfo.extent.depth = 1;
imageInfo.mipLevels = 1;
imageInfo.arrayLayers = 1;
```

图像类型，指定在imageType字段中，告诉Vulkan如何使用坐标系统来寻址图像中的像素。可以创建1D、2D和3D图像。一维图像可以用来存储数组数据或渐变，二维图像主要用于纹理，三维图像可以用来存储体素体积，例如。extent字段指定了图像的尺寸，基本上是每个轴上有多少像素。这就是为什么深度必须为1而不是0。我们的纹理不会是一个数组，我们现在也不会使用mipmapping。

```cpp
imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
```

Vulkan支持许多可能的图像格式，但我们应该使用与缓冲区中像素相同的像素格式，否则复制操作将失败。

```cpp
imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
```

tiling字段可以有两个值之一：

- VK_IMAGE_TILING_LINEAR：像素按行优先顺序排列，就像我们的像素数组
- VK_IMAGE_TILING_OPTIMAL：像素按照实现定义的顺序排列，以便优化访问

与图像的布局不同，贴图模式不能在后期改变。如果你想能够直接访问图像内存中的像素，那么你必须使用VK_IMAGE_TILING_LINEAR。我们将使用阶段缓冲区而不是阶段图像，所以这不是必需的。我们将使用VK_IMAGE_TILING_OPTIMAL以便从着色器有效地访问。

```cpp
imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
```

图像的initialLayout只有两个可能的值：

- VK_IMAGE_LAYOUT_UNDEFINED：GPU无法使用，第一次过渡将丢弃像素。
- VK_IMAGE_LAYOUT_PREINITIALIZED：GPU无法使用，但第一次过渡将保留像素。

在第一次过渡期间需要保留像素的情况很少。然而，一个例子是，如果你想把图像作为阶段图像与VK_IMAGE_TILING_LINEAR布局结合使用。在这种情况下，你会想上传像素数据，然后在不失去数据的情况下转换图像以成为传输源。然而，在我们的情况下，我们首先要将图像转换为传输目标，然后从缓冲区对象复制像素数据到它，所以我们不需要这个属性，可以安全地使用VK_IMAGE_LAYOUT_UNDEFINED。

```cpp
imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
```

usage字段的语义与在创建缓冲区时的语义相同。图像将用作缓冲区复制的目标，因此应该将其设置为传输目标。我们还希望能够从着色器访问图像以着色我们的网格，因此使用应该包括VK_IMAGE_USAGE_SAMPLED_BIT。

```cpp
imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

图像只由一个队列家族使用：支持图形（因此也支持）传输操作的队列家族。

```cpp
imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
imageInfo.flags = 0; // Optional
```

samples标志与多采样相关。这只对将图像用作附件的图像有关，所以坚持使用一个样本。图像有一些与稀疏图像相关的可选标志。稀疏图像是只有某些区域实际背景内存的图像。例如，如果你正在使用一个3D纹理作为体素地形，那么你可以使用这个避免分配内存来存储大量的"空气"值。我们在本教程中不会使用它，所以将其保留为默认值0。

```cpp
if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
}
```

图像是使用vkCreateImage创建的，它没有任何特别值得注意的参数。有可能的是，VK_FORMAT_R8G8B8A8_SRGB格式可能无法通过显卡硬件支持。你应该有一个可以接受的替代方案列表，并选择其中最佳的一种支持的方案。然而，对这个特定格式的支持已经非常普遍，我们将跳过这一步。使用不同的格式也需要繁琐的转换。我们将在深度缓冲章节中回到这个问题，在那里我们将实现这样一个系统。

```cpp
VkMemoryRequirements memRequirements;
vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate image memory!");
}

vkBindImageMemory(device, textureImage, textureImageMemory, 0);
```
给图像分配内存的方式与给缓冲区分配内存的方式完全相同。使用 vkGetImageMemoryRequirements 替代 vkGetBufferMemoryRequirements， 使用 vkBindImageMemory 替代 vkBindBufferMemory。

此函数已经变得相当大，且在后续章节中还需要创建更多的图像，所以我们应该将图像创建抽象成一个createImage函数，就像我们对缓冲区做的那样。创建函数并将图像对象创建和内存分配移至其中：

```cpp
void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
```

我做了宽度、高度、格式、铺设模式、用途和内存特性参数，因为这些都会在我们整个教程中创建的图像之间变化。

现在可以将createTextureImage函数简化为：

```cpp
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
}
```
### Layout transitions

我们现在要编写的函数涉及到再次记录和执行一个命令缓冲，所以现在是时候将这个逻辑移到一个或两个帮助函数中：

```cpp
VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
```
这些函数的代码基于copyBuffer中现有的代码。现在你可以将该函数简化为：

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}
```
如果我们仍在使用缓冲区，那么现在可以编写一个函数来记录和执行vkCmdCopyBufferToImage以完成工作，但是这个命令需要图像首先处于正确的布局。创建一个新函数来处理布局转换：

```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```
执行布局转换的最常见方法之一是使用图像内存障碍。管线障碍通常用于同步访问资源，比如确保在从缓冲区读取之前完成对缓冲区的写入，但也可以用于转换图像布局和在使用VK_SHARING_MODE_EXCLUSIVE时转移队列家族所有权。对于缓冲区，有等效的缓冲区内存障碍可以实现此操作。

```c++
VkImageMemoryBarrier barrier{};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = oldLayout;
barrier.newLayout = newLayout;
```
前两个字段指定了布局转换。如果你不关心图像的现有内容，可以将VK_IMAGE_LAYOUT_UNDEFINED用作oldLayout。

```c++
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
```
如果你正在使用障碍来转移队列家族所有权，那么这两个字段应该是队列家族的索引。如果你不想做这样的事（并非默认值！），它们必须设置为VK_QUEUE_FAMILY_IGNORED。

```c++
barrier.image = image;
barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = 1;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = 1;
```
image和subresourceRange指定了受影响的图像和图像的特定部分。我们的图像不是数组，也没有mipmapping级别，所以只指定了一个级别和层。

```c++
barrier.srcAccessMask = 0; // TODO
barrier.dstAccessMask = 0; // TODO
```
障碍主要用于同步目的，因此你必须指定涉及资源的哪些类型的操作必须在障碍之前发生，以及涉及资源的哪些操作必须等待障碍。尽管我们已经使用vkQueueWaitIdle进行了手动同步，但我们仍需要做这件事。正确的值取决于旧布局和新布局，所以我们将在弄清楚要使用哪些过渡后再回到这个问题。

```c++
vkCmdPipelineBarrier(
    commandBuffer,
    0 /* TODO */, 0 /* TODO */,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```
所有类型的管线障碍都使用同一个函数提交。命令缓冲区后的第一个参数指定了应在障碍之前发生的操作的管线阶段。第二个参数指定了将在障碍处等待的操作的管线阶段。你被允许在障碍之前和之后指定的管线阶段取决于你在障碍之前和之后如何使用资源。允许的值在规范的这个表格中列出。

例如，如果你打算在障碍之后从uniform中读取，你会指定一个VK_ACCESS_UNIFORM_READ_BIT的使用情况，并将最早从uniform读取的着色器作为管线阶段，例如VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT。对于这种类型的使用情况，指定非着色器管线阶段是没有意义的，验证层会警告你当你指定与使用类型不匹配的管线阶段。

第三个参数是0或VK_DEPENDENCY_BY_REGION_BIT。后者将障碍变成一个按区域划分的条件。这意味着实现被允许已经开始从迄今为止写入的资源的部分进行读取。

最后三对参数引用了三种可用类型的管线障碍的数组：内存障碍、缓冲区内存障碍，以及像我们现在使用的图像内存障碍。注意，我们还没有使用VkFormat参数，但我们将在深度缓冲区章节中使用它进行特殊的转换。

### Copying buffer to image

在我们回到createTextureImage之前，我们要写一个更多的辅助函数：copyBufferToImage：

```c++
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```
就像缓冲区复制一样，你需要指定缓冲区的哪一部分将被复制到图像的哪一部分。这是通过VkBufferImageCopy结构体完成的：

```c++
VkBufferImageCopy region{};
region.bufferOffset = 0;
region.bufferRowLength = 0;
region.bufferImageHeight = 0;

region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
region.imageSubresource.mipLevel = 0;
region.imageSubresource.baseArrayLayer = 0;
region.imageSubresource.layerCount = 1;

region.imageOffset = {0, 0, 0};
region.imageExtent = {
    width,
    height,
    1
};
```
大部分字段都是自解释的。bufferOffset指定了像素值开始的缓冲区中的字节偏移量。bufferRowLength和bufferImageHeight字段指定了像素在内存中的布局方式。例如，图像的行之间可能有一些填充字节。对于这两者都指定0表示像素紧凑排列，就像我们的情况一样。imageSubresource、imageOffset和imageExtent字段指示我们要将像素复制到图像的哪一部分。

使用vkCmdCopyBufferToImage函数来排队执行缓冲区到图像的复制操作：

```c++
vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
);
```
第四个参数指示图像当前使用的布局。我在这里假设图像已经过渡到最适合复制像素的布局。现在我们只是将一个像素块复制到整个图像，但是可以指定一个VkBufferImageCopy数组，在一次操作中从缓冲区到图像执行很多不同的复制。

### Preparing the texture image

现在我们已经有了所有完成设置纹理图像所需的工具，我们将回到createTextureImage函数中。我们上一步做的是创建纹理图像。下一步是将暂存缓冲区复制到纹理图像中。这涉及到两个步骤：

1. 将纹理图像转换为VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL。
2. 执行从缓冲区到图像的复制操作。

使用我们刚刚创建的函数，这很容易做到：

```cpp
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
```

该图像是以VK_IMAGE_LAYOUT_UNDEFINED布局创建的，因此在转换textureImage时，应指定这一作为旧布局。请记住，我们可以这样做，因为在执行复制操作之前，我们不关心其内容。

要能够开始从着色器中的纹理图像抽样，我们需要进行最后一次转换以准备着色器访问：

```cpp
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
```

### Transition barrier masks

如果你现在启用验证层运行你的应用程序，那么你会发现它抱怨在transitionImageLayout中的访问掩码和管道阶段无效。我们仍然需要根据转换中的布局来设置这些。

我们需要处理两次转换：

1. 未定义 → 转移目标：转移写入不需要等待任何东西。
2. 转移目标 → 着色器读取：着色器读取应等待转移写入，特别是在片段着色器中的着色器读取，因为我们将在那里使用纹理。

这些规则通过以下访问掩码和管道阶段来指定：

```cpp
VkPipelineStageFlags sourceStage;
VkPipelineStageFlags destinationStage;

if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
} else {
    throw std::invalid_argument("unsupported layout transition!");
}

vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```
如上表中所示，转移写入必须在管道转移阶段中进行。由于写入不必等待任何事情，你可能会为预阻塞操作指定一个空的访问掩码和尽可能早的管道阶段VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT。应注意，VK_PIPELINE_STAGE_TRANSFER_BIT并非图形和计算管道内的实际阶段。这更像是一个伪阶段，其中发生转移。有关更多信息和其他伪阶段的示例，请参阅文档。

图像将在同一管道阶段中写入，并随后由片段着色器读取，这就是我们在片段着色器管道阶段指定着色器读取访问的原因。

如果我们在未来需要做更多的转换，那么我们将扩展该函数。应用程序现在应该可以成功运行，尽管当然还没有可视化的变化。

值得注意的一点是，命令缓冲区提交在开始时会导致隐式的VK_ACCESS_HOST_WRITE_BIT同步。由于transitionImageLayout函数执行只有一个命令的命令缓冲区，所以如果你在布局转换中需要一个VK_ACCESS_HOST_WRITE_BIT依赖性，你可以使用这种隐式同步并将srcAccessMask设置为0。是否希望明确说明这一点取决于你，但我个人并不喜欢依赖这些类似OpenGL的“隐藏”操作。

实际上有一种特殊类型的图像布局支持所有操作，即VK_IMAGE_LAYOUT_GENERAL。当然，问题在于它不一定能为任何操作提供最好的性能。对于一些特殊情况，例如将图像同时用作输入和输出，或者在图像离开预初始化布局后读取图像，这是必需的。

迄今为止提交命令的所有辅助函数都被设置为通过等待队列变为空闲来同步执行。对于实际应用，建议将这些操作组合到一个命令缓冲区中，并异步执行它们以获得更高的吞吐量，特别是在createTextureImage函数中的转换和复制。试着通过创建一个setupCommandBuffer来实验这个函数，让辅助函数记录命令，并添加一个flushSetupCommands来执行到目前为止已经记录的命令。最好在纹理映射工作后再做这个，以检查纹理资源是否仍然正确设置。

### Cleanup

在结束时，完成createTextureImage函数，清理暂存缓冲区及其内存：

```cpp
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

vkDestroyBuffer(device, stagingBuffer, nullptr);
vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```
主纹理图像将在程序结束时使用：

```cpp
void cleanup() {
    cleanupSwapChain();

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    ...
}
```
现在图像包含了纹理，但我们仍然需要一种方式从图形管道中访问它。我们将在下一章中继续处理这个问题。

## 4.2 Image view and sampler

在这一章节中，我们将创建两个为图形管道采样图像所需的资源。第一个资源我们在处理交换链图像时已经见过，但第二个是新的——它关系到着色器如何从图像读取纹理元素。

### Texture image view

我们之前已经看过，对于交换链图像和帧缓冲，图像是通过图像视图进行访问的，而不是直接访问的。我们也需要为纹理图像创建这样的图像视图。

在类中添加一个用于保存纹理图像的VkImageView成员，并创建一个新的函数createTextureImageView来创建它：

```cpp
VkImageView textureImageView;

...

void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createVertexBuffer();
    ...
}

...

void createTextureImageView() {

}
```

这个函数的代码可以直接基于createImageViews。你需要做的仅仅是修改格式和图像：

```cpp
VkImageViewCreateInfo viewInfo{};
viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
viewInfo.image = textureImage;
viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
viewInfo.subresourceRange.baseMipLevel = 0;
viewInfo.subresourceRange.levelCount = 1;
viewInfo.subresourceRange.baseArrayLayer = 0;
viewInfo.subresourceRange.layerCount = 1;
```
我省略了明确的viewInfo.components初始化，因为VK_COMPONENT_SWIZZLE_IDENTITY本身就定义为0。通过调用vkCreateImageView完成图像视图的创建：

```cpp
if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture image view!");
}
```

由于createImageViews的大部分逻辑被重复使用，你可能希望将其抽象为一个新的createImageView函数：

```cpp
VkImageView createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}
```

现在，createTextureImageView函数可以简化为：

```cpp
void createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}
```

并且createImageViews可以简化为：

```cpp
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
}
```

确保在程序结束时销毁图像视图，就在销毁图像本身之前：

```cpp
void cleanup() {
    cleanupSwapChain();

    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
}
```

### Samplers

着色器有可能直接从图像读取纹素，但当它们被用作纹理时这并不常见。纹理通常通过采样器访问，采样器将应用过滤和变换来计算检索到的最终颜色。

这些过滤器有助于处理诸如过采样之类的问题。考虑一个映射到具有更多片段而不是纹素的几何体的纹理。如果你只是在每个片段中为纹理坐标取最近的纹素，那么你将得到类似第一幅图像的结果：

![img](https://vulkan-tutorial.com/images/texture_filtering.png)

如果你通过线性插值结合了四个最近的纹素，那么你将得到如右图所示更平滑的结果。当然，您的应用程序可能有艺术风格要求，更适合左侧的样式（比如Minecraft），但在传统的图形应用中，右侧的样式更受欢迎。当从纹理读取颜色时，采样器对象会自动为你应用这种过滤。

欠采样则是相反的问题，你的纹素比片段多。这将导致在采样高频模式时产生错误，例如以锐角采样棋盘纹理：

![img](https://vulkan-tutorial.com/images/anisotropic_filtering.png)


如左图所示，纹理在远处变成了模糊的混乱。解决这个问题的办法是各向异性过滤，这也可以由采样器自动应用。

除了这些过滤器，采样器还可以处理变换。它确定了当你试图通过其寻址模式读取图像外部的纹素时会发生什么。下面的图片展示了一些可能的情况：

![img](https://vulkan-tutorial.com/images/texture_addressing.png)


现在我们将创建一个函数createTextureSampler来设置这样的采样器对象。稍后我们将使用这个采样器从着色器中读取纹理的颜色。

```c++
void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    ...
}

...

void createTextureSampler() {

}
```
采样器通过VkSamplerCreateInfo结构进行配置，该结构指定了它应该应用的所有过滤器和变换。

```c++
VkSamplerCreateInfo samplerInfo{};
samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
samplerInfo.magFilter = VK_FILTER_LINEAR;
samplerInfo.minFilter = VK_FILTER_LINEAR;
```
magFilter和minFilter字段指定如何插值放大或缩小的纹素。放大关注上述描述的过采样问题，而缩小关注欠采样。可选的有VK_FILTER_NEAREST和VK_FILTER_LINEAR，对应上面图像中演示的模式。

```c++
samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
```
寻址模式可以通过addressMode字段按轴指定。可用的值如下列出。其中大部分在上面的图像中已经演示过。注意，轴被称为U、V和W，而不是X、Y和Z。这是纹理空间坐标的约定。

- VK_SAMPLER_ADDRESS_MODE_REPEAT: 超出图像尺寸时重复纹理。
- VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: 像重复一样，但在超出尺寸时反转坐标以镜像图像。
- VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: 取最靠近超出图像尺寸坐标的边缘的颜色。
- VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: 与clamp to edge类似，但使用与最近边缘相反的边缘。
- VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: 在超出图像尺寸的范围内采样时返回固态颜色。

实际上，我们在这里使用哪种寻址模式并不重要，因为我们在这个教程中不打算在图像外部采样。然而，重复模式可能是最常见的模式，因为它可以用来铺设地板和墙壁等纹理。

```c++
samplerInfo.anisotropyEnable = VK_TRUE;
samplerInfo.maxAnisotropy = ???;
```
这两个字段指定是否应使用各向异性过滤。除非性能是一个问题，否则没有理由不使用它。maxAnisotropy字段限制了用于计算最终颜色的纹素样本的数量。较低的值会导致更好的性能，但质量较低。要找出我们可以使用的值，我们需要检索物理设备的属性，如下所示：

```c++
VkPhysicalDeviceProperties properties{};
vkGetPhysicalDeviceProperties(physicalDevice, &properties);
```
如果你查看VkPhysicalDeviceProperties结构的文档，你会发现它包含一个名为limits的VkPhysicalDeviceLimits成员。这个结构又有一个名为maxSamplerAnisotropy的成员，这就是我们可以为maxAnisotropy指定的最大值。如果我们想要达到最高质量，我们可以直接使用这个值：

```c++
samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
```
你可以在程序开始时查询属性，并将它们传递给需要它们的函数，或者在createTextureSampler函数中查询它们。

```c++
samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
```
borderColor字段指定了在使用clamp to border寻址模式超出图像采样时返回的颜色。可以返回黑色、白色或透明，无论是浮点格式还是整数格式。你不能指定任意颜色。

```c++
samplerInfo.unnormalizedCoordinates = VK_FALSE;
```
unnormalizedCoordinates字段指定了你希望使用哪种坐标系统来寻址图像中的纹素。如果此字段为VK_TRUE，那么你可以简单地使用[0, texWidth)和[0, texHeight)范围内的坐标。如果它是VK_FALSE，那么纹素是使用所有轴上的[0, 1)范围进行寻址的。实际应用几乎总是使用规范化的坐标，因为这样就可以使用完全相同的坐标使用不同分辨率的纹理。

```c++
samplerInfo.compareEnable = VK_FALSE;
samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
```
如果启用了比较函数，那么纹素首先将与一个值进行比较，然后将比较的结果用于过滤操作。这主要用于阴影图的百分比接近过滤。我们将在未来的章节中研究这个问题。

```c++
samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
samplerInfo.mipLodBias = 0.0f;
samplerInfo.minLod = 0.0f;
samplerInfo.maxLod = 0.0f;
```
所有这些字段都适用于 mipmap。我们会在后面的章节中详细讨论 mipmap，但基本上它是另一种可以应用的过滤器类型。

现在，采样器的功能已经完全定义了。添加一个类成员来保存采样器对象的句柄，并使用 vkCreateSampler 创建采样器：

```cpp
VkImageView textureImageView;
VkSampler textureSampler;

...

void createTextureSampler() {
    ...

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
```
注意，采样器并没有在任何地方引用 VkImage。采样器是一个独立的对象，它提供了从纹理中提取颜色的接口。无论你想对 1D、2D 还是 3D 的图像进行采样，都可以应用。这与许多旧的 API 不同，旧的 API 将纹理图像和过滤结合到一个状态中。

在程序结束时，当我们不再访问图像时，销毁采样器：

```cpp
void cleanup() {
    cleanupSwapChain();

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    ...
}
```

### Anisotropy device feature

如果你现在运行你的程序，你将会看到一个类似这样的验证层消息：

![img](https://vulkan-tutorial.com/images/validation_layer_anisotropy.png)

这是因为各向异性过滤实际上是一个可选的设备特性。我们需要更新 createLogicalDevice 函数来请求它：

```cpp
VkPhysicalDeviceFeatures deviceFeatures{};
deviceFeatures.samplerAnisotropy = VK_TRUE;
```
尽管现代显卡几乎肯定会支持它，但我们还是应该更新 isDeviceSuitable 函数来检查它是否可用：

```cpp
bool isDeviceSuitable(VkPhysicalDevice device) {
    ...

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
```
vkGetPhysicalDeviceFeatures 函数复用了 VkPhysicalDeviceFeatures 结构体，通过设置布尔值来指示哪些特性受支持，而不是被请求。

我们可以选择不强制要求各向异性过滤的可用性，也可以选择简单地不使用它，通过有条件地设置：

```cpp
samplerInfo.anisotropyEnable = VK_FALSE;
samplerInfo.maxAnisotropy = 1.0f;
```
在下一章节，我们将暴露图像和采样器对象给着色器，以便将纹理绘制到正方形上。

## 4.3 Combined image sampler

### Introduction

我们在教程的uniform buffers部分首次接触了描述符。在此章节中，我们将会学习一种新型的描述符：合并图像采样器。这种描述符使得shader能够通过像之前章节创建的那样的采样器对象访问图像资源。

首先，我们开始修改描述符布局、描述符池和描述符集以包括这种合并图像采样器描述符。然后，我们将为Vertex添加纹理坐标，并修改fragment shader以从纹理中读取颜色，而不是只是插值vertex颜色。

### Updating the descriptors

浏览至 createDescriptorSetLayout 函数并添加一个用于合并图像采样器描述符的 VkDescriptorSetLayoutBinding。我们将其放在 uniform buffer 后的绑定中：

```cpp
VkDescriptorSetLayoutBinding samplerLayoutBinding{};
samplerLayoutBinding.binding = 1;
samplerLayoutBinding.descriptorCount = 1;
samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
samplerLayoutBinding.pImmutableSamplers = nullptr;
samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
layoutInfo.pBindings = bindings.data();
```
确保设置 stageFlags 以表明我们打算在 fragment shader 中使用合并图像采样器描述符。那就是 fragment 的颜色将被确定的地方。在vertex shader中使用纹理采样也是可能的，例如通过高度图动态变形顶点网格。

我们还必须创建一个更大的描述符池以留出空间给合并图像采样器的分配，方法是在 VkDescriptorPoolCreateInfo 中添加另一个类型为 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER 的 VkPoolSize。前往 createDescriptorPool 函数并修改它以包含这个描述符的 VkDescriptorPoolSize：

```cpp
std::array<VkDescriptorPoolSize, 2> poolSizes{};
poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
poolInfo.pPoolSizes = poolSizes.data();
poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
```
不足的描述符池是一个验证层无法捕获的问题的好例子：如Vulkan 1.1，如果池不足够大，vkAllocateDescriptorSets可能以错误代码VK_ERROR_POOL_OUT_OF_MEMORY失败，但驱动程序也可能尝试内部解决问题。这意味着有时（取决于硬件、池大小和分配大小），驱动程序会让我们逃过一个超出我们描述符池限制的分配。其他时候，vkAllocateDescriptorSets会失败并返回VK_ERROR_POOL_OUT_OF_MEMORY。如果分配在一些机器上成功，但在其他机器上失败，这可能特别令人沮丧。

由于Vulkan将分配的责任转移到了驱动程序，因此只为特定类型的描述符（如VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER等）分配与创建描述符池所指定的对应descriptorCount成员数量，已经不再是严格要求。然而，最佳实践仍是这样做，未来，如果你启用最佳实践验证，VK_LAYER_KHRONOS_validation将会警告此类问题。

最后一步是将实际的图像和采样器资源绑定到描述符集中的描述符。前往 createDescriptorSets 函数。

```cpp
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    ...
}
```
合并图像采样器结构的资源必须在 VkDescriptorImageInfo 结构体中指定，就像uniform buffer描述符的缓存资源在 VkDescriptorBufferInfo 结构体中指定一样。这就是上一章中的对象在此处汇集的地方。

```cpp
std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[0].dstSet = descriptorSets[i];
descriptorWrites[0].dstBinding = 0;
descriptorWrites[0].dstArrayElement = 0;
descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrites[0].descriptorCount = 1;
descriptorWrites[0].pBufferInfo = &bufferInfo;

descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[1].dstSet = descriptorSets[i];
descriptorWrites[1].dstBinding = 1;
descriptorWrites[1].dstArrayElement = 0;
descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[1].descriptorCount = 1;
descriptorWrites[1].pImageInfo = &imageInfo;

vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
```
就像缓存一样，必须用这个图像信息更新描述符。这次我们使用的是pImageInfo数组，而不是pBufferInfo。现在描述符已经准备好被shader使用了！

### Texture coordinates

纹理贴图还缺少一个重要的成分，那就是每个顶点的实际坐标。这些坐标确定了如何将图像映射到几何体上。

```cpp
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};
```
修改 Vertex 结构以包含用于纹理坐标的 vec2。确保也添加了VkVertexInputAttributeDescription，这样我们就可以在顶点着色器中使用纹理坐标作为输入。这是必要的，才能将它们传递给片段着色器，以便在正方形的表面进行差值。

```cpp
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};
```
在本教程中，我将通过使用从左上角的 0,0 到右下角的 1,1 的坐标来填充纹理在正方形上。请随意尝试不同的坐标。尝试使用小于 0 或大于 1 的坐标，看看地址模式如何动作！

### Shaders

最后一步是修改着色器以从纹理中采样颜色。我们首先需要修改顶点着色器，将纹理坐标传递给片段着色器：

```glsl
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```
就像每个顶点的颜色一样，fragTexCoord 的值将被栅格化器平滑地插值在正方形的区域内。我们可以通过让片段着色器输出纹理坐标作为颜色来可视化这一点：

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragTexCoord, 0.0, 1.0);
}
```
你应该可以看到下面的图像。别忘了重新编译着色器！

![img](https://vulkan-tutorial.com/images/texcoord_visualization.png)

绿色通道代表水平坐标，红色通道代表垂直坐标。黑色和黄色的角落证实了纹理坐标已正确地从 0,0 插值到正方形的 1,1。使用颜色可视化数据是着色器编程中打印调试的等价物，因为没有更好的选项！

在GLSL中，结合图像采样器描述符由采样器统一体表示。在片段着色器中添加对它的引用：

```glsl
layout(binding = 1) uniform sampler2D texSampler;
```
对于其他类型的图像，有等效的 sampler1D 和 sampler3D 类型。确保在这里使用正确的绑定。

```glsl
void main() {
    outColor = texture(texSampler, fragTexCoord);
}
```
纹理是使用内置的 texture 函数采样的。它接受一个采样器和坐标作为参数。采样器会自动在后台处理过滤和转换。现在，当你运行应用程序时，你应该能看到正方形上的纹理：

![img](https://vulkan-tutorial.com/images/texture_on_square.png)

尝试通过将纹理坐标扩展到大于 1 的值来实验寻址模式。例如，当使用 VK_SAMPLER_ADDRESS_MODE_REPEAT 时，以下片段着色器产生了下图中的结果：

```glsl
void main() {
    outColor = texture(texSampler, fragTexCoord * 2.0);
}
```
![img](https://vulkan-tutorial.com/images/texture_on_square_repeated.png)

你也可以使用顶点颜色来操纵纹理颜色：

```glsl
void main() {
    outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}
```
这里我分离了 RGB 和 alpha 通道，以避免缩放 alpha 通道。

![img](https://vulkan-tutorial.com/images/texture_on_square_colorized.png)

现在你知道如何在着色器中访问图像了！这是一种非常强大的技术，当它与帧缓冲中也写入的图像一起使用时。你可以使用这些图像作为输入来实现诸如后处理和三维世界中的相机显示等酷炫的效果。

# 5. Depth buffering

## Introduction

我们迄今为止处理的几何形状虽然被投影到了3D，但它仍然是完全平面的。在这个章节中，我们将为位置加上一个Z坐标以准备3D网格。我们将使用这第三个坐标来放置一个方形超过当前方形，以查看当几何形状没有按深度排序时会出现的问题。

## 3D geometry

修改 Vertex 结构体，使其对位置使用3D向量，并在相应的 VkVertexInputAttributeDescription 中更新格式：

```cpp
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    ...

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        ...
    }
};
```
接下来，更新顶点着色器以接受并转换输入的3D坐标。别忘了之后要重新编译它！

```cpp
layout(location = 0) in vec3 inPosition;

...

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```
最后，更新顶点容器以包含Z坐标：

```cpp
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};
```
如果你现在运行你的应用程序，那么你应该会看到和之前完全一样的结果。现在是时候添加一些额外的几何形状来使场景更有趣，并展示我们在本章中要解决的问题。复制顶点以定义一个正方形的位置就像这样：

使用 Z 坐标 -0.5f 并为额外的正方形添加适当的索引：

```cpp
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
```
现在运行你的程序，你将看到类似于埃舍尔插图的东西：

问题在于，下方的正方形的片段被绘制在上方的正方形的片段之上，仅仅是因为它在索引数组中排在后面。有两种方式可以解决这个问题：

1. 从后至前按深度排序所有的绘图调用
2. 使用深度测试和深度缓冲区

第一种方法常用于绘制透明对象，因为无序独立透明是一个难以解决的挑战。然而，通过深度缓冲区更常见地解决了按深度排序片段的问题。深度缓冲区是一个额外的附件，它存储每个位置的深度，就像颜色附件存储每个位置的颜色一样。每次栅格化器产生一个片段时，深度测试都会检查新的片段是否比前一个片段更近。如果不是，那么新的片段就会被丢弃。通过深度测试的片段将其自身的深度写入深度缓冲区。您可以从片段着色器中操作此值，就像您可以操作颜色输出一样。

```cpp
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
```

GLM生成的透视投影矩阵默认会使用OpenGL的深度范围-1.0到1.0。我们需要配置它使用Vulkan的范围0.0到1.0，方法是使用GLM_FORCE_DEPTH_ZERO_TO_ONE定义。

## Depth image and view

深度附件基于一个图像，就像颜色附件一样。不同之处在于，交换链不会自动为我们创建深度图像。我们只需要一个深度图像，因为一次只运行一个绘制操作。深度图像将再次需要三个资源：图像、内存和图像视图。

```cpp
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;
```
创建一个新的函数 createDepthResources 来设置这些资源：

```cpp
void initVulkan() {
    ...
    createCommandPool();
    createDepthResources();
    createTextureImage();
    ...
}

...

void createDepthResources() {

}
```
创建深度图像相当直接。它应该具有与颜色附件相同的分辨率，由交换链范围定义，适用于深度附件的图像使用，最佳瓷砖和设备本地内存。唯一的问题是：深度图像的正确格式是什么？格式必须包含深度组件，由 _D??_ 在 VK_FORMAT_ 中表示。

与纹理图像不同，我们不一定需要特定的格式，因为我们不会直接从程序访问 texels。它只需要具有合理的准确性，至少24位在现实世界的应用程序中很常见。有几种符合此要求的格式：

- VK_FORMAT_D32_SFLOAT: 32位浮点深度
- VK_FORMAT_D32_SFLOAT_S8_UINT: 32位有符号浮点深度和8位模板组件
- VK_FORMAT_D24_UNORM_S8_UINT: 24位浮点深度和8位模板组件

模板组件用于模板测试，这是可以与深度测试结合的额外测试。我们将在未来的章节中查看这个。

我们可以简单地选择 VK_FORMAT_D32_SFLOAT 格式，因为对它的支持非常普遍（参见硬件数据库），但是在可能的情况下为我们的应用增加一些额外的灵活性是很好的。我们将编写一个函数 findSupportedFormat，该函数接受一系列候选格式，按从最期望到最不期望的顺序，并检查哪个是第一个被支持的：

```cpp
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

}
```
格式的支持取决于贴砖模式和使用，所以我们也必须把这些作为参数包括进来。格式的支持可以使用 vkGetPhysicalDeviceFormatProperties 函数进行查询：

```cpp
for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
}
```
VkFormatProperties 结构体包含三个字段：

- linearTilingFeatures: 支持线性贴图的使用情况
- optimalTilingFeatures: 支持最优贴图的使用情况
- bufferFeatures: 缓冲区的支持使用情况

在这里，只有前两个是相关的，我们检查的那个取决于函数的贴图参数：

```cpp
if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
    return format;
} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
    return format;
}
```
如果所有的候选格式都不支持期望的使用，那么我们可以返回一个特殊的值或者直接抛出一个异常：

```cpp
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}
```
我们现在会使用这个函数来创建一个 findDepthFormat 助手函数，以选择一个支持作为深度附件使用的具有深度组件的格式：

```cpp
VkFormat findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
```
确保在这种情况下使用 VK_FORMAT_FEATURE_标记，而不是 VK_IMAGE_USAGE_。所有这些候选格式都包含一个深度组件，但后两者还包含一个模板组件。我们尚未使用它，但在对这些格式的图像执行布局转换时，我们需要考虑到这一点。添加一个简单的助手函数，告诉我们所选的深度格式是否包含模板组件：

```cpp
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
```
从 createDepthResources 调用函数找到深度格式：

```cpp
VkFormat depthFormat = findDepthFormat();
```
我们现在已经拥有了调用我们的 createImage 和 createImageView 助手函数所需的所有信息：

```cpp
createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
depthImageView = createImageView(depthImage, depthFormat);
```
然而，createImageView 函数目前假设子资源始终是 VK_IMAGE_ASPECT_COLOR_BIT，所以我们需要把这个字段变成一个参数：

```cpp
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    ...
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    ...
}
```
更新对此函数的所有调用以使用正确的方面：

```cpp
swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
...
depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
...
textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
```
这就是创建深度图像的全部内容。我们不需要映射它或将另一个图像复制到它上面，因为我们将在渲染通道开始时清除它，就像颜色附件一样。

## Explicitly transitioning the depth image

我们无需显式地将图像的布局转换为深度附件，因为我们会在渲染过程中处理这个问题。然而，出于完整性，我仍然会在这一部分描述这个过程。你如果愿意可以跳过它。

在createDepthResources函数的结尾处调用transitionImageLayout，如下所示：

```cpp
transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
```

未定义的布局可以作为初始布局使用，因为没有现有的深度图像内容是重要的。我们需要更新transitionImageLayout中的一些逻辑以使用正确的子资源方面：

```cpp
if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (hasStencilComponent(format)) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
} else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
}
```

虽然我们没有使用模板组件，但我们确实需要在深度图像的布局转换中包括它。

最后，添加正确的访问掩码和管道阶段：

```cpp
if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
} else {
    throw std::invalid_argument("unsupported layout transition!");
}
```

深度缓冲区将被读取以进行深度测试，以查看一个片段是否可见，并在绘制新片段时写入。读取发生在VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT阶段，写入发生在VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT阶段。你应该选择最早匹配指定操作的管道阶段，以便在需要时准备好作为深度附件使用。

## Render pass

我们现在将修改createRenderPass以包含深度附件。首先，指定VkAttachmentDescription：

```cpp
VkAttachmentDescription depthAttachment{};
depthAttachment.format = findDepthFormat();
depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```
格式应与深度图像本身相同。这次我们不关心存储深度数据（storeOp），因为在绘制完成后它将不被使用。这可能允许硬件执行额外的优化。就像颜色缓冲区一样，我们不关心之前的深度内容，所以我们可以使用VK_IMAGE_LAYOUT_UNDEFINED作为initialLayout。

```cpp
VkAttachmentReference depthAttachmentRef{};
depthAttachmentRef.attachment = 1;
depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```
为第一个（也是唯一的）子通道添加对附件的引用：

```cpp
VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
subpass.pDepthStencilAttachment = &depthAttachmentRef;
```
与颜色附件不同，子通道只能使用一个深度(+模板)附件。对多个缓冲区进行深度测试真的没有任何意义。

```cpp
std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
renderPassInfo.pAttachments = attachments.data();
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```
接下来，更新VkSubpassDependency结构以引用两个附件。

```cpp
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
```
最后，我们需要扩展我们的子通道依赖关系，以确保深度图像的转换和它作为其加载操作的一部分被清除之间没有冲突。深度图像在早期片段测试管道阶段首次被访问，因为我们有一个清除操作，我们应该为写入指定访问掩码。

## Framebuffer

下一步是修改帧缓冲创建以将深度图像绑定到深度附件。进入createFramebuffers并将深度图像视图指定为第二个附件：

```cpp
std::array<VkImageView, 2> attachments = {
    swapChainImageViews[i],
    depthImageView
};

VkFramebufferCreateInfo framebufferInfo{};
framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
framebufferInfo.renderPass = renderPass;
framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
framebufferInfo.pAttachments = attachments.data();
framebufferInfo.width = swapChainExtent.width;
framebufferInfo.height = swapChainExtent.height;
framebufferInfo.layers = 1;
```
每个交换链图像的颜色附件都不同，但是所有图像都可以使用同一个深度图像，因为由于我们的信号量，只有一个子通道正在同时运行。

你还需要将调用createFramebuffers移动到确保在深度图像视图实际创建后调用它的地方：

```cpp
void initVulkan() {
    ...
    createDepthResources();
    createFramebuffers();
    ...
}
```
## Clear values

因为我们现在有多个具有VK_ATTACHMENT_LOAD_OP_CLEAR的附件，我们也需要指定多个清除值。转到recordCommandBuffer并创建一个VkClearValue结构的数组：

```cpp
std::array<VkClearValue, 2> clearValues{};
clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
clearValues[1].depthStencil = {1.0f, 0};

renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
renderPassInfo.pClearValues = clearValues.data();
```

在Vulkan中，深度缓冲区的深度范围是0.0到1.0，其中1.0位于远视平面，0.0位于近视平面。深度缓冲区中每个点的初始值应为最远可能的深度，即1.0。

请注意，clearValues的顺序应与你的附件的顺序相同。

## Depth and stencil state

现在我们可以使用深度附件了，但是仍需要在图形管线中启用深度测试。这可以通过 `VkPipelineDepthStencilStateCreateInfo` 结构进行配置：

```cpp
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = VK_TRUE;
depthStencil.depthWriteEnable = VK_TRUE;
```
`depthTestEnable` 字段指定是否应将新片段的深度与深度缓冲区进行比较，以确定是否应丢弃它们。`depthWriteEnable` 字段指定是否应将通过深度测试的片段的新深度写入深度缓冡区。

```cpp
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
```
`depthCompareOp` 字段指定用于保留或丢弃片段的比较操作。我们坚持较低深度 = 更接近的原则，所以新片段的深度应该更小。

```cpp
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.minDepthBounds = 0.0f; // Optional
depthStencil.maxDepthBounds = 1.0f; // Optional
```
`depthBoundsTestEnable`、`minDepthBounds` 和 `maxDepthBounds` 字段用于可选的深度边界测试。基本上，这允许你只保留落在指定深度范围内的片段。我们不会使用此功能。

```cpp
depthStencil.stencilTestEnable = VK_FALSE;
depthStencil.front = {}; // Optional
depthStencil.back = {}; // Optional
```
最后三个字段配置模板缓冲区操作，我们在这个教程中也不会使用。如果你想使用这些操作，那么你必须确保深度/模板图像的格式包含一个模板组件。

```cpp
pipelineInfo.pDepthStencilState = &depthStencil;
```
更新 `VkGraphicsPipelineCreateInfo` 结构以引用我们刚刚填充的深度模板状态。如果渲染通道包含深度模板附件，则必须始终指定深度模板状态。

如果你现在运行你的程序，那么你应该看到几何体的片段现在已经被正确地排序了。

![img](https://vulkan-tutorial.com/images/depth_correct.png)

## Handling window resize

当窗口大小改变时，深度缓冲区的分辨率应该改变，以匹配新的颜色附件分辨率。扩展 `recreateSwapChain` 函数，在这种情况下重建深度资源：

```cpp
void recreateSwapChain() {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}
```
清理操作应该在交换链清理函数中进行：

```cpp
void cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    ...
}
```
恭喜，你的应用现在终于准备好渲染任意3D几何图形，并使其看起来正确了。我们将在下一章中尝试通过绘制一个纹理模型来测试此功能！

# 6. Loading models

## Introduction

您的程序现在已经准备好渲染纹理化的3D网格，但是目前在顶点和索引数组中的几何形状还不够有趣。在本章中，我们将扩展这个程序，从一个实际的模型文件中加载顶点和索引，使图形卡真正开始工作。

许多图形API教程会在这样的一章中让读者自己编写一个OBJ加载器。这样的问题是，任何稍微有趣的3D应用程序很快就会需要一些该文件格式不支持的特性，如骨骼动画。我们将在这一章从一个OBJ模型加载网格数据，但我们会更多地关注如何将网格数据与程序本身整合，而非从文件中加载它的具体方式。

## Library

我们将使用tinyobjloader库从OBJ文件中加载顶点和面。它速度快且易于集成，因为它像stb_image一样是单文件库。访问上述链接的代码库，下载tiny_obj_loader.h文件到你的库目录的一个文件夹中。

Visual Studio

将包含tiny_obj_loader.h的目录添加到"Additional Include Directories"路径中。

![img](https://vulkan-tutorial.com/images/include_dirs_tinyobjloader.png)

## Sample mesh

在这个章节，我们还无法启用光照，因此使用一个已经将光照烘焙进纹理的样本模型会有所帮助。寻找这种模型的一种简单方式是在Sketchfab上查找3D扫描模型。该站点上的许多模型都以OBJ格式提供，并拥有宽松的许可证。

对于这个教程，我决定使用nigelgoh制作的维京人房间模型 (CC BY 4.0)。我调整了模型的尺寸和方向，以便将其用作当前几何体的替代品：

- [viking_room.obj](https://vulkan-tutorial.com/resources/viking_room.obj)
- [viking_room.png](https://vulkan-tutorial.com/resources/viking_room.png)

你可以自由使用你自己的模型，但请确保它只包含一种材料，并且尺寸大约为1.5 x 1.5 x 1.5个单位。如果它比这个大，那么你将需要修改视图矩阵。把模型文件放在shaders和textures旁边的新models目录中，把纹理图像放在textures目录中。

在你的程序中加入两个新的配置变量来定义模型和纹理路径：

```cpp
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
```

并更新createTextureImage以使用此路径变量：

```cpp
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
```

## Loading vertices and indices

现在我们要从模型文件中加载顶点和索引，所以你应该现在就删除全局的vertices和indices数组。将它们替换为非const容器作为类成员：

```cpp
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
```

你应该将indices的类型从uint16_t更改为uint32_t，因为将来会有超过65535个顶点。也记得更改vkCmdBindIndexBuffer参数：

```cpp
vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
```

tinyobjloader库的包含方式与STB库相同。包含tiny_obj_loader.h文件，并确保在一个源文件中定义TINYOBJLOADER_IMPLEMENTATION以包含函数体并避免链接错误：

```cpp
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
```

现在我们将编写一个loadModel函数，该函数使用这个库来填充顶点和索引容器，从网格获取顶点数据。在创建顶点和索引缓冲区之前，在某个地方调用它：

```cpp
void initVulkan() {
    ...
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    ...
}

...

void loadModel() {

}
```

通过调用tinyobj::LoadObj函数将模型加载到库的数据结构中：

```cpp
void loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }
}
```

OBJ文件由位置、法线、纹理坐标和面组成。面由任意数量的顶点组成，每个顶点都引用一个位置、法线和/或纹理坐标的索引。这使得不仅可以重用整个顶点，还可以重用单个属性。

attrib容器在其attrib.vertices、attrib.normals和attrib.texcoords向量中保存所有的位置、法线和纹理坐标。shapes容器包含所有的单独对象及其面。每个面由一个顶点数组组成，每个顶点包含位置、法线和纹理坐标属性的索引。OBJ模型还可以为每个面定义材质和纹理，但我们将忽略这些。

err字符串包含加载文件时发生的错误，warn字符串包含警告，例如缺少材质定义。只有当LoadObj函数返回false时，加载才真正失败。如上所述，OBJ文件中的面实际上可以包含任意数量的顶点，而我们的应用程序只能渲染三角形。幸运的是，LoadObj具有一个可选参数，可以自动将这些面三角化，这是默认启用的。

我们将把文件中所有的面合并成一个模型，所以只需遍历所有的形状：

```cpp
for (const auto& shape : shapes) {

}
```

三角化功能已经确保每个面有三个顶点，所以我们现在可以直接迭代顶点，并直接将它们转储到我们的顶点向量中：

```cpp
for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        vertices.push_back(vertex);
        indices.push_back(indices.size());
    }
}
```

为了简单起见，我们暂时假设每个顶点都是唯一的，因此索引是简单的自增索引。index变量的类型是tinyobj::index_t，它包含vertex_index、normal_index和texcoord_index成员。我们需要使用这些索引在attrib数组中查找实际的顶点属性：

```cpp
vertex.pos = {
    attrib.vertices[3 * index.vertex_index + 0],
    attrib.vertices[3 * index.vertex_index + 1],
    attrib.vertices[3 * index.vertex_index + 2]
};

vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    attrib.texcoords[2 * index.texcoord_index + 1]
};

vertex.color = {1.0f, 1.0f, 1.0f};
```

不幸的是，attrib.vertices 数组是一个浮点值数组，而不是像 glm::vec3 那样，所以你需要将索引乘以 3。同样，每个条目有两个纹理坐标组件。偏移量 0、1 和 2 用于访问 X、Y 和 Z 组件，或者在纹理坐标的情况下是 U 和 V 组件。

现在以优化模式运行你的程序（例如 Visual Studio 中的 Release 模式和 GCC 的 -O3 编译器标志）。这是必要的，否则加载模型将非常慢。你应该会看到如下内容：

![img](https://vulkan-tutorial.com/images/inverted_texture_coordinates.png)

太好了，几何形状看起来正确，但是纹理上到底发生了什么？OBJ格式假定一个坐标系统，其中垂直坐标为0表示图像的底部，然而我们已经将图像按照从上到下的方向上传到Vulkan中，其中0表示图像的顶部。通过翻转纹理坐标的垂直组件来解决这个问题：

```cpp
vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
};
```

当你再次运行程序时，你应该能看到正确的结果：

![img](https://vulkan-tutorial.com/images/drawing_model.png)

所有的辛苦工作终于在这样的演示中开始得到回报！

> 当模型旋转时，您可能会注意到后部（墙壁的背面）看起来有点奇怪。这是正常的，仅仅是因为该模型实际上并未设计成从那一侧观看。

## Vertex deduplication

遗憾的是，我们还没有真正利用索引缓冲器。vertices向量包含了大量重复的顶点数据，因为许多顶点都包含在多个三角形中。我们应该只保留唯一的顶点，并使用索引缓冲器在它们出现时重用它们。实现这一点的一种直接方法是使用map或unordered_map来跟踪唯一的顶点和相应的索引：

```cpp
#include <unordered_map>

...

std::unordered_map<Vertex, uint32_t> uniqueVertices{};

for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        ...

        if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }

        indices.push_back(uniqueVertices[vertex]);
    }
}
```
每次我们从OBJ文件读取一个顶点，我们检查是否已经看到过具有完全相同位置和纹理坐标的顶点。如果没有，我们将其添加到vertices中，并在uniqueVertices容器中存储其索引。然后，我们将新顶点的索引添加到indices中。如果我们以前看到过完全相同的顶点，那么我们在uniqueVertices中查找其索引，并将该索引存储在indices中。

由于在哈希表中使用自定义类型，如我们的Vertex结构体作为键，需要我们实现两个函数：等式测试和哈希值计算，所以程序现在无法编译。通过在Vertex结构体中覆盖==运算符，可以轻松地实现前者：

```cpp
bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
}
```

通过为std::hash<T>指定模板专用化来实现Vertex的哈希函数。哈希函数是一个复杂的主题，但是cppreference.com推荐以下方法，将结构体的字段组合起来创建一个质量较好的哈希函数：

```cpp
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
```
这段代码应放在Vertex结构体外部。对于GLM类型的哈希函数需要通过以下头文件进行引用：

```cpp
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
```
哈希函数在gtx文件夹中定义，这意味着它技术上仍然是GLM的实验性扩展。因此，您需要定义GLM_ENABLE_EXPERIMENTAL来使用它。这意味着在未来的GLM新版本中API可能会更改，但实际上API非常稳定。

你现在应该能够成功地编译并运行你的程序。如果你检查vertices的大小，你会发现它已经从1,500,000减小到265,645！这意味着每个顶点在平均约6个三角形中被重用。这绝对节省了我们很多的GPU内存。

# 7. Generating Mipmaps

## Introduction

我们的程序现在可以加载并渲染3D模型了。在本章中，我们将添加一个新特性，即mipmap生成。Mipmaps在游戏和渲染软件中广泛应用，Vulkan让我们能够完全控制它们的创建方式。

Mipmaps是图像预先计算出的缩小版本。每个新图像的宽度和高度都是前一个图像的一半。Mipmaps被用作一种形式的细节层次或LOD。远离摄像头的物体会从更小的mip图像中采样其纹理。使用较小的图像可以提高渲染速度，并避免如莫尔波纹等视觉效果。以下是mipmaps的示例：

![img](https://vulkan-tutorial.com/images/mipmaps_example.jpg)

## Image creation

在Vulkan中，每个mip图像都储存在VkImage的不同mip级别中。Mip级别0是原始图像，0级之后的mip级别通常被称为mip链。

在创建VkImage时要指定mip级别的数量。到目前为止，我们一直将这个值设置为1。我们需要根据图像的尺寸来计算mip级别的数量。首先，添加一个类成员来存储这个数字：

```c++
...
uint32_t mipLevels;
VkImage textureImage;
...
```
在我们加载纹理以后,可以在createTextureImage方法中找到`mipLevels`的值：

```c++
int texWidth, texHeight, texChannels;
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
...
mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
```

这计算了mip链中的级别数量。max函数选择最大的维度。log2函数计算那个维度可以被2除多少次。floor函数处理最大维度不是2的幂的情况。原始图像有一个mip级别，所以加1。

要使用这个值，我们需要改变createImage、createImageView和transitionImageLayout函数，以允许我们指定mip级别的数量。在这些函数中添加一个mipLevels参数：

```c++
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.mipLevels = mipLevels;
    ...
}
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    ...
    viewInfo.subresourceRange.levelCount = mipLevels;
    ...
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    ...
    barrier.subresourceRange.levelCount = mipLevels;
    ...
```
更新所有对这些函数的调用，以使用正确的值：

```c++
createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
...
depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
...
textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
...
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
```

## Generating Mipmaps

我们现在的纹理图像有多个 mip  层级，但是暂存缓冲区只能用于填充 mip 级别 0。其他级别仍未定义。要填充这些级别，我们需要从我们拥有的单个级别生成数据。我们将使用 vkCmdBlitImage 命令。此命令执行复制，缩放和过滤操作。我们将多次调用此命令，以将数据 blit 到我们纹理图像的每个级别。

vkCmdBlitImage 被认为是一个传输操作，因此我们必须告知 Vulkan，我们打算将纹理图像用作传输的源和目标。在 createTextureImage 中向纹理图像的使用标志添加 VK_IMAGE_USAGE_TRANSFER_SRC_BIT：

```cpp
createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```
与其他图像操作一样，vkCmdBlitImage 依赖于它操作的图像的布局。我们可以将整个图像转换为 VK_IMAGE_LAYOUT_GENERAL，但这可能会比较慢。为了获得最佳性能，源图像应该位于 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL 中，目标图像应该位于 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 中。Vulkan 允许我们独立地转换图像的每个 mip 级别。每个 blit 只会处理两个 mip 级别，所以我们可以在 blit 命令之间将每个级别转换成最优布局。

transitionImageLayout 只对整个图像执行布局转换，因此我们需要编写更多的 pipeline barrier 命令。在 createTextureImage 中删除现有的转换到 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL：

```cpp
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//在生成 mipmaps 时转换为 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
...
```
这将使纹理图像的每个级别都处于 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL。在从它读取的 blit 命令完成后，每个级别将转换为 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL。

现在，我们将编写生成 mipmaps 的函数：
```cpp
void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    endSingleTimeCommands(commandBuffer);
}
```
我们将进行多次转换，所以我们将重复使用此 VkImageMemoryBarrier。上述设置的字段将对所有屏障保持不变。subresourceRange.miplevel，oldLayout，newLayout，srcAccessMask 和 dstAccessMask 将在每次转换时进行更改。

```cpp
int32_t mipWidth = texWidth;
int32_t mipHeight = texHeight;

for (uint32_t i = 1; i < mipLevels; i++) {

}
```
此循环将记录每个 VkCmdBlitImage 命令。注意 loop 变量从 1 开始，而不是 0。

```cpp
barrier.subresourceRange.baseMipLevel = i - 1;
barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
```
首先，我们将级别 i - 1 转换为 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL。此转换将等待级别 i - 1 被填充，或者来自前一个 blit 命令，或来自 vkCmdCopyBufferToImage。当前的 blit 命令将等待此转换。

```cpp
VkImageBlit blit{};
blit.srcOffsets[0] = { 0, 0, 0 };
blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
blit.srcSubresource.mipLevel = i - 1;
blit.srcSubresource.baseArrayLayer = 0;
blit.srcSubresource.layerCount = 1;
blit.dstOffsets[0] = { 0, 0, 0 };
blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
blit.dstSubresource.mipLevel = i;
blit.dstSubresource.baseArrayLayer = 0;
blit.dstSubresource.layerCount = 1;
```
接下来，我们指定将在 blit 操作中使用的区域。源 mip 级别是 i - 1，目标 mip 级别是 i。srcOffsets 数组的两个元素确定将从其中 blit 数据的 3D 区域。dstOffsets 确定将 blit 数据的区域。由于每个 mip 级别是上一个级别的一半大小，所以 dstOffsets[1] 的 X 和 Y 维度被除以二。srcOffsets[1] 和 dstOffsets[1] 的 Z 维度必须为 1，因为 2D 图像的深度为 1。

```cpp
vkCmdBlitImage(commandBuffer,
    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &blit,
    VK_FILTER_LINEAR);
```
现在，我们记录 blit 命令。注意 textureImage 被用于 srcImage 和 dstImage 参数。这是因为我们在同一图像的不同级别之间进行 blit。源 mip 级别刚刚转换为 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL，目标级别仍然是 createTextureImage 中的 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL。

如果您正在使用专用的传输队列（如 Vertex buffers 中所建议的）要注意：vkCmdBlitImage 必须提交给具有图形能力的队列。

最后一个参数允许我们在 blit 中指定 VkFilter。我们在制作 VkSampler 时有相同的过滤选项。我们使用 VK_FILTER_LINEAR 来启用插值。

```cpp
barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
```
这个障碍将 mip 级别 i - 1 转变为 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL。此转换会等待当前的 blit 命令完成。所有的采样操作都会等待这个转换完成。

    ...
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
}
在循环的最后，我们将当前 mip 维度除以二。我们在进行除法之前检查每一个维度，以确保该维度永远不会变为0。这解决了图像不是平方的情况，因为其中一个 mip 维度会在另一个维度之前到达1。当这种情况发生时，该维度应该在所有剩余的级别中保持为1。

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
    
    endSingleTimeCommands(commandBuffer);
}
在结束命令缓冲区之前，我们插入了另一个管道障碍。这个障碍将最后一个 mip 级别从 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 转换为 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL。这个转换在循环中没有处理，因为最后的 mip 级别从未被 blit。

最后，在 createTextureImage 中添加对 generateMipmaps 的调用：

``````C++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
...
generateMipmaps(textureImage, texWidth, texHeight, mipLevels);
``````

现在我们的纹理图像的 mipmaps 已经完全填充。

## Linear filtering support

使用内建函数如vkCmdBlitImage生成所有的mip等级是非常方便的，但不幸的是它并不能保证在所有平台上都被支持。它要求我们使用的纹理图像格式支持线性过滤，这可以通过vkGetPhysicalDeviceFormatProperties函数进行检查。我们将为此添加一个到generateMipmaps函数的检查。

首先添加一个指定图像格式的额外参数：

```c++
void createTextureImage() {
    ...

    generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

    ...
}
```

在generateMipmaps函数中，使用vkGetPhysicalDeviceFormatProperties请求纹理图像格式的属性：

```c++
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

    // 检查图像格式是否支持线性位块传输Blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    ...
}
```

VkFormatProperties结构体有三个字段名为linearTilingFeatures、optimalTilingFeatures和bufferFeatures，每个字段描述了格式如何使用取决于它的使用方式。我们使用最优贴图格式创建纹理图像，所以我们需要检查optimalTilingFeatures。可以使用VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT来检查线性过滤特征的支持：

```c++
if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("texture image format does not support linear blitting!");
}
```

在这种情况下有两种方法。你可以实现一个函数，用于搜索支持线性位块传输的普通纹理图像格式，或者你可以使用stb_image_resize这样的库在软件中实现mipmap的生成。然后每个mip等级都可以像你加载原始图像一样被加载到图像中。

应该注意的是，实际上在运行时生成mipmap等级是不常见的。通常，它们是预先生成的，并存储在纹理文件中，与基础级别一起，以提高加载速度。在软件中实现调整大小并从文件中加载多个等级作为读者的练习留给读者。

## Sampler

虽然VkImage保存了mipmap数据，VkSampler控制着在渲染时如何读取这些数据。Vulkan允许我们指定minLod、maxLod、mipLodBias和mipmapMode（"Lod"表示"详细级别"）。当采样一个纹理时，采样器根据以下伪代码选择一个mip等级：

```c++
lod = getLodLevelFromScreenSize(); // 当对象较近时较小，可能为负数
lod = clamp(lod + mipLodBias, minLod, maxLod);

level = clamp(floor(lod), 0, texture.mipLevels - 1);  // 限制到纹理中的mip等级数量

if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
    color = sample(level);
} else {
    color = blend(sample(level), sample(level + 1));
}
```

如果samplerInfo.mipmapMode是VK_SAMPLER_MIPMAP_MODE_NEAREST，lod选取从哪个mip等级进行采样。如果mipmap模式是VK_SAMPLER_MIPMAP_MODE_LINEAR，lod被用来选择两个要被采样的mip等级。这两个等级被采样，结果被线性混合。

采样操作也受到lod的影响：

```c++
if (lod <= 0) {
    color = readTexture(uv, magFilter);
} else {
    color = readTexture(uv, minFilter);
}
```

如果物体靠近相机，magFilter被用作滤波器。如果物体离相机较远，使用minFilter。通常，lod是非负的，只有在接近相机时才为0。mipLodBias使我们可以强制Vulkan使用比正常情况下更低的lod和level。

要查看本章的结果，我们需要为textureSampler选择值。我们已经设置了minFilter和magFilter使用VK_FILTER_LINEAR。我们只需要选取minLod、maxLod、mipLodBias和mipmapMode的值。

```c++
void createTextureSampler() {
    ...
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f; 
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f; 
    ...
}
```

为了允许使用全范围的mip等级，我们将minLod设为0.0f，而maxLod设为mip等级的数量。我们没有改变lod值的理由，所以把mipLodBias设为0.0f。

现在运行你的程序，你应该会看到以下的结果：

![img](https://vulkan-tutorial.com/images/mipmaps.png)

这并没有明显的差异，因为我们的场景非常简单。如果仔细看，会发现一些微妙的区别。

![img](https://vulkan-tutorial.com/images/mipmaps_comparison.png)

最明显的区别是纸张上的字迹。有了mipmaps，字迹被平滑了。没有mipmaps，字迹有严格的边缘和摩尔纹路造成的间隙。

你可以玩弄采样器的设置，看看它们如何影响mipmapping。例如，通过改变minLod，你可以强制采样器不使用最低的mip等级：

```c++
samplerInfo.minLod = static_cast<float>(mipLevels / 2);
```

这些设置将产生这样的图像：

![img](https://vulkan-tutorial.com/images/highmipmaps.png)

这就是当物体离相机远时，将如何使用更高的mip等级。

# 8. Multisampling

## Introduction

我们的程序现在可以为纹理加载多个细节等级，这修复了从远离观察者的地方渲染物体时产生的工件。图像现在看起来更加平滑，然而如果仔细观察，你会注意到画出的几何形状的边缘有锯齿状的模式。这在我们早期程序中尤为明显，那时我们渲染了一个四边形：

![img](https://vulkan-tutorial.com/images/texcoord_visualization.png)

这种不希望的效果被称为"走样"，这是由于用于渲染的像素数量有限所造成的。由于没有具有无限分辨率的显示器，因此在某种程度上这将总是可见的。有许多方法可以解决这个问题，在本章中，我们将重点介绍最流行的一种：多采样抗锯齿（MSAA）。

在普通的渲染中，像素颜色是基于一个单一的采样点确定的，大多数情况下这个采样点是屏幕上目标像素的中心。如果绘制的线条部分经过某个像素，但没有覆盖采样点，那么该像素将被留空，导致锯齿状的"阶梯"效果。

![img](https://vulkan-tutorial.com/images/aliasing.png)

MSAA所做的是使用每个像素的多个采样点（因此得名）来确定其最终颜色。正如人们所期望的，更多的样本导致更好的结果，然而它也更耗计算资源。

![img](https://vulkan-tutorial.com/images/antialiasing.png)

在我们的实现中，我们将专注于使用最大可用的样本数量。根据你的应用，这可能并不总是最好的方法，如果最终结果满足你的质量需求，使用较少的样本以获得更高的性能可能更好。

## Getting available sample count

首先，让我们确定我们的硬件可以使用多少样本。大多数现代GPU至少支持8个样本，但这个数字并不能保证在所有地方都是一样的。我们通过添加一个新的类成员来跟踪它：

```
VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
```

默认情况下，我们只使用每个像素的一个样本，这相当于没有多重采样，这样最终图像将保持不变。与我们选择的物理设备相关联的VkPhysicalDeviceProperties可以提取出精确的最大样本数。我们正在使用一个深度缓冲区，所以我们必须考虑颜色和深度的样本数。颜色和深度都支持的最高样本数（&）将是我们可以支持的最大值。添加一个函数，它将为我们获取这个信息：

```c++
VkSampleCountFlagBits getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}
```

现在我们将使用这个函数在物理设备选择过程中设置msaaSamples变量。为此，我们需要稍微修改pickPhysicalDevice函数：

```c++
void pickPhysicalDevice() {
    ...
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }
    ...
}
```

## Setting up a render target

在多重采样抗锯齿（MSAA）中，每个像素都会在一个离屏缓冲器中进行采样，然后再渲染到屏幕上。这个新的缓冲器与我们之前渲染的普通图像略有不同 - 它们需要能够存储每个像素的多个样本。一旦创建了多重采样缓冲器，就必须将其解析为默认帧缓冲（每个像素只存储一个样本）。这就是我们需要创建另一个渲染目标并修改我们当前的绘制过程的原因。我们只需要一个渲染目标，因为一次只有一个绘制操作处于活动状态，就像深度缓冲区一样。添加以下类成员：

```markdown
...
VkImage colorImage;
VkDeviceMemory colorImageMemory;
VkImageView colorImageView;
...
```
此新图像需存储每个像素所需的样本数，因此我们需要在图像创建过程中将该数字传递给 VkImageCreateInfo。通过添加 numSamples 参数修改 createImage 函数：

```markdown
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.samples = numSamples;
    ...
}
```
现在，使用 VK_SAMPLE_COUNT_1_BIT 更新对此函数的所有调用 - 随着实现的进展，我们将替换这些值：

```markdown
createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```
我们现在要创建一个多重采样颜色缓冲器。添加一个 createColorResources 函数，并注意我们在这里使用 msaaSamples 作为 createImage 的函数参数。我们还使用了一个 mipmap 级别，因为 Vulkan 规范规定，对于每个像素有多个样本的图像，必须只使用一个 mipmap 级别。另外，此颜色缓冲区不需要 mipmap，因为它不会被用作纹理：

```markdown
void createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
```
为了保持一致性，在 createDepthResources 之前调用此函数：

```markdown
void initVulkan() {
    ...
    createColorResources();
    createDepthResources();
    ...
}
```
现在我们已经有了一个多重采样颜色缓冲器，是时候处理深度了。修改 createDepthResources，并更新深度缓冲器使用的样本数：

```markdown
void createDepthResources() {
    ...
    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    ...
}
```
我们现在已经创建了一些新的 Vulkan 资源，所以当需要的时候不要忘记释放它们：

```markdown
void cleanupSwapChain() {
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
    ...
}
```
并更新 recreateSwapChain，以便在窗口大小调整时，新的颜色图像可以以正确的分辨率重新创建：

```markdown
void recreateSwapChain() {
    ...
    createImageViews();
    createColorResources();
    createDepthResources();
    ...
}
```
我们已经完成了初始 MSAA 设置，现在我们需要开始在我们的图形管道、帧缓冲、渲染通道中使用这个新资源，并看到结果！

## Adding new attachments

首先我们来处理渲染过程。修改 createRenderPass 并更新颜色和深度附件创建信息结构体：

```cpp
void createRenderPass() {
    ...
    colorAttachment.samples = msaaSamples;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
    depthAttachment.samples = msaaSamples;
    ...
}
```
你会发现我们将 finalLayout 从 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR 更改为 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL。这是因为多采样图像不能直接展示，我们需要先将它们解析成常规图像。这个要求对深度缓冲区并不适用，因为它在任何时候都不会被展示。因此我们只需要为颜色添加一个新的附件，也就是所谓的解析附件：

```cpp
    ...
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ...
```
现在必须指导渲染过程将多采样颜色图像解析到常规附件中。创建一个新的附件引用，该引用将指向作为解析目标的颜色缓冲区：

```cpp
    ...
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
```
设置pResolveAttachments子通道结构体成员以指向新创建的附件引用。这足以让渲染过程定义一个多重采样解决操作，使我们能够将图像渲染到屏幕上：

```cpp
    ...
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    ...
```
现在使用新的颜色附件更新渲染过程信息结构体：

```cpp
    ...
    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    ...
```
有了渲染过程后，修改createFramebuffers并将新的图像视图添加到列表中：

```cpp
void createFramebuffers() {
        ...
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };
        ...
}
```
最后，通过修改 createGraphicsPipeline 来告诉新创建的流水线使用多于一个的样本：

```cpp
void createGraphicsPipeline() {
    ...
    multisampling.rasterizationSamples = msaaSamples;
    ...
}
```
现在运行你的程序，你应该会看到以下情况：

![img](https://vulkan-tutorial.com/images/multisampling.png)

就像 Mipmapping，差异可能一开始并不明显。但仔细观察，你会发现边缘不再那么锯齿状，整个图像比原来看起来稍微平滑一些。

![img](https://vulkan-tutorial.com/images/multisampling_comparison.png)

当近距离观察其中一个边缘时，差异更加明显：

![img](https://vulkan-tutorial.com/images/multisampling_comparison2.png)

## Quality improvements

我们当前的MSAA实现有一些限制，可能会影响到更详细场景中输出图像的质量。例如，我们目前没有解决由着色器混叠引起的潜在问题，即MSAA只平滑几何形状的边缘，而不是内部填充。这可能导致在屏幕上渲染出平滑的多边形，但如果它包含高对比度的颜色，应用的纹理看起来仍然会有混叠。解决这个问题的一种方法是启用样本阴影，这将进一步提高图像质量，尽管会增加额外的性能成本：

```c++
void createLogicalDevice() {
    ...
    deviceFeatures.sampleRateShading = VK_TRUE; // 为设备启用样本阴影功能
    ...
}

void createGraphicsPipeline() {
    ...
    multisampling.sampleShadingEnable = VK_TRUE; // 在管线中启用样本阴影
    multisampling.minSampleShading = .2f; // 样本阴影的最小分数；越接近1越平滑
    ...
}
```
在这个例子中，我们将禁用样本阴影，但在某些情况下，可能会注意到质量的改善。

![img](https://vulkan-tutorial.com/images/sample_shading.png)

## Conclusion

到达这一点需要大量工作，但现在你终于有了一个良好的Vulkan程序的基础。你现在掌握的Vulkan的基本原则知识应该足够开始探索更多的特性，例如：

- 压入常量
- 实例渲染
- 动态uniforms
- 分离图像和采样器描述符
- 管线缓存
- 多线程命令缓冲生成
- 多次子通道
- 计算着色器

当前的程序可以通过添加Blinn-Phong光照、后处理效果和阴影映射等方式进行多方面的扩展。你应该能够从其他API的教程中学习这些效果的工作原理，因为尽管Vulkan的明确性，许多概念仍然相同。
