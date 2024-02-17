#include "gpch.h"

#ifdef USE_VULKAN_RENDERER

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#ifdef GDEBUG
#define VMA_DEBUG_LOG_FORMAT
#endif

#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanShaderReflect.h"
#include "VulkanTransitionManager.h"
#include "VulkanPipelineStateManager.h"

#include "Core/Application.h"
#include "Core/WindowSystem.h"

#include <SDL_vulkan.h>

using namespace Gleam;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
												   VkDebugUtilsMessageTypeFlagsEXT type,
												   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
												   void* pUserData)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		GLEAM_CORE_ERROR("Vulkan: {0}", pCallbackData->pMessage);
		GLEAM_ASSERT(false);
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		GLEAM_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		GLEAM_CORE_INFO("Vulkan: {0}", pCallbackData->pMessage);
	}
	else
	{
		GLEAM_CORE_TRACE("Vulkan: {0}", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

Scope<GraphicsDevice> GraphicsDevice::Create()
{
	return CreateScope<VulkanDevice>();
}

void GraphicsDevice::Configure(const RendererConfig& config)
{
	As<VulkanSwapchain*>(mSwapchain.get())->Configure(config);
}

MemoryRequirements GraphicsDevice::QueryMemoryRequirements(const HeapDescriptor& descriptor) const
{
	VkBuffer buffer;
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (descriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(As<VkDevice>(mHandle), &createInfo, nullptr, &buffer));

	// Query memory requirements to get the alignment
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(As<VkDevice>(mHandle), buffer, &memoryRequirements);

	return MemoryRequirements
	{
		.size = memoryRequirements.size,
		.alignment = memoryRequirements.alignment
	};
}

Heap GraphicsDevice::AllocateHeap(const HeapDescriptor& descriptor) const
{
	Heap heap(descriptor);
	heap.mDevice = this;

	VkBuffer buffer;
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (descriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(As<VkDevice>(mHandle), &createInfo, nullptr, &buffer));

	// Query memory requirements to get the alignment
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(As<VkDevice>(mHandle), buffer, &memoryRequirements);

	VmaAllocationCreateInfo vmaCreateInfo{};
	vmaCreateInfo.usage = MemoryTypeToVmaMemoryUsage(descriptor.memoryType);
	vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vmaCreateInfo.priority = 1.0f;
	VmaAllocationInfo vmaAllocationInfo{};

	VK_CHECK(vmaAllocateMemoryForBuffer(As<const VulkanDevice*>(this)->GetAllocator(), buffer, &vmaCreateInfo, As<VmaAllocation*>(&heap.mHandle), &vmaAllocationInfo));
	vkDestroyBuffer(As<VkDevice>(mHandle), buffer, nullptr);

	if (heap.mDescriptor.memoryType != MemoryType::GPU)
	{
		VK_CHECK(vmaMapMemory(As<const VulkanDevice*>(this)->GetAllocator(), As<VmaAllocation>(heap.mHandle), &heap.mContents));
	}

	heap.mDescriptor.size = vmaAllocationInfo.size;
	heap.mAlignment = memoryRequirements.alignment;
	return heap;
}

Texture GraphicsDevice::AllocateTexture(const TextureDescriptor& descriptor) const
{
	Texture texture(descriptor);

	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (descriptor.type == TextureType::TextureCube)
	{
		float size = Math::Min(descriptor.size.width, descriptor.size.height);
		texture.mDescriptor.size.width = size;
		texture.mDescriptor.size.height = size;

		createInfo.imageType = VK_IMAGE_TYPE_3D;
		createInfo.usage = usage;
	}
	else
	{
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.usage = descriptor.type == TextureType::RenderTexture ? usage | (Utils::IsColorFormat(descriptor.format) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) : usage;
	}
	createInfo.format = TextureFormatToVkFormat(descriptor.format);
	createInfo.extent.width = static_cast<uint32_t>(descriptor.size.width);
	createInfo.extent.height = static_cast<uint32_t>(descriptor.size.height);
	createInfo.extent.depth = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.mipLevels = texture.mMipMapLevels;

	VmaAllocationCreateInfo vmaCreateInfo{};
	vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateInfo.priority = 1.0f;
	VK_CHECK(vmaCreateImage(As<const VulkanDevice*>(this)->GetAllocator(), &createInfo, &vmaCreateInfo, As<VkImage*>(&texture.mHandle), As<VmaAllocation*>(&texture.mHeap), nullptr));

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.image = As<VkImage>(texture.mHandle);
	viewCreateInfo.viewType = descriptor.type == TextureType::TextureCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = createInfo.format;
	viewCreateInfo.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;

	if (Utils::IsColorFormat(descriptor.format))
	{
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (Utils::IsDepthFormat(descriptor.format))
	{
		viewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (Utils::IsStencilFormat(descriptor.format))
	{
		viewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VK_CHECK(vkCreateImageView(As<VkDevice>(mHandle), &viewCreateInfo, nullptr, As<VkImageView*>(&texture.mView)));

	if (descriptor.sampleCount > 1)
	{
		// TODO: Switch to memoryless MSAA render targets when Tile shading is supported
		createInfo.samples = GetVkSampleCount(descriptor.sampleCount);
		createInfo.usage = Utils::IsColorFormat(descriptor.format) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		VK_CHECK(vmaCreateImage(As<const VulkanDevice*>(this)->GetAllocator(), &createInfo, &vmaCreateInfo, As<VkImage*>(&texture.mMultisampleHandle), As<VmaAllocation*>(&texture.mMultisampleHeap), nullptr));

		viewCreateInfo.image = As<VkImage>(texture.mMultisampleHandle);
		VK_CHECK(vkCreateImageView(As<VkDevice>(mHandle), &viewCreateInfo, nullptr, As<VkImageView*>(&texture.mMultisampleView)));
	}

	return texture;
}

Shader GraphicsDevice::GenerateShader(const TString& entryPoint, ShaderStage stage) const
{
	Shader shader(entryPoint, stage);

	TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(GameInstance->GetDefaultAssetPath().append("Shaders/" + entryPoint + ".spv"));
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = As<uint32_t*>(shaderCode.data());
	VK_CHECK(vkCreateShaderModule(As<VkDevice>(mHandle), &createInfo, nullptr, As<VkShaderModule*>(&shader.mHandle)));
	shader.mReflection = CreateScope<Shader::Reflection>(this, shaderCode);

	return shader;
}

void GraphicsDevice::Dispose(Heap& heap) const
{
	auto allocator = As<const VulkanDevice*>(this)->GetAllocator();
	auto allocation = As<VmaAllocation>(heap.mHandle);

	if (heap.mDescriptor.memoryType != MemoryType::GPU)
	{
		vmaUnmapMemory(allocator, allocation);
	}
	vmaFreeMemory(allocator, allocation);
	heap.mHandle = VK_NULL_HANDLE;
	heap.mDevice = nullptr;
}

void GraphicsDevice::Dispose(Buffer& buffer) const
{
	vkDestroyBuffer(As<VkDevice>(mHandle), As<VkBuffer>(buffer.mHandle), nullptr);
	buffer.mHandle = VK_NULL_HANDLE;
}

void GraphicsDevice::Dispose(Texture& texture) const
{
	vkDestroyImageView(As<VkDevice>(mHandle), As<VkImageView>(texture.mView), nullptr);
	vmaDestroyImage(As<const VulkanDevice*>(this)->GetAllocator(), As<VkImage>(texture.mHandle), As<VmaAllocation>(texture.mHeap));

	if (texture.mDescriptor.sampleCount > 1)
	{
		vkDestroyImageView(As<VkDevice>(mHandle), As<VkImageView>(texture.mMultisampleView), nullptr);
		vmaDestroyImage(As<const VulkanDevice*>(this)->GetAllocator(), As<VkImage>(texture.mMultisampleHandle), As<VmaAllocation>(texture.mMultisampleHeap));
	}

	texture.mHeap = VK_NULL_HANDLE;
	texture.mView = VK_NULL_HANDLE;
	texture.mHandle = VK_NULL_HANDLE;
	texture.mMultisampleHeap = VK_NULL_HANDLE;
	texture.mMultisampleView = VK_NULL_HANDLE;
	texture.mMultisampleHandle = VK_NULL_HANDLE;
}

VulkanDevice::VulkanDevice()
{
	auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();
	const auto& version = GameInstance->GetVersion();

	mSwapchain = CreateScope<VulkanSwapchain>();

	VkResult loadVKResult = volkInitialize();
	GLEAM_ASSERT(loadVKResult == VK_SUCCESS, "Vulkan: Meta-loader failed to load entry points!");

	// Get window subsystem extensions
	uint32_t extensionCount;
	auto SDLextensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

	TArray<const char*> extensions(extensionCount);
	for (uint32_t i = 0; i < extensionCount; ++i)
	{
		extensions[i] = SDLextensions[i];
	}

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = windowSystem->GetConfiguration().title.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(version.major, version.minor, version.patch);
	appInfo.pEngineName = "Gleam Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION);
	appInfo.apiVersion = VULKAN_API_VERSION;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	// Get validation layers
#ifdef GDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	TArray<const char*, 1> validationLayers{ "VK_LAYER_KHRONOS_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
#endif
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Create instance
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &mInstance));
	volkLoadInstanceOnly(mInstance);

#ifdef GDEBUG
	// Create debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

	debugInfo.pfnUserCallback = VulkanDebugCallback;
	debugInfo.pUserData = nullptr;

	VK_CHECK(vkCreateDebugUtilsMessengerEXT(mInstance, &debugInfo, nullptr, &mDebugMessenger));
#endif

	// Get available extensions
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	mExtensions.resize(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, mExtensions.data()));

	// Create device
	uint32_t physicalDeviceCount;
	VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr));

	TArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data()));

	mPhysicalDevice = physicalDevices[0];
	for (VkPhysicalDevice physicalDevice : physicalDevices)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			mPhysicalDevice = physicalDevice;
			break;
		}
	}

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);

	TArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
	mDeviceExtensions.resize(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, mDeviceExtensions.data()));

	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);

	// Create swapchain
	As<VulkanSwapchain*>(mSwapchain.get())->Initialize(this);

	// Get physical device queues
	bool mainQueueFound = false;
	bool computeQueueFound = false;
	bool transferQueueFound = false;

	float queuePriority = 1.0f;
	TArray<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
	for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
	{
		deviceQueueCreateInfo.queueFamilyIndex = i;
		bool queueFamilySupportsGraphics = queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool queueFamiySupportsCompute = queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
		bool queueFamilySupportsTransfer = queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;

		if (queueFamilySupportsGraphics &&
			queueFamiySupportsCompute &&
			queueFamilySupportsTransfer &&
			!mainQueueFound)
		{
			mainQueueFound = true;
			mGraphicsQueue.index = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

			VkBool32 mainQueueSupportsPresent = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, As<VulkanSwapchain*>(mSwapchain.get())->GetSurface(), &mainQueueSupportsPresent);
			GLEAM_ASSERT(mainQueueSupportsPresent, "Vulkan: Main queue does not support presentation!");
		}
		else if (queueFamiySupportsCompute &&
			queueFamilySupportsTransfer &&
			!queueFamilySupportsGraphics &&
			!computeQueueFound)
		{
			computeQueueFound = true;
			mComputeQueue.index = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
		else if (queueFamilySupportsTransfer &&
			!queueFamiySupportsCompute &&
			!queueFamilySupportsGraphics &&
			!transferQueueFound)
		{
			transferQueueFound = true;
			mTransferQueue.index = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
	}

	TArray<const char*> requiredDeviceExtension
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
		VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
		VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME
	};

	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayout{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES };
	scalarBlockLayout.scalarBlockLayout = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.pNext = &scalarBlockLayout;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	VK_CHECK(vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, As<VkDevice*>(&mHandle)));
	volkLoadDevice(As<VkDevice>(mHandle));

	GLEAM_ASSERT(mainQueueFound, "Vulkan: Main queue could not found!");
	vkGetDeviceQueue(As<VkDevice>(mHandle), mGraphicsQueue.index, 0, &mGraphicsQueue.handle);
	if (computeQueueFound)
	{
		vkGetDeviceQueue(As<VkDevice>(mHandle), mComputeQueue.index, 0, &mComputeQueue.handle);
	}
	else
	{
		mComputeQueue = mGraphicsQueue;
		GLEAM_CORE_WARN("Vulkan: Async compute queue family could not found, mapping to main queue.");
	}
	if (transferQueueFound)
	{
		vkGetDeviceQueue(As<VkDevice>(mHandle), mTransferQueue.index, 0, &mTransferQueue.handle);
	}
	else
	{
		mTransferQueue = mGraphicsQueue;
		GLEAM_CORE_WARN("Vulkan: Transfer queue family could not found, mapping to main queue.");
	}

	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK(vkCreatePipelineCache(As<VkDevice>(mHandle), &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

	// Create allocator
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo vmaCreateInfo{};
	vmaCreateInfo.instance = mInstance;
	vmaCreateInfo.device = As<VkDevice>(mHandle);
	vmaCreateInfo.physicalDevice = mPhysicalDevice;
	vmaCreateInfo.vulkanApiVersion = VULKAN_API_VERSION;
	vmaCreateInfo.pVulkanFunctions = &vulkanFunctions;
	VK_CHECK(vmaCreateAllocator(&vmaCreateInfo, &mAllocator));

	VulkanPipelineStateManager::Init(this);
	VulkanTransitionManager::Init(this);

	GLEAM_CORE_INFO("Vulkan: Graphics device created.");
}

VulkanDevice::~VulkanDevice()
{
	VK_CHECK(vkDeviceWaitIdle(As<VkDevice>(mHandle)));

	// Destroy swapchain
	As<VulkanSwapchain*>(mSwapchain.get())->Destroy();

	for (auto& shader : mShaderCache)
	{
		vkDestroyShaderModule(As<VkDevice>(mHandle), As<VkShaderModule>(shader.GetHandle()), nullptr);
	}
	mShaderCache.clear();
	Clear();

	VulkanPipelineStateManager::Destroy();

	// Destroy pipeline cache
	vkDestroyPipelineCache(As<VkDevice>(mHandle), mPipelineCache, nullptr);

	// Destroy allocator
	vmaDestroyAllocator(mAllocator);

	// Destroy device
	vkDestroyDevice(As<VkDevice>(mHandle), nullptr);

	// Destroy instance
#ifdef GDEBUG
	vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
	vkDestroyInstance(mInstance, nullptr);

	GLEAM_CORE_INFO("Vulkan: Graphics device destroyed.");
}

const VulkanQueue& VulkanDevice::GetGraphicsQueue() const
{
	return mGraphicsQueue;
}

const VulkanQueue& VulkanDevice::GetComputeQueue() const
{
	return mComputeQueue;
}

const VulkanQueue& VulkanDevice::GetTransferQueue() const
{
	return mTransferQueue;
}

VkInstance VulkanDevice::GetInstance() const
{
	return mInstance;
}

VkPhysicalDevice VulkanDevice::GetPhysicalDevice() const
{
	return mPhysicalDevice;
}

VkPipelineCache VulkanDevice::GetPipelineCache() const
{
	return mPipelineCache;
}

VmaAllocator VulkanDevice::GetAllocator() const
{
	return mAllocator;
}
#endif
