#pragma once

#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "stb_image.h"

#include "DDSLoader.h"

#ifdef _DEBUG
#define VK_CHECK(x) if((x) != VK_SUCCESS) { std::cerr << "[" << __func__ << "] an error occurs in Vulkan" << std::endl; std::exit(EXIT_FAILURE); }
#define SDL_CHECK(x) if((x) != SDL_TRUE) { std::cerr << "[" << __func__ << "] an error occurs in SDL" << std::endl; std::exit(EXIT_FAILURE); }
#else
#define VK_CHECK(x) x
#define SDL_CHECK(x) x
#endif

struct VertexData {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;
};

struct PushConstants {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 normalMatrix;
};

class GraphicsEngine {
	// engine version information (requires C++17 or later)
	static constexpr std::string_view engineName = "VulkanGraphicsEngine";
	static constexpr std::uint32_t engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);

	static constexpr VkAllocationCallbacks* allocator = nullptr;

	static constexpr VkFormat desiredFormat = VK_FORMAT_B8G8R8A8_UNORM;
	static constexpr VkFormat desiredDepthFormat = VK_FORMAT_D32_SFLOAT;

	VkExtent2D imageSize_;

	std::uint32_t frame_;

	VkInstance instance_;
	VkDevice device_;

	VkSurfaceKHR surface_;
	VkSwapchainKHR swapchain_;

	VkPhysicalDevice physicalDevice_;
	VkPhysicalDeviceProperties properties_;
	VkPhysicalDeviceMemoryProperties memoryProperties_;
	std::uint32_t queueFamilyIndex_;
	VkQueue deviceQueue_;

	VkCommandPool commandPool_;
	VkCommandBuffer commandBuffer_;

	VkFence fence_;
	
	std::vector<VkImage> defaultImages_;
	std::vector<VkImageView> defaultImageViews_;
	VkImage defaultDepthImage_;
	VkDeviceMemory defaultDepthImageMemory_;
	VkImageView defaultDepthImageView_;
	VkRenderPass defaultRenderPass_;
	std::vector<VkFramebuffer> defaultFramebuffers_;
	std::uint32_t currentFrameIndex_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;

	VkImage textureImage_;
	VkDeviceMemory textureImageMemory_;
	VkImageView textureImageView_;
	VkSampler textureSampler_;

	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;
	std::vector<std::uint8_t*> uniformBuffersMapped_;

	VkShaderModule vertexShaderModule_;
	VkShaderModule fragmentShaderModule_;

	VkDescriptorSetLayout defaultDescriptorSetLayout_;
	VkDescriptorPool defaultDescriptorPool_;
	std::vector<VkDescriptorSet> defaultDescriptorSets_;

	VkPipelineLayout defaultPipelineLayout_;
	VkPipeline defaultGraphicsPipeline_;

	// resorce creation

	void createInstance(const char*, std::uint32_t, const std::vector<const char*>&, const std::vector<const char*>&);

	void createSurface_SDL(SDL_Window*);

	void getPhysicalDevice();
	void getQueueFamilyIndex();

	void createDevice(const std::vector<const char*>&, const std::vector<const char*>&);
	void getDeviceQueue();

	void createSwapchain();

	void createDefaultImages();
	void createDefaultImageViews();
	void createDefaultDepthImage();
	void createDefaultDepthImageView();
	void createDefaultRenderPass();
	void createDefaultFramebuffers();

	void createBuffer(std::size_t, VkBufferUsageFlags, VkBuffer&);
	void createImage(VkFormat, const VkExtent3D&, VkImageUsageFlags, VkSampleCountFlagBits, VkImage&);
	void createImageView(const VkImage&, VkFormat, const VkImageSubresourceRange&, VkImageView&);
	template<typename T, std::enable_if_t<std::is_same_v<T, VkBuffer> || std::is_same_v<T, VkImage>, std::nullptr_t> = nullptr>
	void allocateDeviceMemory(const T&, VkDeviceMemory&, VkMemoryPropertyFlags);

	template<typename T>
	void createVertexBuffer(const std::vector<T>&);
	template<typename T, std::enable_if_t<std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::uint32_t>, std::nullptr_t> = nullptr>
	void createIndexBuffer(const std::vector<T>&);
	template<typename T>
	void createUniformBuffer(std::size_t);

	void createTextureImage(const std::vector<std::uint8_t>&, VkFormat, VkExtent2D);
	void createTextureImageView(VkFormat);
	void createTextureSampler();

	void createShaderModule(const char*, const char*);

	void createDefaultDescriptorSetLayout();
	void createDefaultDescriptorPool(std::size_t);
	void createDefaultDescriptorSets(std::size_t);

	void createDefaultPipelineLayout();
	void createDefaultGraphicsPipeline();

	void createCommandPool();
	void createCommandBuffer();

	void createFence();

	// ----------------

	// command utilities

	void acquireNextImage();
	
	void beginCommand();
	void endCommand();
	void submitCommands();

	template<typename Func>
	void submitCommandsOnce(Func);

	void present();

	VkResult waitFence();
	void resetFence();

	void beginRenderPass();
	void endRenderPass();

	// -----------------

public:
	GraphicsEngine() = default;
	~GraphicsEngine() = default;

	// disallow copy
	GraphicsEngine(const GraphicsEngine&) = delete;
	GraphicsEngine& operator=(const GraphicsEngine&) = delete;

	GraphicsEngine(GraphicsEngine&&) = default;
	GraphicsEngine& operator=(GraphicsEngine&&) = default;

	void initialize(SDL_Window* window, const char* applicationName, std::uint32_t applicationVersion, const VkExtent2D& imageSize);
	//void deinitialize();

	void draw();
};