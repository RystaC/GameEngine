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
#include <unordered_map>
#include <vector>

#ifdef _DEBUG
#define VK_CHECK(x) if((x) != VK_SUCCESS) { std::cerr << "[" << __func__ << "] an error occurs in Vulkan" << std::endl; std::exit(EXIT_FAILURE); }
#define SDL_CHECK(x) if((x) != SDL_TRUE) { std::cerr << "[" << __func__ << "] an error occurs in SDL" << std::endl; std::exit(EXIT_FAILURE); }
#else
#define VK_CHECK(x) x
#define SDL_CHECK(x) x
#endif

struct VertexData {
	glm::vec3 position;
	glm::vec4 color;
};

struct PushConstants {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

class GraphicsEngine {
	// engine version information (requires C++17 or later)
	static constexpr std::string_view engineName = "VulkanGraphicsEngine";
	static constexpr std::uint32_t engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);

	static constexpr VkAllocationCallbacks* allocator = nullptr;

	static constexpr VkFormat desiredFormat = VK_FORMAT_B8G8R8A8_UNORM;

	VkExtent2D imageSize_;

	std::uint32_t frame_;

	VkInstance instance_;
	VkDevice device_;

	VkSurfaceKHR surface_;
	VkSwapchainKHR swapchain_;

	VkPhysicalDevice physicalDevice_;
	VkPhysicalDeviceMemoryProperties memoryProperties_;
	std::uint32_t queueFamilyIndex_;
	VkQueue deviceQueue_;

	VkCommandPool commandPool_;
	VkCommandBuffer commandBuffer_;

	VkFence fence_;
	
	std::vector<VkImage> defaultImages_;
	std::vector<VkImageView> defaultImageViews_;
	VkRenderPass defaultRenderPass_;
	std::vector<VkFramebuffer> defaultFramebuffers_;
	std::uint32_t currentFrameIndex_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;

	VkShaderModule vertexShaderModule_;
	VkShaderModule fragmentShaderModule_;

	VkDescriptorSetLayout defaultDescriptorsetLayout_;

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
	void createDefaultRenderPass();
	void createDefaultFramebuffers();

	template<typename T>
	void createVertexBuffer(const std::vector<T>&);
	template<typename T>
	void createIndexBuffer(const std::vector<T>&);

	void createShaderModule(const char*, const char*);

	void createDefaultPipelineLayout();
	void createDefaultGraphicsPipeline();

	void createCommandPool();
	void createCommandBuffer();

	void createFence();

	// ----------------

	// command utilities

	void initializeImages();

	void acquireNextImage();
	
	void beginCommand();
	void endCommand();
	void submitCommands();

	void present();

	VkResult waitFence();
	void resetFence();

	void beginRenderPass();
	void endRenderPass();

	void barrierReadToWrite();
	void barrierWriteToRead();

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