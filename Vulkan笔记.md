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
