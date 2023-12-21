#include "InstanceManager.h"

namespace vkw {
	VkResult InstanceManager::initialize(const char* applicationName, std::uint32_t applicationVersion, const char* engineName, std::uint32_t engineVersion, const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = applicationName,
			.applicationVersion = applicationVersion,
			.pEngineName = engineName,
			.engineVersion = engineVersion,
			.apiVersion = VK_API_VERSION_1_0,
		};

		VkInstanceCreateInfo instanceInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = size_cast(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = size_cast(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};

		VkInstance instance{};
		auto result = vkCreateInstance(&instanceInfo, allocator_, &instance);

		if (result == VK_SUCCESS) {
			instance_ = std::make_shared<Instance>(instance, allocator_);
			return result;
		}
		else return result;
	}
}