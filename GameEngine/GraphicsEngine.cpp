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
	//colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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
	renderPassInfo.attachmentCount = static_cast<std::uint32_t>(attachmentDesc.size());
	renderPassInfo.pAttachments = attachmentDesc.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDesc;
	renderPassInfo.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
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
		framebufferInfo.attachmentCount = static_cast<std::uint32_t>(attachmentViews.size());
		framebufferInfo.pAttachments = attachmentViews.data();
		framebufferInfo.width = imageSize_.width;
		framebufferInfo.height = imageSize_.height;
		framebufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(device_, &framebufferInfo, allocator, &framebuffers[i]));
	}

	defaultFramebuffers_ = std::move(framebuffers);
}

void GraphicsEngine::createBuffer(std::size_t bufferSize, VkBufferUsageFlags usage, VkBuffer& buffer) {

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = usage;
	bufferInfo.size = bufferSize;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateBuffer(device_, &bufferInfo, allocator, &buffer));
}

void GraphicsEngine::createImage2D(VkFormat format, const VkExtent3D& imageSize, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, VkImage& image) {

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = imageSize;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	// TODO: use VK_IMAGE_TILING_OPTIMAL (staging buffer needed)
	imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = sampleCount;

	VK_CHECK(vkCreateImage(device_, &imageInfo, allocator, &image));
}

void GraphicsEngine::createImageView2D(const VkImage & image, VkFormat format, const VkImageSubresourceRange & subresorceRange, VkImageView & imageView) {

	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = image;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = format;
	imageViewInfo.components = {
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A,
	};
	imageViewInfo.subresourceRange = subresorceRange;

	VK_CHECK(vkCreateImageView(device_, &imageViewInfo, allocator, &imageView));
}

template<typename T, std::enable_if_t<std::is_same_v<T, VkBuffer> || std::is_same_v<T, VkImage>, std::nullptr_t>>
void GraphicsEngine::allocateDeviceMemory(const T& destObject, VkDeviceMemory& deviceMemory, VkMemoryPropertyFlags memoryPropertyFlags) {

	VkMemoryRequirements memoryRequirements{};
	if constexpr (std::is_same_v<T, VkBuffer>) {
		vkGetBufferMemoryRequirements(device_, destObject, &memoryRequirements);
	}
	else if constexpr (std::is_same_v<T, VkImage>) {
		vkGetImageMemoryRequirements(device_, destObject, &memoryRequirements);
	}

	uint32_t memoryTypeIndex = UINT32_MAX;
	for (std::uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
		if (memoryProperties_.memoryTypes[i].propertyFlags & memoryPropertyFlags) {
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

	VK_CHECK(vkAllocateMemory(device_, &allocateInfo, allocator, &deviceMemory));
	
	if constexpr (std::is_same_v<T, VkBuffer>) {
		VK_CHECK(vkBindBufferMemory(device_, destObject, deviceMemory, 0));
	}
	else if constexpr (std::is_same_v<T, VkImage>) {
		VK_CHECK(vkBindImageMemory(device_, destObject, deviceMemory, 0));
	}
}

template<typename T>
void GraphicsEngine::createVertexBuffer(const std::vector<T>& data, VertexBuffer<T>& buffer) {

	auto bufferSize = sizeof(T) * data.size();
	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer.buffer);

	allocateDeviceMemory(buffer.buffer, buffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, buffer.memory, 0, sizeof(T) * data.size(), 0, reinterpret_cast<void**>(&mappedMemory)));

	std::memcpy(mappedMemory, data.data(), sizeof(T) * data.size());

	vkUnmapMemory(device_, buffer.memory);
}

template<typename T, std::enable_if_t<std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::uint32_t>, std::nullptr_t>>
void GraphicsEngine::createIndexBuffer(const std::vector<T>& data, IndexBuffer& buffer) {

	if constexpr (std::is_same_v<T, std::uint16_t>) {
		buffer.indexType = VK_INDEX_TYPE_UINT16;
	}
	else if constexpr (std::is_same_v<T, std::uint32_t>) {
		buffer.indexType = VK_INDEX_TYPE_UINT32;
	}

	buffer.size = data.size();

	auto bufferSize = sizeof(T) * data.size();
	createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, buffer.buffer);

	allocateDeviceMemory(buffer.buffer, buffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, buffer.memory, 0, sizeof(T) * data.size(), 0, reinterpret_cast<void**>(&mappedMemory)));

	std::memcpy(mappedMemory, data.data(), sizeof(T) * data.size());

	vkUnmapMemory(device_, buffer.memory);
}

template<typename T>
void GraphicsEngine::createUniformBuffer(UniformBuffer<T>& buffer) {

	createBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, buffer.buffer);

	allocateDeviceMemory(buffer.buffer, buffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	VK_CHECK(vkMapMemory(device_, buffer.memory, 0, sizeof(T), 0, reinterpret_cast<void**>(&buffer.pointer)));
}

template<typename T>
void GraphicsEngine::createConstantUnifromBuffer(const T* data, UniformBuffer<T>& buffer) {

	createBuffer(sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, buffer.buffer);

	allocateDeviceMemory(buffer.buffer, buffer.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, buffer.memory, 0, sizeof(T), 0, reinterpret_cast<void**>(&mappedMemory)));

	std::memcpy(mappedMemory, data, sizeof(T));

	vkUnmapMemory(device_, buffer.memory);
}

void GraphicsEngine::createTexture(const std::vector<std::uint8_t>& data, VkFormat format, const VkExtent3D& size, Texture& texture) {

	createImage2D(format, size, VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, texture.image);
	allocateDeviceMemory(texture.image, texture.memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	createImageView2D(texture.image, format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, texture.view);

	std::uint8_t* mappedMemory{};
	VK_CHECK(vkMapMemory(device_, texture.memory, 0, sizeof(std::uint8_t) * data.size(), 0, reinterpret_cast<void**>(&mappedMemory)));

	std::memcpy(mappedMemory, data.data(), sizeof(std::uint8_t) * data.size());

	vkUnmapMemory(device_, texture.memory);
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
	VkDescriptorSetLayoutBinding transformLayoutBinding{};
	transformLayoutBinding.binding = 0;
	transformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	transformLayoutBinding.descriptorCount = 1;
	transformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding materialLayoutBinding{};
	materialLayoutBinding.binding = 1;
	materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialLayoutBinding.descriptorCount = 1;
	materialLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding textureLayoutBinding{};
	textureLayoutBinding.binding = 2;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding sphereLayoutBinding{};
	sphereLayoutBinding.binding = 3;
	sphereLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sphereLayoutBinding.descriptorCount = 1;
	sphereLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding toonLayoutBinding{};
	toonLayoutBinding.binding = 4;
	toonLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	toonLayoutBinding.descriptorCount = 1;
	toonLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 5> bindings{ transformLayoutBinding, materialLayoutBinding, textureLayoutBinding, sphereLayoutBinding, toonLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VK_CHECK(vkCreateDescriptorSetLayout(device_, &layoutInfo, allocator, &defaultDescriptorSetLayout_));
}

void GraphicsEngine::createDefaultDescriptorPool(std::uint32_t descriptorSetCount) {
	VkDescriptorPoolSize transformPoolSize{};
	transformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	transformPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize materialPoolSize{};
	materialPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	materialPoolSize.descriptorCount = 1;

	VkDescriptorPoolSize texturePoolSize{};
	texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texturePoolSize.descriptorCount = 1;

	VkDescriptorPoolSize spherePoolSize{};
	spherePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	spherePoolSize.descriptorCount = 1;

	VkDescriptorPoolSize toonPoolSize{};
	toonPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	toonPoolSize.descriptorCount = 1;

	std::array<VkDescriptorPoolSize, 5> poolSizes{ transformPoolSize, materialPoolSize, texturePoolSize, spherePoolSize, toonPoolSize };

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = descriptorSetCount;

	VK_CHECK(vkCreateDescriptorPool(device_, &poolInfo, allocator, &defaultDescriptorPool_));
}

void GraphicsEngine::createDefaultDescriptorSets(const UniformBuffer<MaterialBufferObject>& materialBuffer, const Texture& materialTexture, const Texture& sphereTexture, const Texture& toonTexture, VkDescriptorSet& descriptorSet) {
	VkDescriptorSetLayout layout = defaultDescriptorSetLayout_;

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = defaultDescriptorPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;

	VK_CHECK(vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet));

	VkDescriptorBufferInfo transformBufferInfo{};
	transformBufferInfo.buffer = transformBuffer_.buffer;
	transformBufferInfo.offset = 0;
	transformBufferInfo.range = sizeof(decltype(transformBuffer_)::type);

	VkDescriptorBufferInfo materialBufferInfo{};
	materialBufferInfo.buffer = materialBuffer.buffer;
	materialBufferInfo.offset = 0;
	materialBufferInfo.range = sizeof(MaterialBufferObject);

	VkDescriptorImageInfo textureInfo{};
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureInfo.imageView = materialTexture.view;
	textureInfo.sampler = textureSampler_;

	VkDescriptorImageInfo sphereInfo{};
	sphereInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	sphereInfo.imageView = sphereTexture.view;
	sphereInfo.sampler = textureSampler_;

	VkDescriptorImageInfo toonInfo{};
	toonInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	toonInfo.imageView = toonTexture.view;
	toonInfo.sampler = textureSampler_;

	VkWriteDescriptorSet descriptorTransformWrite{};
	descriptorTransformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorTransformWrite.dstSet = descriptorSet;
	descriptorTransformWrite.dstBinding = 0;
	descriptorTransformWrite.dstArrayElement = 0;
	descriptorTransformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorTransformWrite.descriptorCount = 1;
	descriptorTransformWrite.pBufferInfo = &transformBufferInfo;

	VkWriteDescriptorSet descriptorMaterialWrite{};
	descriptorMaterialWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorMaterialWrite.dstSet = descriptorSet;
	descriptorMaterialWrite.dstBinding = 1;
	descriptorMaterialWrite.dstArrayElement = 0;
	descriptorMaterialWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorMaterialWrite.descriptorCount = 1;
	descriptorMaterialWrite.pBufferInfo = &materialBufferInfo;

	VkWriteDescriptorSet descriptorTextureWrite{};
	descriptorTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorTextureWrite.dstSet = descriptorSet;
	descriptorTextureWrite.dstBinding = 2;
	descriptorTextureWrite.dstArrayElement = 0;
	descriptorTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorTextureWrite.descriptorCount = 1;
	descriptorTextureWrite.pImageInfo = &textureInfo;

	VkWriteDescriptorSet descriptorSphereWrite{};
	descriptorSphereWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorSphereWrite.dstSet = descriptorSet;
	descriptorSphereWrite.dstBinding = 3;
	descriptorSphereWrite.dstArrayElement = 0;
	descriptorSphereWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSphereWrite.descriptorCount = 1;
	descriptorSphereWrite.pImageInfo = &sphereInfo;

	VkWriteDescriptorSet descriptorToonWrite{};
	descriptorToonWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorToonWrite.dstSet = descriptorSet;
	descriptorToonWrite.dstBinding = 4;
	descriptorToonWrite.dstArrayElement = 0;
	descriptorToonWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorToonWrite.descriptorCount = 1;
	descriptorToonWrite.pImageInfo = &toonInfo;

	std::array<VkWriteDescriptorSet, 5> descriptorWrites{ descriptorTransformWrite, descriptorMaterialWrite, descriptorTextureWrite, descriptorSphereWrite, descriptorToonWrite };

	vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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
	VkVertexInputBindingDescription{0, sizeof(PMXVertexAttribute), VK_VERTEX_INPUT_RATE_VERTEX},
	};

	std::array<VkVertexInputAttributeDescription, 7> inputAttributeDesc = {
		VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PMXVertexAttribute, position)},
		VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(PMXVertexAttribute, normal)},
		VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(PMXVertexAttribute, uv)},
		VkVertexInputAttributeDescription{3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(PMXVertexAttribute, a_uv1)},
		VkVertexInputAttributeDescription{4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(PMXVertexAttribute, a_uv2)},
		VkVertexInputAttributeDescription{5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(PMXVertexAttribute, a_uv3)},
		VkVertexInputAttributeDescription{6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(PMXVertexAttribute, a_uv4)},
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
	rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
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
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
	colorClearValue.color = { 0.8f, 0.8f, 0.8f, 1.0f };
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
	imageSize_.width = imageSize.width;
	imageSize_.height = imageSize.height;

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

	PMXLoader loader{};
	PMXData modelData{};
	loader.load("assets\\Tda式改変ミク　JKStyle\\Tda式改変ミク　JKStyle.pmx", modelData);

	textures_.resize(modelData.texturePaths.size());
	for (auto i = 0; i < textures_.size(); ++i) {
		//std::cout << texturePathTable[i] << std::endl;
		TexLoader loader{};
		VkExtent3D imageSize{};
		VkFormat imageFormat{};
		std::vector<std::uint8_t> imageData{};
		if (loader.load(modelData.texturePaths[i], imageSize, imageFormat, imageData)) {
			std::cout << "success" << std::endl;
			createTexture(imageData, imageFormat, imageSize, textures_[i]);
		}
		else {
			std::cout << "failure" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	materialBuffers_.resize(materials.size());

	for (auto i = 0; i < materials.size(); ++i) {
		std::cout << "material #" << i << std::endl;
		std::cout << "texture index:" << materials[i].textureIndex << std::endl;
		std::cout << "sphere index: " << materials[i].sphereIndex << std::endl;
		std::cout << "toon index: " << materials[i].toonIndex << std::endl << std::endl;
		MaterialBufferObject materialBufferObject{};
		materialBufferObject.diffuse = materials[i].color.diffuse;
		materialBufferObject.specular = materials[i].color.specular;
		materialBufferObject.specCoef = materials[i].color.specCoef;
		materialBufferObject.ambient = materials[i].color.ambient;
		materialBufferObject.isTextureUsed = (static_cast<std::int8_t>(materials[i].textureIndex) >= 0);
		materialBufferObject.isSphereUsed = (static_cast<std::int8_t>(materials[i].sphereIndex) >= 0);
		materialBufferObject.isToonUsed = (static_cast<std::int8_t>(materials[i].toonIndex) >= 0);
		std::cout << std::boolalpha << materialBufferObject.isToonUsed << std::endl;
		createConstantUnifromBuffer(&materialBufferObject, materialBuffers_[i]);
	}

	for (auto i = 0; i < bones.size(); ++i) {
		std::cout << "bone #" << i << std::endl;
		std::cout << "position x: " << bones[i].position.x << ", y: " << bones[i].position.y << ", z: " << bones[i].position.z << std::endl;
		std::cout << "parent index: " << bones[i].parentIndex << std::endl;
		std::cout << "hierarchy: " << bones[i].hierarchy << std::endl;

		if (bones[i].flags & 0x0020) std::cout << "IK" << std::endl;

		if (bones[i].flags & 0x0080) std::cout << "local transform: parent" << std::endl;
		else std::cout << "local transform: others" << std::endl;

		if (bones[i].flags & 0x0100) std::cout << "rotation" << std::endl;

		if (bones[i].flags & 0x0200) std::cout << "translation" << std::endl;

		if (bones[i].flags & 0x1000) std::cout << "after physics" << std::endl;

		if (bones[i].flags & 0x2000) std::cout << "external parent" << std::endl;

		std::cout << std::endl;
	}

	numIndices_ = static_cast<std::uint32_t>(indexData.size());
	materials_ = std::move(materials);

	createVertexBuffer(vertexData, vertexBuffer_);
	createIndexBuffer(indexData, indexBuffer_);

	createShaderModule("basic.vert.spv", "basic.frag.spv");

	createUniformBuffer(transformBuffer_);

	createTextureSampler();

	defaultDescriptorSets_.resize(materials_.size());

	createDefaultDescriptorSetLayout();
	createDefaultDescriptorPool(static_cast<std::uint32_t>(materials_.size()));
	for (auto i = 0; i < materials_.size(); ++i) {
		auto& texture = static_cast<std::int8_t>(materials_[i].textureIndex) != -1 ? textures_[materials_[i].textureIndex] : textures_[0];
		auto& sphere = static_cast<std::int8_t>(materials_[i].sphereIndex) != -1 ? textures_[materials_[i].sphereIndex] : textures_[0];
		auto& toon = static_cast<std::int8_t>(materials_[i].toonIndex) != -1 ? textures_[materials_[i].toonIndex] : textures_[0];

		createDefaultDescriptorSets(materialBuffers_[i], texture, sphere, toon, defaultDescriptorSets_[i]);
	}

	createDefaultPipelineLayout();
	createDefaultGraphicsPipeline();

	acquireNextImage();
}

void GraphicsEngine::draw() {
	std::size_t offset = 0;

	//auto model = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(frame_)), glm::vec3(1.0f, 0.0f, 0.0f));
	//model *= glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(frame_ * 2)), glm::vec3(0.0f, 0.0f, 1.0f));
	//auto model = glm::mat4(1.0f);
	auto model = glm::rotate(glm::mat4(1.0f), glm::radians(static_cast<float>(frame_)), glm::vec3(0.0f, 1.0f, 0.0f));
	++frame_;
	float cameraLength = 30.0f;
	//auto view = glm::lookAt(glm::vec3(cameraLength * glm::cos(glm::radians(static_cast<float>(frame_))), 0.0f, cameraLength * glm::sin(glm::radians(static_cast<float>(frame_)))), glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	auto view = glm::lookAt(glm::vec3(0.0f, 10.0f, -cameraLength), glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	auto projection = glm::perspective(glm::radians(45.0f), static_cast<float>(imageSize_.width) / imageSize_.height, 0.1f, 100.0f);
	projection[1][1] *= -1;

	auto normalMatrix = glm::transpose(glm::inverse(model));

	TransformBufferObject transformBufferObject{ model, view, projection, normalMatrix };

	std::memcpy(transformBuffer_.pointer, &transformBufferObject, sizeof(decltype(transformBufferObject)));

	beginCommand();
	beginRenderPass();

	vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultGraphicsPipeline_);
	vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &vertexBuffer_.buffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer_, indexBuffer_.buffer, 0, indexBuffer_.indexType);
	for (auto i = 0; i < materials_.size(); ++i) {
		vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultPipelineLayout_, 0, 1, &defaultDescriptorSets_[i], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer_, materials_[i].indexCount, 1, materials_[i].indexOffset, 0, 0);
	}

	endRenderPass();
	endCommand();

	submitCommands();

	switch (waitFence()) {
	case VK_SUCCESS:
		break;
	case VK_TIMEOUT:
		std::cerr << "[draw] command execution timed out" << std::endl;
		std::exit(EXIT_FAILURE);
		break;
	default:
		break;
	}
	resetFence();

	present();

	acquireNextImage();
}