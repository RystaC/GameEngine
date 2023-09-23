#include "GraphicsEngine.h"

// helper operator for std::find
bool operator==(const VkSurfaceFormatKHR& a, const VkSurfaceFormatKHR& b) { return (a.format == b.format && a.colorSpace == b.colorSpace); }

void GraphicsEngine::createInstance(const char* applicationName, std::uint32_t applicationVersion, const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = applicationName;
	appInfo.applicationVersion = applicationVersion;
	appInfo.pEngineName = engineName.data();
	appInfo.engineVersion = engineVersion;
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instInfo{};
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pApplicationInfo = &appInfo;
	instInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	instInfo.ppEnabledExtensionNames = extensions.data();
	instInfo.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
	instInfo.ppEnabledLayerNames = layers.data();
	
	VK_CHECK(vkCreateInstance(&instInfo, allocator, &instance_));
}

void GraphicsEngine::createSurface_SDL(SDL_Window* window) {
	SDL_CHECK(SDL_Vulkan_CreateSurface(window, instance_, &surface_));
}

void GraphicsEngine::getPhysicalDevice() {
	uint32_t numPhysicalDevices{};
	VK_CHECK(vkEnumeratePhysicalDevices(instance_, &numPhysicalDevices, nullptr));
	if (numPhysicalDevices == 0) {
		std::cerr << "[getPhysicalDevice] failed to find physical devices for Vulkan" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
	VK_CHECK(vkEnumeratePhysicalDevices(instance_, &numPhysicalDevices, physicalDevices.data()));

#ifdef _DEBUG
	for (const auto& pDev : physicalDevices) {
		VkPhysicalDeviceProperties properties{};

		vkGetPhysicalDeviceProperties(pDev, &properties);

		std::cerr << "Device Name: " << properties.deviceName << std::endl;
	}
#endif

	physicalDevice_ = physicalDevices[0];

	vkGetPhysicalDeviceProperties(physicalDevice_, &properties_);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties_);
}

void GraphicsEngine::getQueueFamilyIndex() {
	constexpr VkQueueFlags desiredQueueFlags = VK_QUEUE_GRAPHICS_BIT;

	uint32_t numQueueFamilyProperties{};
	uint32_t queueFamilyIndex = UINT32_MAX;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &numQueueFamilyProperties, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(numQueueFamilyProperties);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &numQueueFamilyProperties, queueFamilyProperties.data());

	for (std::uint32_t i = 0; i < numQueueFamilyProperties; ++i) {
		if ((queueFamilyProperties[i].queueFlags & desiredQueueFlags) != 0) {
			queueFamilyIndex = i;
			break;
		}
	}
	if (queueFamilyIndex == UINT32_MAX) {
		std::cerr << "[getDeviceQueue] failed to find device queue for Vulkan" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	queueFamilyIndex_ = queueFamilyIndex;
}

void GraphicsEngine::createDevice(const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
	float queuePriority = 0.0f;

	VkDeviceQueueCreateInfo queueInfo{};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueCount = 1;
	queueInfo.queueFamilyIndex = queueFamilyIndex_;
	queueInfo.pQueuePriorities = &queuePriority;

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	deviceInfo.ppEnabledExtensionNames = extensions.data();
	deviceInfo.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
	deviceInfo.ppEnabledLayerNames = layers.data();
	deviceInfo.pEnabledFeatures = nullptr;

	VK_CHECK(vkCreateDevice(physicalDevice_, &deviceInfo, allocator, &device_));
}

void GraphicsEngine::getDeviceQueue() {
	vkGetDeviceQueue(device_, queueFamilyIndex_, 0, &deviceQueue_);
}

void GraphicsEngine::createSwapchain() {
	constexpr VkColorSpaceKHR desiredColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	constexpr VkSurfaceFormatKHR desiredSurfaceFormat = { desiredFormat, desiredColorSpace };
	constexpr VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	VkBool32 surfaceSupported{};
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, queueFamilyIndex_, surface_, &surfaceSupported);
	if (!surfaceSupported) {
		std::cerr << "[createSwapchain] physical device does not support surface" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &surfaceCapabilities);

	std::uint32_t numSurfaceFormats{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &numSurfaceFormats, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(numSurfaceFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &numSurfaceFormats, surfaceFormats.data());
	if (std::find(surfaceFormats.begin(), surfaceFormats.end(), desiredSurfaceFormat) == surfaceFormats.end()) {
		std::cerr << "[createSwapchain] physical device does not support desired surface format" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	std::uint32_t numPresentModes{};
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &numPresentModes, nullptr);
	std::vector<VkPresentModeKHR> presentModes(numPresentModes);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &numPresentModes, presentModes.data());
	if (std::find(presentModes.begin(), presentModes.end(), desiredPresentMode) == presentModes.end()) {
		std::cerr << "[createSwapchain] physical device does not support desired present mode" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = surface_;
	swapchainInfo.minImageCount = 2;
	swapchainInfo.imageFormat = desiredFormat;
	swapchainInfo.imageColorSpace = desiredColorSpace;
	swapchainInfo.imageExtent = imageSize_;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainInfo.presentMode = desiredPresentMode;
	swapchainInfo.clipped = VK_TRUE;

	VK_CHECK(vkCreateSwapchainKHR(device_, &swapchainInfo, allocator, &swapchain_));
}

void GraphicsEngine::createDefaultImages() {
	std::uint32_t numImages{};
	VK_CHECK(vkGetSwapchainImagesKHR(device_, swapchain_, &numImages, nullptr));
	std::vector<VkImage> swapchainImages(numImages);
	VK_CHECK(vkGetSwapchainImagesKHR(device_, swapchain_, &numImages, swapchainImages.data()));

	defaultImages_ = std::move(swapchainImages);
}

void GraphicsEngine::createDefaultImageViews() {
	std::vector<VkImageView> imageViews(defaultImages_.size());

	for (auto i = 0; i < imageViews.size(); ++i) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = defaultImages_[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = desiredFormat;
		viewInfo.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};
		// aspectMask, baseMipLevel, levelCount, baseArrayLayer, layerCount
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1, };

		VK_CHECK(vkCreateImageView(device_, &viewInfo, allocator, &imageViews[i]));
	}

	defaultImageViews_ = std::move(imageViews);
}

void GraphicsEngine::createDefaultDepthImage() {
	constexpr VkFormatFeatureFlags desiredFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkFormatProperties formatProperties{};
	vkGetPhysicalDeviceFormatProperties(physicalDevice_, desiredDepthFormat, &formatProperties);

	if ((formatProperties.optimalTilingFeatures & desiredFeatures) != desiredFeatures) {
		std::cerr << "[createDefaultDepthImage] failed to find desired format for depth image" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = imageSize_.width;
	imageInfo.extent.height = imageSize_.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = desiredDepthFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	
	VK_CHECK(vkCreateImage(device_, &imageInfo, allocator, &defaultDepthImage_));

	VkMemoryRequirements memoryRequirements{};
	vkGetImageMemoryRequirements(device_, defaultDepthImage_, &memoryRequirements);
	uint32_t memoryTypeIndex = UINT32_MAX;
	for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
		if ((memoryProperties_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
			memoryTypeIndex = i;
			break;
		}
	}
	if (memoryTypeIndex == UINT32_MAX) {
		std::cerr << "[createDefaultDepthImage] failed to find memory for depth buffer" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VK_CHECK(vkAllocateMemory(device_, &allocateInfo, allocator, &defaultDepthImageMemory_));
	VK_CHECK(vkBindImageMemory(device_, defaultDepthImage_, defaultDepthImageMemory_, 0));
}

void GraphicsEngine::createDefaultDepthImageView() {
	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = defaultDepthImage_;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = VK_FORMAT_D32_SFLOAT;
	imageViewInfo.components = {
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A,
	};
	imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

	VK_CHECK(vkCreateImageView(device_, &imageViewInfo, allocator, &defaultDepthImageView_));
}

void GraphicsEngine::createDefaultRenderPass() {
	VkAttachmentDescription colorAttachmentDesc{};
	colorAttachmentDesc.format = desiredFormat;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentDesc{};
	depthAttachmentDesc.format = desiredDepthFormat;
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentDescription, 2> attachmentDesc{ colorAttachmentDesc, depthAttachmentDesc };

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentRef;
	subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency colorDependency{};
	colorDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	colorDependency.dstSubpass = 0;
	colorDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	colorDependency.srcAccessMask = 0;
	colorDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	colorDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency{};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkSubpassDependency, 2> dependencies{ colorDependency, depthDependency };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachmentDesc.size();
	renderPassInfo.pAttachments = attachmentDesc.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	VK_CHECK(vkCreateRenderPass(device_, &renderPassInfo, allocator, &defaultRenderPass_));
}

void GraphicsEngine::createDefaultFramebuffers() {
	std::vector<VkFramebuffer> framebuffers(defaultImageViews_.size());

	for (auto i = 0; i < framebuffers.size(); ++i) {
		std::array<VkImageView, 2> attachmentViews{};
		attachmentViews[0] = defaultImageViews_[i];
		attachmentViews[1] = defaultDepthImageView_;

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = defaultRenderPass_;
		framebufferInfo.attachmentCount = attachmentViews.size();
		framebufferInfo.pAttachments = attachmentViews.data();
		framebufferInfo.width = imageSize_.width;
		framebufferInfo.height = imageSize_.height;
		framebufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(device_, &framebufferInfo, allocator, &framebuffers[i]));
	}

	defaultFramebuffers_ = std::move(framebuffers);
}

template<typename T>
void GraphicsEngine::createVertexBuffer(const std::vector<T>& vertexData) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.size = sizeof(T) * vertexData.size();
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device_, &bufferInfo, allocator, &vertexBuffer_));

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device_, vertexBuffer_, &memoryRequirements);
	uint32_t memoryTypeIndex = UINT32_MAX;
	for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
		if ((memoryProperties_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
			memoryTypeIndex = i;
			break;
		}
	}
	if (memoryTypeIndex == UINT32_MAX) {
		std::cerr << "[createVertexBuffer] failed to find memory for vertex buffer" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VK_CHECK(vkAllocateMemory(device_, &allocateInfo, allocator, &vertexBufferMemory_));
	VK_CHECK(vkBindBufferMemory(device_, vertexBuffer_, vertexBufferMemory_, 0));

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, vertexBufferMemory_, 0, sizeof(T) * vertexData.size(), 0, reinterpret_cast<void**>(& mappedMemory)));
	
	std::memcpy(mappedMemory, vertexData.data(), sizeof(T) * vertexData.size());

	vkUnmapMemory(device_, vertexBufferMemory_);
}

template<typename T>
void GraphicsEngine::createIndexBuffer(const std::vector<T>& indexData) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferInfo.size = sizeof(T) * indexData.size();
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device_, &bufferInfo, allocator, &indexBuffer_));

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device_, indexBuffer_, &memoryRequirements);
	uint32_t memoryTypeIndex = UINT32_MAX;
	for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
		if ((memoryProperties_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
			memoryTypeIndex = i;
			break;
		}
	}
	if (memoryTypeIndex == UINT32_MAX) {
		std::cerr << "[createIndexBuffer] failed to find memory for index buffer" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VK_CHECK(vkAllocateMemory(device_, &allocateInfo, allocator, &indexBufferMemory_));
	VK_CHECK(vkBindBufferMemory(device_, indexBuffer_, indexBufferMemory_, 0));

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, indexBufferMemory_, 0, sizeof(T) * indexData.size(), 0, reinterpret_cast<void**>(&mappedMemory)));

	std::memcpy(mappedMemory, indexData.data(), sizeof(T) * indexData.size());

	vkUnmapMemory(device_, indexBufferMemory_);
}

template<typename T>
void GraphicsEngine::createUniformBuffer(std::size_t numBuffers) {
	uniformBuffers_.resize(numBuffers);
	uniformBuffersMemory_.resize(numBuffers);
	uniformBuffersMapped_.resize(numBuffers);

	for (auto i = 0; i < numBuffers; ++i) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.size = sizeof(T);
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateBuffer(device_, &bufferInfo, allocator, &uniformBuffers_[i]));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device_, uniformBuffers_[i], &memoryRequirements);
		uint32_t memoryTypeIndex = UINT32_MAX;
		for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
			if ((memoryProperties_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
				memoryTypeIndex = i;
				break;
			}
		}
		if (memoryTypeIndex == UINT32_MAX) {
			std::cerr << "[createIndexBuffer] failed to find memory for index buffer" << std::endl;
			std::exit(EXIT_FAILURE);
		}

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = memoryTypeIndex;

		VK_CHECK(vkAllocateMemory(device_, &allocateInfo, allocator, &uniformBuffersMemory_[i]));
		VK_CHECK(vkBindBufferMemory(device_, uniformBuffers_[i], uniformBuffersMemory_[i], 0));

		VK_CHECK(vkMapMemory(device_, uniformBuffersMemory_[i], 0, sizeof(T), 0, reinterpret_cast<void**>(&uniformBuffersMapped_[i])));
	}
}

void GraphicsEngine::createTextureImage(const std::vector<std::uint8_t>& imageData, VkExtent2D imageSize) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = imageSize.width;
	imageInfo.extent.height = imageSize.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK(vkCreateImage(device_, &imageInfo, allocator, &textureImage_));

	VkMemoryRequirements memoryRequirements{};
	vkGetImageMemoryRequirements(device_, textureImage_, &memoryRequirements);
	uint32_t memoryTypeIndex = UINT32_MAX;
	for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
		if ((memoryProperties_.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
			memoryTypeIndex = i;
			break;
		}
	}
	if (memoryTypeIndex == UINT32_MAX) {
		std::cerr << "[createTextureImage] failed to find memory for texture image" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VK_CHECK(vkAllocateMemory(device_, &allocateInfo, allocator, &textureImageMemory_));
	VK_CHECK(vkBindImageMemory(device_, textureImage_, textureImageMemory_, 0));

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, textureImageMemory_, 0, sizeof(std::uint8_t) * imageData.size(), 0, reinterpret_cast<void**>(&mappedMemory)));

	std::memcpy(mappedMemory, imageData.data(), sizeof(std::uint8_t) * imageData.size());

	vkUnmapMemory(device_, textureImageMemory_);
}

void GraphicsEngine::createTextureImageView() {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage_;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VK_CHECK(vkCreateImageView(device_, &viewInfo, allocator, &textureImageView_));
}

void GraphicsEngine::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties_.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VK_CHECK(vkCreateSampler(device_, &samplerInfo, allocator, &textureSampler_));
}

void GraphicsEngine::createShaderModule(const char* vertexSPIRVPath, const char* fragmentSPIRVPath) {
	{
		std::ifstream file(vertexSPIRVPath, std::ios::in | std::ios::binary);
		if (file.fail()) {
			std::cerr << "[createShaderModule]: failed to read a file: " << vertexSPIRVPath << std::endl;
			std::exit(EXIT_FAILURE);
		}

		file.seekg(0, std::ios_base::end);
		std::size_t fileSize = file.tellg();
		file.seekg(0, std::ios_base::beg);

		std::vector<uint8_t> bin(fileSize);
		file.read(reinterpret_cast<char*>(bin.data()), sizeof(std::uint8_t) * fileSize);

		VkShaderModuleCreateInfo shaderInfo{};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.codeSize = bin.size();
		shaderInfo.pCode = reinterpret_cast<std::uint32_t*>(bin.data());

		VK_CHECK(vkCreateShaderModule(device_, &shaderInfo, allocator, &vertexShaderModule_));
	}

	{
		std::ifstream file(fragmentSPIRVPath, std::ios::in | std::ios::binary);
		if (file.fail()) {
			std::cerr << "[createShaderModule]: failed to read a file: " << fragmentSPIRVPath << std::endl;
			std::exit(EXIT_FAILURE);
		}

		file.seekg(0, std::ios_base::end);
		std::size_t fileSize = file.tellg();
		file.seekg(0, std::ios_base::beg);

		std::vector<uint8_t> bin(fileSize);
		file.read(reinterpret_cast<char*>(bin.data()), sizeof(std::uint8_t) * fileSize);

		VkShaderModuleCreateInfo shaderInfo{};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.codeSize = bin.size();
		shaderInfo.pCode = reinterpret_cast<std::uint32_t*>(bin.data());

		VK_CHECK(vkCreateShaderModule(device_, &shaderInfo, allocator, &fragmentShaderModule_));
	}
}

void GraphicsEngine::createDefaultDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uniformLayoutBinding{};
	uniformLayoutBinding.binding = 0;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.descriptorCount = 1;
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings{ uniformLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VK_CHECK(vkCreateDescriptorSetLayout(device_, &layoutInfo, allocator, &defaultDescriptorSetLayout_));
}

void GraphicsEngine::createDefaultDescriptorPool(std::size_t numDescriptors) {
	VkDescriptorPoolSize uniformPoolSize{};
	uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformPoolSize.descriptorCount = static_cast<std::uint32_t>(numDescriptors);

	VkDescriptorPoolSize samplderPoolSize{};
	samplderPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplderPoolSize.descriptorCount = static_cast<std::uint32_t>(numDescriptors);

	std::array<VkDescriptorPoolSize, 2> poolSizes{ uniformPoolSize, samplderPoolSize };

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<std::uint32_t>(numDescriptors);

	VK_CHECK(vkCreateDescriptorPool(device_, &poolInfo, allocator, &defaultDescriptorPool_));
}

void GraphicsEngine::createDefaultDescriptorSets(std::size_t numDescriptorSets) {
	std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, defaultDescriptorSetLayout_);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = defaultDescriptorPool_;
	allocateInfo.descriptorSetCount = static_cast<std::uint32_t>(numDescriptorSets);
	allocateInfo.pSetLayouts = layouts.data();

	defaultDescriptorSets_.resize(numDescriptorSets);

	VK_CHECK(vkAllocateDescriptorSets(device_, &allocateInfo, defaultDescriptorSets_.data()));

	for (auto i = 0; i < numDescriptorSets; ++i) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers_[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView_;
		imageInfo.sampler = textureSampler_;

		VkWriteDescriptorSet descriptorUniformWrite{};
		descriptorUniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorUniformWrite.dstSet = defaultDescriptorSets_[i];
		descriptorUniformWrite.dstBinding = 0;
		descriptorUniformWrite.dstArrayElement = 0;
		descriptorUniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorUniformWrite.descriptorCount = 1;
		descriptorUniformWrite.pBufferInfo = &bufferInfo;

		VkWriteDescriptorSet descriptorSamplerWrite{};
		descriptorSamplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSamplerWrite.dstSet = defaultDescriptorSets_[i];
		descriptorSamplerWrite.dstBinding = 1;
		descriptorSamplerWrite.dstArrayElement = 0;
		descriptorSamplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSamplerWrite.descriptorCount = 1;
		descriptorSamplerWrite.pImageInfo = &imageInfo;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{ descriptorUniformWrite, descriptorSamplerWrite };

		vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void GraphicsEngine::createDefaultPipelineLayout() {
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &defaultDescriptorSetLayout_;

	VK_CHECK(vkCreatePipelineLayout(device_, &layoutInfo, allocator, &defaultPipelineLayout_));
}

void GraphicsEngine::createDefaultGraphicsPipeline() {
	std::array<VkPipelineShaderStageCreateInfo, 2> stageInfo{};
	stageInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stageInfo[0].module = vertexShaderModule_;
	stageInfo[0].pName = "main";
	stageInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stageInfo[1].module = fragmentShaderModule_;
	stageInfo[1].pName = "main";

	std::array<VkVertexInputBindingDescription, 1> inputBindingDesc = {
		VkVertexInputBindingDescription{0, sizeof(VertexData), VK_VERTEX_INPUT_RATE_VERTEX},
	};

	std::array<VkVertexInputAttributeDescription, 4> inputAttributeDesc = {
		VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, position)},
		VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, normal)},
		VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexData, texCoord)},
		VkVertexInputAttributeDescription{3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexData, color)},
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<std::uint32_t>(inputBindingDesc.size());
	vertexInputInfo.pVertexBindingDescriptions = inputBindingDesc.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(inputAttributeDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = inputAttributeDesc.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(imageSize_.width), static_cast<float>(imageSize_.height), 0.0f, 1.0f };
	VkRect2D scissor = { {0, 0}, imageSize_ };

	VkPipelineViewportStateCreateInfo viewportInfo{};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	rasterizationInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleInfo{};
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.minDepthBounds = 0.0f;
	depthStencilInfo.maxDepthBounds = 1.0f;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	VkPipelineDynamicStateCreateInfo dynamicInfo{};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.dynamicStateCount = 0;
	dynamicInfo.pDynamicStates = nullptr;

	VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
	graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineInfo.stageCount = static_cast<std::uint32_t>(stageInfo.size());
	graphicsPipelineInfo.pStages = stageInfo.data();
	graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	graphicsPipelineInfo.pViewportState = &viewportInfo;
	graphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
	graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
	graphicsPipelineInfo.pColorBlendState = &colorBlendInfo;
	graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
	graphicsPipelineInfo.pDynamicState = &dynamicInfo;
	graphicsPipelineInfo.layout = defaultPipelineLayout_;
	graphicsPipelineInfo.renderPass = defaultRenderPass_;
	graphicsPipelineInfo.subpass = 0;

	VK_CHECK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, allocator, &defaultGraphicsPipeline_));
}

void GraphicsEngine::createCommandPool() {
	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.queueFamilyIndex = queueFamilyIndex_;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, allocator, &commandPool_));
}

void GraphicsEngine::createCommandBuffer() {
	VkCommandBufferAllocateInfo commandBufferInfo{};
	commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferInfo.commandPool = commandPool_;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device_, &commandBufferInfo, &commandBuffer_));
}

void GraphicsEngine::createFence() {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VK_CHECK(vkCreateFence(device_, &fenceInfo, allocator, &fence_));
}

void GraphicsEngine::acquireNextImage() {
	VK_CHECK(vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, VK_NULL_HANDLE, fence_, &currentFrameIndex_));
	VK_CHECK(vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(device_, 1, &fence_));
}

void GraphicsEngine::beginCommand() {
	VkCommandBufferInheritanceInfo inheritanceInfo{};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.framebuffer = defaultFramebuffers_[currentFrameIndex_];

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer_, &beginInfo));
}

void GraphicsEngine::endCommand() {
	vkEndCommandBuffer(commandBuffer_);
}

void GraphicsEngine::submitCommands() {
	VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer_;
	submitInfo.pWaitDstStageMask = &waitStageFlags;

	VK_CHECK(vkQueueSubmit(deviceQueue_, 1, &submitInfo, fence_));
}

template<typename Func>
void GraphicsEngine::submitCommandsOnce(Func func) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer_, &beginInfo));

	func();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer_;

	VK_CHECK(vkQueueSubmit(deviceQueue_, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(deviceQueue_));

	VK_CHECK(vkResetCommandBuffer(commandBuffer_, 0));
}

void GraphicsEngine::present() {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain_;
	presentInfo.pImageIndices = &currentFrameIndex_;

	VK_CHECK(vkQueuePresentKHR(deviceQueue_, &presentInfo));
}

VkResult GraphicsEngine::waitFence() {
	return vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);
}

void GraphicsEngine::resetFence() {
	vkResetFences(device_, 1, &fence_);
}

void GraphicsEngine::beginRenderPass() {
	VkClearValue colorClearValue{}, depthClearValue{};
	colorClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	depthClearValue.depthStencil = { 1.0f, 0 };

	std::array<VkClearValue, 2> clearValues{ colorClearValue, depthClearValue };

	VkRenderPassBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.framebuffer = defaultFramebuffers_[currentFrameIndex_];
	beginInfo.renderPass = defaultRenderPass_;
	beginInfo.renderArea.extent = imageSize_;
	beginInfo.clearValueCount = static_cast<std::uint32_t>(clearValues.size());
	beginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer_, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void GraphicsEngine::endRenderPass() {
	vkCmdEndRenderPass(commandBuffer_);
}

void GraphicsEngine::initialize(SDL_Window* window, const char* applicationName, std::uint32_t applicationVersion, const VkExtent2D& imageSize) {
	imageSize_ = imageSize;

	std::uint32_t numInstanceExtensions{};
	SDL_CHECK(SDL_Vulkan_GetInstanceExtensions(window, &numInstanceExtensions, nullptr));
	std::vector<const char*> instanceExtensions(numInstanceExtensions);
	SDL_CHECK(SDL_Vulkan_GetInstanceExtensions(window, &numInstanceExtensions, instanceExtensions.data()));

	std::vector<const char*> instanceLayers{};

	createInstance(applicationName, applicationVersion, instanceExtensions, instanceLayers);

	createSurface_SDL(window);

	getPhysicalDevice();
	getQueueFamilyIndex();

	std::vector<const char*> deviceExtensions{ "VK_KHR_swapchain" };
	std::vector<const char*> deviceLayers{};

	createDevice(deviceExtensions, deviceLayers);
	getDeviceQueue();

	createSwapchain();

	createCommandPool();
	createCommandBuffer();
	createFence();

	createDefaultImages();
	createDefaultImageViews();
	createDefaultDepthImage();
	createDefaultDepthImageView();
	createDefaultRenderPass();
	createDefaultFramebuffers();

	std::vector<VertexData> vertexData = {
		// -x
		{glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
		{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
		{glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
		{glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)},
		// +x
		{glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
		{glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)},
		// -y
		{glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
		{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)},
		// +y
		{glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
		{glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
		{glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f),glm::vec2(1.0f, 0.0f),  glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
		{glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)},
		// -z
		{glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
		{glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
		{glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)},
		// +z
		{glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
		{glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
		{glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f)},
	};
	createVertexBuffer(vertexData);

	std::vector<std::uint16_t> indexData = {
		// -x
		0, 1, 2,
		2, 1, 3,
		// +x
		4, 5, 6,
		6, 5, 7,
		// -y
		8, 9, 10,
		10, 9, 11,
		// +y
		12, 13, 14,
		14, 13, 15,
		// -z
		16, 17, 18,
		18, 17, 19,
		// +z
		20, 21, 22,
		22, 21, 23,
	};
	createIndexBuffer(indexData);

	createShaderModule("basic.vert.spv", "basic.frag.spv");

	createUniformBuffer<UniformBufferObject>(defaultFramebuffers_.size());

	{
		std::int32_t texWidth, texHeight, texChannels;
		stbi_uc* texRawData = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		if (!texRawData) {
			std::cerr << "[initialize] failed to read texture" << std::endl;
			std::exit(EXIT_FAILURE);
		}
		std::vector<std::uint8_t> texData(texRawData, texRawData + (texWidth * texHeight * 4));
		stbi_image_free(texRawData);

		createTextureImage(texData, { static_cast<std::uint32_t>(texWidth), static_cast<std::uint32_t>(texHeight) });
	}

	createTextureImageView();
	createTextureSampler();

	createDefaultDescriptorSetLayout();
	createDefaultDescriptorPool(defaultFramebuffers_.size());
	createDefaultDescriptorSets(defaultFramebuffers_.size());

	createDefaultPipelineLayout();
	createDefaultGraphicsPipeline();

	acquireNextImage();
}

void GraphicsEngine::draw() {
	std::size_t offset = 0;

	auto model = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(frame_)), glm::vec3(1.0f, 0.0f, 0.0f));
	model *= glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(frame_ * 2)), glm::vec3(0.0f, 0.0f, 1.0f));
	++frame_;
	float cameraLength = 10.0f;
	auto view = glm::lookAt(glm::vec3(cameraLength * glm::cos(glm::radians(static_cast<float>(frame_))), 0.0f, cameraLength * glm::sin(glm::radians(static_cast<float>(frame_)))), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	auto projection = glm::perspective(glm::radians(45.0f), static_cast<float>(imageSize_.width) / imageSize_.height, 0.1f, 100.0f);
	projection[1][1] *= -1;

	auto normalMatrix = glm::transpose(glm::inverse(model));

	UniformBufferObject uniformBufferObject{ model, view, projection, normalMatrix };

	std::memcpy(uniformBuffersMapped_[currentFrameIndex_], &uniformBufferObject, sizeof(UniformBufferObject));

	beginCommand();
	beginRenderPass();

	vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultGraphicsPipeline_);
	vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &vertexBuffer_, &offset);
	vkCmdBindIndexBuffer(commandBuffer_, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultPipelineLayout_, 0, 1, &defaultDescriptorSets_[currentFrameIndex_], 0, nullptr);
	vkCmdDrawIndexed(commandBuffer_, 36, 1, 0, 0, 0);

	endRenderPass();
	endCommand();

	submitCommands();

	switch (waitFence()) {
	case VK_SUCCESS:
		present();
		break;
	case VK_TIMEOUT:
		std::cerr << "[draw] command execution timed out" << std::endl;
		std::exit(EXIT_FAILURE);
		break;
	default:
		break;
	}
	resetFence();

	acquireNextImage();
}