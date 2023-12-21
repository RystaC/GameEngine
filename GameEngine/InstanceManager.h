#pragma once

#include "common.h"

namespace vkw {
	class InstanceManager {
		std::shared_ptr<Instance> instance_;
		VkAllocationCallbacks* allocator_;

	public:
		InstanceManager(VkAllocationCallbacks* allocator = nullptr) noexcept : instance_(), allocator_(allocator) {};

		VkResult initialize(const char* applicationName, std::uint32_t applicationVersion, const char* engineName, std::uint32_t engineVersion, const std::vector<const char*>& extensions, const std::vector<const char*>& layers);

		
	};
}