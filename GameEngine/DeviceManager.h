#pragma once

#include "common.h"

namespace vkw {
	class DeviceManager {
		std::shared_ptr<Device> device_;
		VkAllocationCallbacks* allocator_;

	public:
		DeviceManager(VkAllocationCallbacks* allocator = nullptr) noexcept : device_(), allocator_(allocator) {}

		VkResult initialize(const VkPhysicalDevice& physicalDevice, const std::vector<std::uint32_t>& queueFamilyIndices, const std::vector<std::vector<float>>& queuePriorities, const std::vector<const char*>& extensions, const std::vector<const char*>& layers);
	};
}
